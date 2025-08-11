#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <QString>
#include <QList>
#include <QJsonArray>
#include "course.h"
#include "schedule.h"  // ✅ 必须包含 ScheduledCourse 定义

class JsonParser {
public:
    // 从 JSON 文件中解析课程列表
    QList<Course> parseCourseJson(const QString& filePath);

    // 导出排课结果为 JSON 文件
    bool exportScheduleJson(const QList<ScheduledCourse>& schedule, const QString& filePath);

private:
    // 班次解析
    bool parseOfferings(const QJsonArray& offeringsArray, QVector<CourseOffering>& offerings);

    // 以下未启用扩展方法
    quint32 parseTimeSlot(const QString& timeStr) { return 0; }
    bool validateCourseJson(const QJsonObject& json) { return true; }
    bool validateScheduleJson(const QJsonArray& json) { return true; }
    void handleJsonError(const QString& errorMessage);
};

#endif
