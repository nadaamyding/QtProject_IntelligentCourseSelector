#include "mainwindow.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QStatusBar>
#include <QLabel>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QCoreApplication>
#include <QBrush>
#include <QColor>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    mainTabs = new QTabWidget(this);
    setCentralWidget(mainTabs);

    // 状态栏
    statusLabel = new QLabel(this);
    statusBar()->addWidget(statusLabel);

    // 初始化界面
    setupCourseTab();
    setupScheduleTab();
    setupPreferenceTab();
    loadCourses();

    // 信号与槽连接
    connect(genButton, &QPushButton::clicked, this, &MainWindow::generateSchedule);
    connect(expButton, &QPushButton::clicked, this, &MainWindow::exportSchedule);
    connect(courseTree, &QTreeWidget::itemClicked, this, &MainWindow::showCourseDetails);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addSelectedCourse);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeSelectedCourse);
    connect(preferenceButton, &QPushButton::clicked, this, &MainWindow::setCoursePreference);
    connect(conflictButton, &QPushButton::clicked, this, &MainWindow::showScheduleConflicts);
}
void MainWindow::addSelectedCourse() {
    auto items = courseTree->selectedItems();
    if (items.isEmpty()) {
        showStatusMessage("请先选择一门课程", true);
        return;
    }

    QString courseId = items[0]->data(0, Qt::UserRole).toString();
    if (courseId.isEmpty()) {
        showStatusMessage("无效的课程选择", true);
        return;
    }

    manuallySelected.insert(courseId);
    showStatusMessage(QString("已添加课程：%1").arg(items[0]->text(0)));

    // 更新 ScheduleManager 的选中课程 + 学分限制
    schedMgr->setSelectedCourses(manuallySelected);
    schedMgr->setTotalCreditLimit(creditSpinBox->value() * 2);
    schedMgr->generateSchedule();
    updateScheduleView();
}

void MainWindow::generateSchedule() {
    showStatusMessage("正在生成课表...");

    // 从界面读取设置：选中课程集合和学分下限
    schedMgr->setSelectedCourses(manuallySelected);
    schedMgr->setTotalCreditLimit(creditSpinBox->value() * 2);

    if (!schedMgr->generateSchedule()) {
        showStatusMessage("排课失败，存在冲突或先修限制", true);
        return;
    }
    updateScheduleView();
    showStatusMessage("排课成功");
}

void MainWindow::updateScheduleView() {
    for (int sem = 0; sem < 8; ++sem) {
        auto page = semesterTabs->widget(sem);
        auto tbl = page->findChild<QTableWidget*>();
        tbl->clearContents();
        auto list = schedMgr->getCoursesForSemester(sem);
        for (auto& sc : list) {
            const Course* pc = nullptr;
            for (auto& c : courses) {
                if (c.id == sc.courseId) {
                    pc = &c;
                    break;
                }
            }
            if (!pc) continue;
            const auto& off = pc->getOffering(sc.classId);
            for (int d = 0; d < 7; ++d) {
                if (off.times[d] == 0) continue;
                for (int s = 0; s < 13; ++s) {
                    if (off.times[d] & (1u << s)) {
                        QTableWidgetItem* item = new QTableWidgetItem(pc->name);
                        item->setBackground(pc->required == "Compulsory" ?
                                                QBrush(QColor(173, 216, 230)) : QBrush(QColor(255, 255, 224)));
                        QString tooltip = QString("%1\n教师:%2\n学分:%3\n时间:%4")
                                              .arg(pc->name).arg(off.teacher).arg(pc->credit).arg(off.timeSlotsToString());
                        item->setToolTip(tooltip);
                        tbl->setItem(s, d, item);
                    }
                }
            }
        }
        int creditSum = schedMgr->getCreditSum(sem);
        semesterTabs->setTabText(sem, QString("学期%1 (%2 学分)").arg(sem + 1).arg(creditSum));
    }
}

void MainWindow::setCreditLimits(int) {
    schedMgr->setTotalCreditLimit(creditSpinBox->value() * 2);
}

void MainWindow::removeSelectedCourse() {
    auto items = courseTree->selectedItems();
    if (items.isEmpty()) return;
    QString courseId = items[0]->data(0, Qt::UserRole).toString();
    manuallySelected.remove(courseId);
    schedMgr->setSelectedCourses(manuallySelected);
    schedMgr->generateSchedule();
    updateScheduleView();
}

