#include "jsonparser.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

QList<Course> JsonParser::parseCourseJson(const QString& filePath) {
    QList<Course> courses;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开文件：" << filePath;
        return courses;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "课程文件格式错误：" << filePath;
        return courses;
    }

    QJsonArray arr = doc.array();
    for (const auto& val : arr) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        Course c;
        c.id = obj["id"].toString();
        c.name = obj["name"].toString();
        c.credit = obj["credit"].toInt();
        c.required = obj["required"].toString();

        QJsonArray prereqArr = obj["prerequisites"].toArray();
        for (const auto& pr : prereqArr) {
            c.prerequisites.append(pr.toString());
        }

        QJsonArray offers = obj["offerings"].toArray();
        parseOfferings(offers, c.offerings);

        courses.append(c);
    }
    return courses;
}

bool JsonParser::exportScheduleJson(const QList<ScheduledCourse>& schedule, const QString& filePath) {
    QJsonArray arr;
    for (const auto& sc : schedule) {
        QJsonObject obj;
        obj["course_id"] = sc.courseId;
        obj["class_id"] = sc.classId;
        obj["semester"] = sc.semester;
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法写入文件：" << filePath;
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool JsonParser::parseOfferings(const QJsonArray& arr, QVector<CourseOffering>& offerings) {
    for (const auto& val : arr) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();
        CourseOffering off;
        off.id = obj["id"].toString();
        off.teacher = obj["teacher"].toString();
        QJsonArray timesArr = obj["times"].toArray();
        for (int i = 0; i < 7 && i < timesArr.size(); ++i) {
            off.times[i] = static_cast<quint32>(timesArr.at(i).toInt());
        }
        offerings.append(off);
    }
    return true;
}

void JsonParser::handleJsonError(const QString& errorMessage) {
    qWarning() << "JSON解析错误：" << errorMessage;
}
