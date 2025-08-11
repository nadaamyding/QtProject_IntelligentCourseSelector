#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <QString>
#include <QList>
#include <QMap>
#include <QSet>
#include <QVector>
#include "course.h"

// 单条排课结果
struct ScheduledCourse {
    QString courseId;
    QString classId;
    int semester;
};

class ScheduleManager {
public:
    ScheduleManager(const QList<Course>& courses);

    QList<QString> topologicalSort() const;
    bool generateSchedule();

    QList<ScheduledCourse> getCoursesForSemester(int sem) const;
    QList<ScheduledCourse> getAllScheduled() const;

    void setCreditLimit(int semester, int limit);
    void setTotalCreditLimit(int credit);  // ✅ 新增总学分限制接口
    void setPriority(const QString& courseId, int priority);
    int getPriority(const QString& courseId) const;

    void setSelectedCourses(const QSet<QString>& courseIds);  // ✅ 用于 UI 传入选中课程
    void addBlockedTime(int semester, int day, quint32 mask);
    quint32 getBlockedTime(int semester, int day) const;

    void addCourse(const QString& courseId);
    void removeCourse(const QString& courseId);

    int getCreditSum(int semester) const;
    bool hasTimeConflict(int semester) const;
    QList<QString> getPrerequisites(const QString& courseId) const;

private:
    bool checkTimeConflicts(const ScheduledCourse& newCourse) const;
    const CourseOffering& getOffering(const ScheduledCourse& sc) const;
    bool isPrerequisiteSatisfied(const QString& courseId, int semester) const;

    QList<Course> allCourses;
    QList<ScheduledCourse> schedule;
    QVector<int> creditLimits;
    QMap<QString, int> priorities;
    QVector<QVector<quint32>> blockedTime;
    QSet<QString> selectedCourses;
    int totalCreditLimit = 0;
};

#endif // SCHEDULE_H