void MainWindow::showCourseDetails(QTreeWidgetItem* it, int) {
    courseDetail->setRowCount(0);
    QString cid = it->data(0, Qt::UserRole).toString();
    const Course* pc = nullptr;
    for (auto& c : courses) {
        if (c.id == cid) { pc = &c; break; }
    }
    if (!pc) return;

    int row = 0;
    auto add = [&](const QString& k, const QString& v) {
        courseDetail->insertRow(row);
        courseDetail->setItem(row, 0, new QTableWidgetItem(k));
        courseDetail->setItem(row, 1, new QTableWidgetItem(v));
        ++row;
    };
    add("ID", pc->id);
    add("名称", pc->name);
    add("学分", QString::number(pc->credit));
    add("类型", pc->required == "Compulsory" ? "必修" : "选修");
    add("优先级", QString::number(schedMgr->getPriority(pc->id)));
    add("先修课程", pc->prerequisites.join("，"));
    for (auto& o : pc->offerings) {
        add("班次", o.id);
        add("教师", o.teacher);
        add("时间", o.timeSlotsToString());
    }
}

void MainWindow::setCoursePreference() {
    auto items = courseTree->selectedItems();
    if (items.isEmpty()) return;
    QString courseId = items[0]->data(0, Qt::UserRole).toString();
    bool ok;
    int priority = QInputDialog::getInt(this, "设置优先级",
                                        "请输入课程优先级(1-10)：", 5, 1, 10, 1, &ok);
    if (ok) {
        schedMgr->setPriority(courseId, priority);
        showStatusMessage(QString("已设置课程 %1 的优先级为 %2").arg(courseId).arg(priority));
    }
}

void MainWindow::setBlockedTime() {
    int semester = semesterCombo->currentIndex();
    int day = dayCombo->currentIndex();
    quint32 mask = 0;
    for (int i = 0; i < 13; ++i) {
        if (timeTable->item(i, 0)->checkState() == Qt::Checked) {
            mask |= (1u << i);
        }
    }
    if (mask == 0) {
        showStatusMessage("请至少选择一个时间段", true);
        return;
    }
    schedMgr->addBlockedTime(semester, day, mask);
    showStatusMessage(QString("已设置第%1学期 %2 的屏蔽时间").arg(semester + 1).arg(dayCombo->currentText()));
}

void MainWindow::showScheduleConflicts() {
    QString conflicts;
    for (int sem = 0; sem < 8; ++sem) {
        if (schedMgr->hasTimeConflict(sem)) {
            conflicts += QString("第 %1 学期 存在时间冲突\n").arg(sem + 1);
        }
    }
    if (conflicts.isEmpty()) {
        showStatusMessage("未发现冲突");
    } else {
        QMessageBox::warning(this, "冲突", conflicts);
        showStatusMessage("存在冲突", true);
    }
}

void MainWindow::showPrerequisites() {
    auto items = courseTree->selectedItems();
    if (items.isEmpty()) return;
    QString cid = items[0]->data(0, Qt::UserRole).toString();
    auto pre = schedMgr->getPrerequisites(cid);
    if (pre.isEmpty()) {
        QMessageBox::information(this, "无先修课程", "该课程无先修要求");
    } else {
        QMessageBox::information(this, "先修课程", pre.join("\n"));
    }
}

void MainWindow::exportSchedule() {
    QString path = QFileDialog::getSaveFileName(
        this,
        "导出课表",
        QCoreApplication::applicationDirPath() + "/schedule.json",
        "JSON 文件 (*.json);;所有文件 (*.*)");

    if (path.isEmpty()) return;
    if (!path.endsWith(".json", Qt::CaseInsensitive)) {
        path += ".json";
    }

    auto full = schedMgr->getAllScheduled();
    if (parser.exportScheduleJson(full, path)) {
        showStatusMessage(QString("导出成功：%1").arg(path));
    } else {
        showStatusMessage(QString("导出失败：%1").arg(path), true);
    }
}

void MainWindow::showStatusMessage(const QString& message, bool isError) {
    statusLabel->setText(message);
    statusLabel->setStyleSheet(isError ? "color: red;" : "color: black;");
}
void MainWindow::setupCourseTab() {
    QWidget* tab = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(tab);

    QSplitter* spl = new QSplitter(Qt::Horizontal, tab);
    courseTree = new QTreeWidget(spl);
    courseTree->setHeaderLabel("课程分类");
    courseTree->setMinimumWidth(200);

    courseDetail = new QTableWidget(spl);
    courseDetail->setColumnCount(2);
    courseDetail->setHorizontalHeaderLabels({"属性","值"});
    courseDetail->horizontalHeader()->setStretchLastSection(true);

    mainLayout->addWidget(spl);

    QHBoxLayout* btnLayout = new QHBoxLayout;
    addButton = new QPushButton("添加到选课方案", tab);
    removeButton = new QPushButton("移除课程", tab);
    preferenceButton = new QPushButton("设置优先级", tab);
    btnLayout->addWidget(addButton);
    btnLayout->addWidget(removeButton);
    btnLayout->addWidget(preferenceButton);
    mainLayout->addLayout(btnLayout);

    tab->setLayout(mainLayout);
    mainTabs->addTab(tab, "课程浏览");
}

