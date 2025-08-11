#ifndef COURSE_H
#define COURSE_H

#include <QString>
#include <QStringList>
#include <QVector>

// 课程班次信息
struct CourseOffering {
    QString id;
    QString teacher;
    quint32 times[7];

    QString timeSlotsToString() const;
};

// 课程信息
class Course {
public:
    QString id;
    QString name;
    int credit;
    QString required;
    QStringList prerequisites;
    QVector<CourseOffering> offerings;

    const CourseOffering& getOffering(const QString& offeringId) const;
};

#endif // COURSE_H
