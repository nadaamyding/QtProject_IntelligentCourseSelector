#include "course.h"
#include <QStringList>
#include <QChar>
#include <QDebug>

// 将时间安排转换为可读字符串
QString CourseOffering::timeSlotsToString() const {
    static const QStringList dayNames = {
        "周一", "周二", "周三",
        "周四", "周五", "周六",
        "周日"
    };

    QStringList parts;
    for (int d = 0; d < 7; ++d) {
        quint32 mask = times[d];
        if (mask == 0) continue;

        QStringList timeBlocks;
        for (int i = 0; i < 13; ++i) {
            if (mask & (1u << i)) {
                // 每节课45分钟，从8:00开始
                int startM = 8 * 60 + i * 45;
                int endM = startM + 45;
                if (startM >= 24 * 60 || endM >= 24 * 60) {
                    qWarning() << "CourseOffering::timeSlotsToString: 时间超出范围" << i;
                    continue;
                }
                int sh = startM / 60, sm = startM % 60;
                int eh = endM / 60, em = endM % 60;
                QString timeStr = QString("%1:%2-%3:%4")
                                      .arg(sh, 2, 10, QChar('0'))
                                      .arg(sm, 2, 10, QChar('0'))
                                      .arg(eh, 2, 10, QChar('0'))
                                      .arg(em, 2, 10, QChar('0'));
                timeBlocks.append(timeStr);
            }
        }
        QString part = dayNames.at(d) + ": " + timeBlocks.join(", ");
        parts.append(part);
    }
    return parts.join("; ");
}

// 获取指定班次信息
const CourseOffering& Course::getOffering(const QString& offeringId) const {
    static CourseOffering emptyOffering;
    if (offeringId.isEmpty()) {
        qWarning() << "Course::getOffering: 班次ID为空 in course" << id;
        return emptyOffering;
    }
    if (offerings.isEmpty()) {
        qWarning() << "Course::getOffering: 课程没有班次信息" << "course id:" << id;
        return emptyOffering;
    }
    for (const CourseOffering& o : offerings) {
        if (o.id == offeringId)
            return o;
    }
    qWarning() << "Course::getOffering: 未找到班次" << offeringId << "in course" << id;
    return offerings.first();
}