void MainWindow::setupScheduleTab() {
    QWidget* tab = new QWidget;
    QVBoxLayout* vlay = new QVBoxLayout(tab);

    QHBoxLayout* topLayout = new QHBoxLayout;
    QLabel* creditLabel = new QLabel("总学分下限（×2）:", tab);
    creditSpinBox = new QSpinBox(tab);
    creditSpinBox->setRange(0, 500);  // 学分下限控制
    creditSpinBox->setValue(0);
    creditSpinBox->setSuffix(" 分");

    connect(creditSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::setCreditLimits);

    topLayout->addWidget(creditLabel);
    topLayout->addWidget(creditSpinBox);
    topLayout->addStretch();
    vlay->addLayout(topLayout);

    semesterTabs = new QTabWidget(tab);
    for (int sem = 0; sem < 8; ++sem) {
        QWidget* page = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(page);
        QLabel* title = new QLabel(QString("第%1学期").arg(sem + 1), page);
        QTableWidget* tbl = new QTableWidget(13, 7, page);
        tbl->setHorizontalHeaderLabels({"周一","周二","周三","周四","周五","周六","周日"});
        QStringList vhead;
        for (int i = 0; i < 13; ++i) {
            int m = 8 * 60 + i * 45;
            vhead << QString("%1:%2").arg(m / 60).arg(m % 60, 2, 10, QChar('0'));
        }
        tbl->setVerticalHeaderLabels(vhead);
        tbl->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tbl->setSelectionMode(QAbstractItemView::SingleSelection);
        layout->addWidget(title);
        layout->addWidget(tbl);
        page->setLayout(layout);
        semesterTabs->addTab(page, QString("学期%1").arg(sem + 1));
    }
    vlay->addWidget(semesterTabs);

    QHBoxLayout* btns = new QHBoxLayout;
    genButton = new QPushButton("生成选课方案", tab);
    expButton = new QPushButton("导出方案", tab);
    conflictButton = new QPushButton("检查冲突", tab);
    btns->addWidget(genButton);
    btns->addWidget(expButton);
    btns->addWidget(conflictButton);
    vlay->addLayout(btns);

    tab->setLayout(vlay);
    mainTabs->addTab(tab, "课表展示");
}

void MainWindow::setupPreferenceTab() {
    QWidget* tab = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGroupBox* group = new QGroupBox("时间限制设置", tab);
    QVBoxLayout* vbox = new QVBoxLayout(group);

    QHBoxLayout* semRow = new QHBoxLayout;
    QLabel* semLabel = new QLabel("学期：", group);
    semesterCombo = new QComboBox(group);
    for (int i = 1; i <= 8; ++i) {
        semesterCombo->addItem(QString("第%1学期").arg(i));
    }
    semRow->addWidget(semLabel);
    semRow->addWidget(semesterCombo);
    vbox->addLayout(semRow);

    QHBoxLayout* dayRow = new QHBoxLayout;
    QLabel* dayLabel = new QLabel("星期：", group);
    dayCombo = new QComboBox(group);
    dayCombo->addItems({"周一","周二","周三","周四","周五","周六","周日"});
    dayRow->addWidget(dayLabel);
    dayRow->addWidget(dayCombo);
    vbox->addLayout(dayRow);

    timeTable = new QTableWidget(13, 1, group);
    QStringList times;
    for (int i = 0; i < 13; ++i) {
        int m = 8 * 60 + i * 45;
        times << QString("%1:%2").arg(m / 60).arg(m % 60, 2, 10, QChar('0'));
    }
    timeTable->setVerticalHeaderLabels(times);
    timeTable->setHorizontalHeaderLabels({"选择"});
    timeTable->horizontalHeader()->setStretchLastSection(true);
    for (int i = 0; i < 13; ++i) {
        QTableWidgetItem* item = new QTableWidgetItem;
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Unchecked);
        timeTable->setItem(i, 0, item);
    }
    vbox->addWidget(timeTable);

    QPushButton* setBtn = new QPushButton("设为不可用时间", group);
    connect(setBtn, &QPushButton::clicked, this, &MainWindow::setBlockedTime);
    vbox->addWidget(setBtn);

    layout->addWidget(group);
    tab->setLayout(layout);
    mainTabs->addTab(tab, "偏好设置");
}

void MainWindow::loadCourses() {
    QString path = QCoreApplication::applicationDirPath() + "/data/course.json";
    courses = parser.parseCourseJson(path);
    schedMgr = new ScheduleManager(courses);

    QTreeWidgetItem* comp = new QTreeWidgetItem(courseTree);
    comp->setText(0, "必修课程");
    QTreeWidgetItem* elec = new QTreeWidgetItem(courseTree);
    elec->setText(0, "选修课程");

    for (auto& c : courses) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, c.name);
        item->setData(0, Qt::UserRole, c.id);
        item->setToolTip(0, QString("学分：%1\n类型：%2").arg(c.credit).arg(c.required));
        if (c.required == "Compulsory")
            comp->addChild(item);
        else
            elec->addChild(item);
    }
    courseTree->expandAll();
}
