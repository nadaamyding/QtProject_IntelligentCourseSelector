#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QList>

#include "jsonparser.h"
#include "schedule.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private slots:
    void showCourseDetails(QTreeWidgetItem* item, int col);   // 显示课程详情
    void generateSchedule();    // 生成选课方案
    void exportSchedule();      // 导出选课方案
    void addSelectedCourse();   // 添加选中的课程
    void removeSelectedCourse(); // 移除选中的课程
    void setCoursePreference();  // 设置课程优先级
    void setBlockedTime();       // 设置时间限制
    void setCreditLimits(int limit); // 设置学期学分上限
    void showScheduleConflicts(); // 显示课程冲突
    void showPrerequisites();     // 显示先修课程要求

private:
    void setupCourseTab();       // 设置课程浏览页
    void setupScheduleTab();     // 设置课表页
    void setupPreferenceTab();   // 设置偏好页
    void loadCourses();          // 加载课程数据
    void updateScheduleView();   // 更新课表视图
    void showStatusMessage(const QString& msg, bool isError = false);  // 显示状态信息

    QTabWidget*   mainTabs = nullptr;
    // 课程浏览
    QTreeWidget*  courseTree = nullptr;
    QTableWidget* courseDetail = nullptr;
    QPushButton*  addButton = nullptr;
    QPushButton*  removeButton = nullptr;
    QPushButton*  preferenceButton = nullptr;

    // 课表展示
    QTabWidget*   semesterTabs = nullptr;
    QPushButton*  genButton = nullptr;
    QPushButton*  expButton = nullptr;
    QPushButton*  conflictButton = nullptr;

    // 偏好设置面板
    QWidget*      preferenceTab = nullptr;
    QSpinBox*     creditSpinBox = nullptr;
    QComboBox*    semesterCombo = nullptr;
    QComboBox*    dayCombo = nullptr;
    QTableWidget* timeTable = nullptr;

    // 状态栏
    QLabel*       statusLabel = nullptr;

    // 数据
    QList<Course>       courses;
    JsonParser          parser;
    ScheduleManager*    schedMgr = nullptr;

    QSet<QString> manuallySelected;
};

#endif // MAINWINDOW_H
