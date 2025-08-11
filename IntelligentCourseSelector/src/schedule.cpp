#include "schedule.h"
#include <QDebug>
#include <QQueue>
#include <QtGlobal>

ScheduleManager::ScheduleManager(const QList<Course>& courses)
    : allCourses(courses),
    creditLimits(8, 500),  // 默认每学期上限 500
    blockedTime(8, QVector<quint32>(7, 0)),
    totalCreditLimit(0)    // 默认总学分限制为 0
{
    for (const auto& course : allCourses) {
        priorities[course.id] = 5;
    }
}

int ScheduleManager::getPriority(const QString& courseId) const {
    return priorities.value(courseId, 0);
}

void ScheduleManager::setTotalCreditLimit(int credit) {
    totalCreditLimit = credit;
}

void ScheduleManager::setSelectedCourses(const QSet<QString>& courseIds) {
    selectedCourses = courseIds;
}

QList<QString> ScheduleManager::topologicalSort() const {
    QMap<QString,int> indeg;
    for (auto& c : allCourses)
        indeg[c.id] = c.prerequisites.size();
    QQueue<QString> q;
    for (auto it = indeg.cbegin(); it != indeg.cend(); ++it) {
        if (it.value() == 0) q.enqueue(it.key());
    }
    QList<QString> result;
    while (!q.isEmpty()) {
        auto cur = q.dequeue();
        result.append(cur);
        for (auto& c : allCourses) {
            if (c.prerequisites.contains(cur)) {
                indeg[c.id]--;
                if (indeg[c.id] == 0) q.enqueue(c.id);
            }
        }
    }
    if (result.size() != allCourses.size()) {
        qWarning() << "检测到循环先修课程依赖！";
    }
    return result;
}

bool ScheduleManager::generateSchedule() {
    schedule.clear();
    QVector<int> semCredit(8, 0);
    int totalCredit = 0;

    auto order = topologicalSort();
    std::sort(order.begin(), order.end(), [this](const QString& a, const QString& b){
        return priorities[a] > priorities[b];
    });

    for (const auto& cid : order) {
        if (!selectedCourses.contains(cid) && priorities.value(cid, 0) < 5) continue;

        const Course* pc = nullptr;
        for (const auto& c : allCourses) {
            if (c.id == cid) { pc = &c; break; }
        }
        if (!pc) continue;

        int earliest = 0;
        for (const auto& prereq : pc->prerequisites) {
            for (const auto& sc : schedule) {
                if (sc.courseId == prereq) {
                    earliest = qMax(earliest, sc.semester + 1);
                }
            }
        }

        bool placed = false;
        for (int sem = earliest; sem < 8 && !placed; ++sem) {
            if (semCredit[sem] + pc->credit > creditLimits[sem]) continue;
            if (!isPrerequisiteSatisfied(pc->id, sem)) continue;

            for (const auto& off : pc->offerings) {
                ScheduledCourse sc;
                sc.courseId = cid;
                sc.classId = off.id;
                sc.semester = sem;

                if (!checkTimeConflicts(sc)) {
                    schedule.append(sc);
                    semCredit[sem] += pc->credit;
                    totalCredit += pc->credit;
                    placed = true;
                    break;
                }
            }
        }

        if (placed && totalCredit >= totalCreditLimit) {
            break;
        }
    }

    return true;
}

bool ScheduleManager::checkTimeConflicts(const ScheduledCourse& newSc) const {
    const auto& noff = getOffering(newSc);

    for (int day = 0; day < 7; ++day) {
        if ((noff.times[day] & blockedTime[newSc.semester][day]) != 0) {
            return true;
        }
    }

    for (const auto& sc : schedule) {
        if (sc.semester != newSc.semester) continue;
        const auto& eoff = getOffering(sc);
        for (int day = 0; day < 7; ++day) {
            if ((noff.times[day] & eoff.times[day]) != 0) {
                return true;
            }
        }
    }

    return false;
}

const CourseOffering& ScheduleManager::getOffering(const ScheduledCourse& sc) const {
    for (const auto& c : allCourses) {
        if (c.id == sc.courseId) {
            return c.getOffering(sc.classId);
        }
    }
    qWarning() << "getOffering: 未找到课程" << sc.courseId;
    return allCourses.first().getOffering(sc.classId);
}

QList<ScheduledCourse> ScheduleManager::getCoursesForSemester(int sem) const {
    QList<ScheduledCourse> out;
    for (const auto& sc : schedule) {
        if (sc.semester == sem) out.append(sc);
    }
    return out;
}

QList<ScheduledCourse> ScheduleManager::getAllScheduled() const {
    return schedule;
}

void ScheduleManager::setCreditLimit(int semester, int limit) {
    if (semester >= 0 && semester < creditLimits.size()) {
        creditLimits[semester] = limit;
    }
}

void ScheduleManager::setPriority(const QString& courseId, int priority) {
    if (priority >= 0 && priority <= 10) {
        priorities[courseId] = priority;
    }
}

void ScheduleManager::addBlockedTime(int semester, int day, quint32 mask) {
    if (semester >= 0 && semester < 8 && day >= 0 && day < 7) {
        blockedTime[semester][day] |= mask;
    }
}

quint32 ScheduleManager::getBlockedTime(int semester, int day) const {
    if (semester >= 0 && semester < 8 && day >= 0 && day < 7) {
        return blockedTime[semester][day];
    }
    return 0;
}

void ScheduleManager::addCourse(const QString& courseId) {
    setPriority(courseId, 10);
}

void ScheduleManager::removeCourse(const QString& courseId) {
    setPriority(courseId, 0);
}

int ScheduleManager::getCreditSum(int semester) const {
    int sum = 0;
    for (const auto& sc : schedule) {
        if (sc.semester == semester) {
            for (const auto& course : allCourses) {
                if (course.id == sc.courseId) {
                    sum += course.credit;
                    break;
                }
            }
        }
    }
    return sum;
}

bool ScheduleManager::hasTimeConflict(int semester) const {
    for (const auto& sc : schedule) {
        if (sc.semester != semester) continue;
        const auto& off = getOffering(sc);
        for (int day = 0; day < 7; ++day) {
            if ((off.times[day] & blockedTime[semester][day]) != 0) {
                return true;
            }
        }
    }
    return false;
}

QList<QString> ScheduleManager::getPrerequisites(const QString& courseId) const {
    for (const auto& course : allCourses) {
        if (course.id == courseId) {
            return course.prerequisites.toVector().toList();
        }
    }
    return {};
}

bool ScheduleManager::isPrerequisiteSatisfied(const QString& courseId, int semester) const {
    for (const auto& course : allCourses) {
        if (course.id == courseId) {
            for (const auto& prereq : course.prerequisites) {
                bool found = false;
                for (const auto& sc : schedule) {
                    if (sc.courseId == prereq && sc.semester < semester) {
                        found = true;
                        break;
                    }
                }
                if (!found) return false;
            }
            return true;
        }
    }
    return false;
}
