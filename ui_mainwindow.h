/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionImport;
    QAction *actionExport;
    QAction *actionExit;
    QAction *actionDesktopWidget;
    QWidget *centralwidget;
    QVBoxLayout *mainLayout;
    QTabWidget *tabWidget;
    QWidget *listTab;
    QHBoxLayout *listTabLayout;
    QWidget *leftPanel;
    QVBoxLayout *leftLayout;
    QLabel *folderLabel;
    QHBoxLayout *folderButtonLayout;
    QPushButton *newFolderBtn;
    QPushButton *deleteFolderBtn;
    QListWidget *folderListWidget;
    QWidget *middlePanel;
    QVBoxLayout *middleLayout;
    QLabel *todoLabel;
    QHBoxLayout *todoButtonLayout;
    QPushButton *newTodoBtn;
    QPushButton *syncBtn;
    QListWidget *todoListWidget;
    QWidget *rightPanel;
    QVBoxLayout *rightLayout;
    QLabel *detailLabel;
    QScrollArea *detailScrollArea;
    QWidget *detailContent;
    QVBoxLayout *detailContentLayout;
    QLabel *emptyStateLabel;
    QLabel *titleLabel;
    QLineEdit *titleEdit;
    QLabel *detailsLabel;
    QTextEdit *detailsEdit;
    QLabel *plannedDateLabel;
    QDateEdit *plannedDateEdit;
    QLabel *dueDateLabel;
    QDateEdit *dueDateEdit;
    QLabel *priorityLabel;
    QComboBox *priorityComboBox;
    QLabel *tagColorLabel;
    QComboBox *tagColorComboBox;
    QLabel *tagsLabel;
    QWidget *tagsWidget;
    QHBoxLayout *tagsLayout;
    QLabel *tagsDisplayLabel;
    QPushButton *addTagBtn;
    QSpacerItem *tagsSpacer;
    QLabel *timeLabel;
    QLabel *createdTimeLabel;
    QLabel *completedTimeLabel_title;
    QLabel *completedTimeLabel;
    QCheckBox *completedCheckBox;
    QHBoxLayout *buttonLayout;
    QPushButton *saveBtn;
    QPushButton *deleteBtn;
    QSpacerItem *verticalSpacer;
    QWidget *calendarTab;
    QVBoxLayout *calendarTabLayout;
    QWidget *calendarContainer;
    QWidget *tagTab;
    QVBoxLayout *tagTabLayout;
    QWidget *tagContainer;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuView;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1280, 720);
        MainWindow->setMinimumSize(QSize(900, 600));
        actionImport = new QAction(MainWindow);
        actionImport->setObjectName("actionImport");
        actionExport = new QAction(MainWindow);
        actionExport->setObjectName("actionExport");
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
        actionDesktopWidget = new QAction(MainWindow);
        actionDesktopWidget->setObjectName("actionDesktopWidget");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        mainLayout = new QVBoxLayout(centralwidget);
        mainLayout->setSpacing(0);
        mainLayout->setObjectName("mainLayout");
        mainLayout->setContentsMargins(0, 0, 0, 0);
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName("tabWidget");
        tabWidget->setDocumentMode(true);
        listTab = new QWidget();
        listTab->setObjectName("listTab");
        listTabLayout = new QHBoxLayout(listTab);
        listTabLayout->setSpacing(0);
        listTabLayout->setObjectName("listTabLayout");
        listTabLayout->setContentsMargins(0, 0, 0, 0);
        leftPanel = new QWidget(listTab);
        leftPanel->setObjectName("leftPanel");
        leftPanel->setMinimumSize(QSize(240, 0));
        leftPanel->setMaximumSize(QSize(280, 16777215));
        leftPanel->setStyleSheet(QString::fromUtf8("QWidget#leftPanel {\n"
"    background-color: #ffffff;\n"
"    border-right: 1px solid #e2e8f0;\n"
"}"));
        leftLayout = new QVBoxLayout(leftPanel);
        leftLayout->setSpacing(12);
        leftLayout->setObjectName("leftLayout");
        leftLayout->setContentsMargins(16, 16, 16, 16);
        folderLabel = new QLabel(leftPanel);
        folderLabel->setObjectName("folderLabel");
        folderLabel->setStyleSheet(QString::fromUtf8("font-weight: 600; font-size: 14px; color: #1e293b;"));

        leftLayout->addWidget(folderLabel);

        folderButtonLayout = new QHBoxLayout();
        folderButtonLayout->setSpacing(8);
        folderButtonLayout->setObjectName("folderButtonLayout");
        newFolderBtn = new QPushButton(leftPanel);
        newFolderBtn->setObjectName("newFolderBtn");

        folderButtonLayout->addWidget(newFolderBtn);

        deleteFolderBtn = new QPushButton(leftPanel);
        deleteFolderBtn->setObjectName("deleteFolderBtn");
        deleteFolderBtn->setEnabled(false);

        folderButtonLayout->addWidget(deleteFolderBtn);


        leftLayout->addLayout(folderButtonLayout);

        folderListWidget = new QListWidget(leftPanel);
        folderListWidget->setObjectName("folderListWidget");
        folderListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        leftLayout->addWidget(folderListWidget);


        listTabLayout->addWidget(leftPanel);

        middlePanel = new QWidget(listTab);
        middlePanel->setObjectName("middlePanel");
        middlePanel->setMinimumSize(QSize(360, 0));
        middlePanel->setStyleSheet(QString::fromUtf8("QWidget#middlePanel {\n"
"    background-color: #f8fafc;\n"
"    border-right: 1px solid #e2e8f0;\n"
"}"));
        middleLayout = new QVBoxLayout(middlePanel);
        middleLayout->setSpacing(12);
        middleLayout->setObjectName("middleLayout");
        middleLayout->setContentsMargins(16, 16, 16, 16);
        todoLabel = new QLabel(middlePanel);
        todoLabel->setObjectName("todoLabel");
        todoLabel->setStyleSheet(QString::fromUtf8("font-weight: 600; font-size: 14px; color: #1e293b;"));

        middleLayout->addWidget(todoLabel);

        todoButtonLayout = new QHBoxLayout();
        todoButtonLayout->setSpacing(8);
        todoButtonLayout->setObjectName("todoButtonLayout");
        newTodoBtn = new QPushButton(middlePanel);
        newTodoBtn->setObjectName("newTodoBtn");

        todoButtonLayout->addWidget(newTodoBtn);

        syncBtn = new QPushButton(middlePanel);
        syncBtn->setObjectName("syncBtn");

        todoButtonLayout->addWidget(syncBtn);


        middleLayout->addLayout(todoButtonLayout);

        todoListWidget = new QListWidget(middlePanel);
        todoListWidget->setObjectName("todoListWidget");
        todoListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

        middleLayout->addWidget(todoListWidget);


        listTabLayout->addWidget(middlePanel);

        rightPanel = new QWidget(listTab);
        rightPanel->setObjectName("rightPanel");
        rightPanel->setMinimumSize(QSize(320, 0));
        rightPanel->setStyleSheet(QString::fromUtf8("QWidget#rightPanel {\n"
"    background-color: #ffffff;\n"
"}"));
        rightLayout = new QVBoxLayout(rightPanel);
        rightLayout->setSpacing(16);
        rightLayout->setObjectName("rightLayout");
        rightLayout->setContentsMargins(20, 20, 20, 20);
        detailLabel = new QLabel(rightPanel);
        detailLabel->setObjectName("detailLabel");
        detailLabel->setStyleSheet(QString::fromUtf8("font-weight: 600; font-size: 14px; color: #1e293b;"));

        rightLayout->addWidget(detailLabel);

        detailScrollArea = new QScrollArea(rightPanel);
        detailScrollArea->setObjectName("detailScrollArea");
        detailScrollArea->setWidgetResizable(true);
        detailScrollArea->setStyleSheet(QString::fromUtf8("QScrollArea {\n"
"    border: none;\n"
"    background-color: transparent;\n"
"}"));
        detailContent = new QWidget();
        detailContent->setObjectName("detailContent");
        detailContent->setGeometry(QRect(0, 0, 280, 500));
        detailContentLayout = new QVBoxLayout(detailContent);
        detailContentLayout->setSpacing(16);
        detailContentLayout->setObjectName("detailContentLayout");
        detailContentLayout->setContentsMargins(0, 0, 0, 0);
        emptyStateLabel = new QLabel(detailContent);
        emptyStateLabel->setObjectName("emptyStateLabel");
        emptyStateLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        emptyStateLabel->setWordWrap(true);
        emptyStateLabel->setStyleSheet(QString::fromUtf8("color: #94a3b8; font-size: 13px; padding: 20px; background-color: #f8fafc; border-radius: 8px;"));

        detailContentLayout->addWidget(emptyStateLabel);

        titleLabel = new QLabel(detailContent);
        titleLabel->setObjectName("titleLabel");
        titleLabel->setStyleSheet(QString::fromUtf8("font-weight: 500; font-size: 12px; color: #64748b;"));

        detailContentLayout->addWidget(titleLabel);

        titleEdit = new QLineEdit(detailContent);
        titleEdit->setObjectName("titleEdit");

        detailContentLayout->addWidget(titleEdit);

        detailsLabel = new QLabel(detailContent);
        detailsLabel->setObjectName("detailsLabel");
        detailsLabel->setStyleSheet(QString::fromUtf8("font-weight: 500; font-size: 12px; color: #64748b;"));

        detailContentLayout->addWidget(detailsLabel);

        detailsEdit = new QTextEdit(detailContent);
        detailsEdit->setObjectName("detailsEdit");
        detailsEdit->setMinimumSize(QSize(0, 80));

        detailContentLayout->addWidget(detailsEdit);

        plannedDateLabel = new QLabel(detailContent);
        plannedDateLabel->setObjectName("plannedDateLabel");
        plannedDateLabel->setStyleSheet(QString::fromUtf8("font-weight: 500; font-size: 12px; color: #64748b;"));

        detailContentLayout->addWidget(plannedDateLabel);

        plannedDateEdit = new QDateEdit(detailContent);
        plannedDateEdit->setObjectName("plannedDateEdit");
        plannedDateEdit->setCalendarPopup(true);

        detailContentLayout->addWidget(plannedDateEdit);

        dueDateLabel = new QLabel(detailContent);
        dueDateLabel->setObjectName("dueDateLabel");
        dueDateLabel->setStyleSheet(QString::fromUtf8("font-weight: 500; font-size: 12px; color: #64748b;"));

        detailContentLayout->addWidget(dueDateLabel);

        dueDateEdit = new QDateEdit(detailContent);
        dueDateEdit->setObjectName("dueDateEdit");
        dueDateEdit->setCalendarPopup(true);

        detailContentLayout->addWidget(dueDateEdit);

        priorityLabel = new QLabel(detailContent);
        priorityLabel->setObjectName("priorityLabel");
        priorityLabel->setStyleSheet(QString::fromUtf8("font-weight: 500; font-size: 12px; color: #64748b;"));

        detailContentLayout->addWidget(priorityLabel);

        priorityComboBox = new QComboBox(detailContent);
        priorityComboBox->addItem(QString());
        priorityComboBox->addItem(QString());
        priorityComboBox->addItem(QString());
        priorityComboBox->setObjectName("priorityComboBox");

        detailContentLayout->addWidget(priorityComboBox);

        tagColorLabel = new QLabel(detailContent);
        tagColorLabel->setObjectName("tagColorLabel");
        tagColorLabel->setStyleSheet(QString::fromUtf8("font-weight: 500; font-size: 12px; color: #64748b;"));

        detailContentLayout->addWidget(tagColorLabel);

        tagColorComboBox = new QComboBox(detailContent);
        tagColorComboBox->addItem(QString());
        tagColorComboBox->addItem(QString());
        tagColorComboBox->addItem(QString());
        tagColorComboBox->addItem(QString());
        tagColorComboBox->addItem(QString());
        tagColorComboBox->addItem(QString());
        tagColorComboBox->setObjectName("tagColorComboBox");

        detailContentLayout->addWidget(tagColorComboBox);

        tagsLabel = new QLabel(detailContent);
        tagsLabel->setObjectName("tagsLabel");
        tagsLabel->setStyleSheet(QString::fromUtf8("font-weight: 500; font-size: 12px; color: #64748b;"));

        detailContentLayout->addWidget(tagsLabel);

        tagsWidget = new QWidget(detailContent);
        tagsWidget->setObjectName("tagsWidget");
        tagsWidget->setMinimumSize(QSize(0, 32));
        tagsLayout = new QHBoxLayout(tagsWidget);
        tagsLayout->setSpacing(4);
        tagsLayout->setObjectName("tagsLayout");
        tagsLayout->setContentsMargins(0, 0, 0, 0);
        tagsDisplayLabel = new QLabel(tagsWidget);
        tagsDisplayLabel->setObjectName("tagsDisplayLabel");
        tagsDisplayLabel->setStyleSheet(QString::fromUtf8("color: #94a3b8; font-size: 12px;"));

        tagsLayout->addWidget(tagsDisplayLabel);

        addTagBtn = new QPushButton(tagsWidget);
        addTagBtn->setObjectName("addTagBtn");
        addTagBtn->setMinimumSize(QSize(24, 24));
        addTagBtn->setMaximumSize(QSize(24, 24));

        tagsLayout->addWidget(addTagBtn);

        tagsSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        tagsLayout->addItem(tagsSpacer);


        detailContentLayout->addWidget(tagsWidget);

        timeLabel = new QLabel(detailContent);
        timeLabel->setObjectName("timeLabel");
        timeLabel->setStyleSheet(QString::fromUtf8("font-weight: 500; font-size: 12px; color: #64748b;"));

        detailContentLayout->addWidget(timeLabel);

        createdTimeLabel = new QLabel(detailContent);
        createdTimeLabel->setObjectName("createdTimeLabel");
        createdTimeLabel->setStyleSheet(QString::fromUtf8("color: #334155; font-size: 13px;"));

        detailContentLayout->addWidget(createdTimeLabel);

        completedTimeLabel_title = new QLabel(detailContent);
        completedTimeLabel_title->setObjectName("completedTimeLabel_title");
        completedTimeLabel_title->setStyleSheet(QString::fromUtf8("font-weight: 500; font-size: 12px; color: #64748b;"));

        detailContentLayout->addWidget(completedTimeLabel_title);

        completedTimeLabel = new QLabel(detailContent);
        completedTimeLabel->setObjectName("completedTimeLabel");
        completedTimeLabel->setStyleSheet(QString::fromUtf8("color: #334155; font-size: 13px;"));

        detailContentLayout->addWidget(completedTimeLabel);

        completedCheckBox = new QCheckBox(detailContent);
        completedCheckBox->setObjectName("completedCheckBox");

        detailContentLayout->addWidget(completedCheckBox);

        buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(12);
        buttonLayout->setObjectName("buttonLayout");
        saveBtn = new QPushButton(detailContent);
        saveBtn->setObjectName("saveBtn");

        buttonLayout->addWidget(saveBtn);

        deleteBtn = new QPushButton(detailContent);
        deleteBtn->setObjectName("deleteBtn");

        buttonLayout->addWidget(deleteBtn);


        detailContentLayout->addLayout(buttonLayout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        detailContentLayout->addItem(verticalSpacer);

        detailScrollArea->setWidget(detailContent);

        rightLayout->addWidget(detailScrollArea);


        listTabLayout->addWidget(rightPanel);

        tabWidget->addTab(listTab, QString());
        calendarTab = new QWidget();
        calendarTab->setObjectName("calendarTab");
        calendarTabLayout = new QVBoxLayout(calendarTab);
        calendarTabLayout->setSpacing(0);
        calendarTabLayout->setObjectName("calendarTabLayout");
        calendarTabLayout->setContentsMargins(0, 0, 0, 0);
        calendarContainer = new QWidget(calendarTab);
        calendarContainer->setObjectName("calendarContainer");
        calendarContainer->setStyleSheet(QString::fromUtf8("QWidget#calendarContainer {\n"
"    background-color: #f8fafc;\n"
"}"));

        calendarTabLayout->addWidget(calendarContainer);

        tabWidget->addTab(calendarTab, QString());
        tagTab = new QWidget();
        tagTab->setObjectName("tagTab");
        tagTabLayout = new QVBoxLayout(tagTab);
        tagTabLayout->setSpacing(0);
        tagTabLayout->setObjectName("tagTabLayout");
        tagTabLayout->setContentsMargins(0, 0, 0, 0);
        tagContainer = new QWidget(tagTab);
        tagContainer->setObjectName("tagContainer");
        tagContainer->setStyleSheet(QString::fromUtf8("QWidget#tagContainer {\n"
"    background-color: #f8fafc;\n"
"}"));

        tagTabLayout->addWidget(tagContainer);

        tabWidget->addTab(tagTab, QString());

        mainLayout->addWidget(tabWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1280, 25));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName("menuFile");
        menuView = new QMenu(menubar);
        menuView->setObjectName("menuView");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuView->menuAction());
        menuFile->addAction(actionImport);
        menuFile->addAction(actionExport);
        menuFile->addSeparator();
        menuFile->addAction(actionExit);
        menuView->addAction(actionDesktopWidget);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Todo List - \345\276\205\345\212\236\344\272\213\351\241\271\347\256\241\347\220\206", nullptr));
        actionImport->setText(QCoreApplication::translate("MainWindow", "\345\257\274\345\205\245", nullptr));
        actionExport->setText(QCoreApplication::translate("MainWindow", "\345\257\274\345\207\272", nullptr));
        actionExit->setText(QCoreApplication::translate("MainWindow", "\351\200\200\345\207\272", nullptr));
        actionDesktopWidget->setText(QCoreApplication::translate("MainWindow", "\346\241\214\351\235\242\345\260\217\350\264\264\345\243\253", nullptr));
        folderLabel->setText(QCoreApplication::translate("MainWindow", "\346\226\207\344\273\266\345\244\271", nullptr));
        newFolderBtn->setText(QCoreApplication::translate("MainWindow", "\346\226\260\345\273\272", nullptr));
#if QT_CONFIG(tooltip)
        newFolderBtn->setToolTip(QCoreApplication::translate("MainWindow", "\345\210\233\345\273\272\346\226\260\346\226\207\344\273\266\345\244\271", nullptr));
#endif // QT_CONFIG(tooltip)
        deleteFolderBtn->setText(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244", nullptr));
#if QT_CONFIG(tooltip)
        deleteFolderBtn->setToolTip(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244\351\200\211\344\270\255\347\232\204\346\226\207\344\273\266\345\244\271", nullptr));
#endif // QT_CONFIG(tooltip)
        todoLabel->setText(QCoreApplication::translate("MainWindow", "\345\276\205\345\212\236\344\272\213\351\241\271", nullptr));
        newTodoBtn->setText(QCoreApplication::translate("MainWindow", "\346\226\260\345\273\272", nullptr));
#if QT_CONFIG(tooltip)
        newTodoBtn->setToolTip(QCoreApplication::translate("MainWindow", "\345\210\233\345\273\272\346\226\260\345\276\205\345\212\236\344\272\213\351\241\271", nullptr));
#endif // QT_CONFIG(tooltip)
        syncBtn->setText(QCoreApplication::translate("MainWindow", "\345\220\214\346\255\245", nullptr));
#if QT_CONFIG(tooltip)
        syncBtn->setToolTip(QCoreApplication::translate("MainWindow", "\345\220\214\346\255\245\346\225\260\346\215\256", nullptr));
#endif // QT_CONFIG(tooltip)
        detailLabel->setText(QCoreApplication::translate("MainWindow", "\350\257\246\346\203\205", nullptr));
        emptyStateLabel->setText(QCoreApplication::translate("MainWindow", "\350\257\267\351\200\211\346\213\251\344\270\200\344\270\252\345\276\205\345\212\236\344\272\213\351\241\271\346\237\245\347\234\213\350\257\246\346\203\205\n"
"\n"
"\360\237\222\241 \346\217\220\347\244\272\357\274\232\n"
"\342\200\242 \347\202\271\345\207\273\345\267\246\344\276\247\345\276\205\345\212\236\344\272\213\351\241\271\346\237\245\347\234\213\350\257\246\346\203\205\n"
"\342\200\242 \347\202\271\345\207\273\"\346\226\260\345\273\272\"\345\210\233\345\273\272\346\226\260\347\232\204\345\276\205\345\212\236\344\272\213\351\241\271\n"
"\342\200\242 \345\217\214\345\207\273\345\276\205\345\212\236\344\272\213\351\241\271\345\217\257\345\277\253\351\200\237\347\274\226\350\276\221", nullptr));
        titleLabel->setText(QCoreApplication::translate("MainWindow", "\346\240\207\351\242\230", nullptr));
        titleEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "\350\257\267\350\276\223\345\205\245\346\240\207\351\242\230", nullptr));
        detailsLabel->setText(QCoreApplication::translate("MainWindow", "\350\257\246\346\203\205", nullptr));
        detailsEdit->setPlaceholderText(QCoreApplication::translate("MainWindow", "\350\257\267\350\276\223\345\205\245\350\257\246\347\273\206\346\217\217\350\277\260", nullptr));
        plannedDateLabel->setText(QCoreApplication::translate("MainWindow", "\350\256\241\345\210\222\346\227\245\346\234\237", nullptr));
        plannedDateEdit->setDisplayFormat(QCoreApplication::translate("MainWindow", "yyyy-MM-dd", nullptr));
        plannedDateEdit->setSpecialValueText(QCoreApplication::translate("MainWindow", "\346\234\252\350\256\276\347\275\256", nullptr));
        dueDateLabel->setText(QCoreApplication::translate("MainWindow", "\346\210\252\346\255\242\346\227\245\346\234\237", nullptr));
        dueDateEdit->setDisplayFormat(QCoreApplication::translate("MainWindow", "yyyy-MM-dd", nullptr));
        dueDateEdit->setSpecialValueText(QCoreApplication::translate("MainWindow", "\346\234\252\350\256\276\347\275\256", nullptr));
        priorityLabel->setText(QCoreApplication::translate("MainWindow", "\344\274\230\345\205\210\347\272\247", nullptr));
        priorityComboBox->setItemText(0, QCoreApplication::translate("MainWindow", "\344\275\216\344\274\230\345\205\210\347\272\247", nullptr));
        priorityComboBox->setItemText(1, QCoreApplication::translate("MainWindow", "\344\270\255\344\274\230\345\205\210\347\272\247", nullptr));
        priorityComboBox->setItemText(2, QCoreApplication::translate("MainWindow", "\351\253\230\344\274\230\345\205\210\347\272\247", nullptr));

        tagColorLabel->setText(QCoreApplication::translate("MainWindow", "\346\240\207\347\255\276\351\242\234\350\211\262", nullptr));
        tagColorComboBox->setItemText(0, QCoreApplication::translate("MainWindow", "\350\223\235\350\211\262", nullptr));
        tagColorComboBox->setItemText(1, QCoreApplication::translate("MainWindow", "\347\273\277\350\211\262", nullptr));
        tagColorComboBox->setItemText(2, QCoreApplication::translate("MainWindow", "\347\272\242\350\211\262", nullptr));
        tagColorComboBox->setItemText(3, QCoreApplication::translate("MainWindow", "\346\251\231\350\211\262", nullptr));
        tagColorComboBox->setItemText(4, QCoreApplication::translate("MainWindow", "\347\264\253\350\211\262", nullptr));
        tagColorComboBox->setItemText(5, QCoreApplication::translate("MainWindow", "\351\235\222\350\211\262", nullptr));

        tagsLabel->setText(QCoreApplication::translate("MainWindow", "\346\240\207\347\255\276", nullptr));
        tagsDisplayLabel->setText(QCoreApplication::translate("MainWindow", "\346\227\240\346\240\207\347\255\276", nullptr));
        addTagBtn->setText(QCoreApplication::translate("MainWindow", "+", nullptr));
#if QT_CONFIG(tooltip)
        addTagBtn->setToolTip(QCoreApplication::translate("MainWindow", "\346\267\273\345\212\240\346\240\207\347\255\276", nullptr));
#endif // QT_CONFIG(tooltip)
        timeLabel->setText(QCoreApplication::translate("MainWindow", "\345\210\233\345\273\272\346\227\266\351\227\264", nullptr));
        createdTimeLabel->setText(QCoreApplication::translate("MainWindow", "-", nullptr));
        completedTimeLabel_title->setText(QCoreApplication::translate("MainWindow", "\345\256\214\346\210\220\346\227\266\351\227\264", nullptr));
        completedTimeLabel->setText(QCoreApplication::translate("MainWindow", "-", nullptr));
        completedCheckBox->setText(QCoreApplication::translate("MainWindow", "\346\240\207\350\256\260\344\270\272\345\267\262\345\256\214\346\210\220", nullptr));
        saveBtn->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230", nullptr));
#if QT_CONFIG(tooltip)
        saveBtn->setToolTip(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\345\275\223\345\211\215\344\277\256\346\224\271", nullptr));
#endif // QT_CONFIG(tooltip)
        deleteBtn->setText(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244", nullptr));
#if QT_CONFIG(tooltip)
        deleteBtn->setToolTip(QCoreApplication::translate("MainWindow", "\345\210\240\351\231\244\345\275\223\345\211\215\345\276\205\345\212\236\344\272\213\351\241\271", nullptr));
#endif // QT_CONFIG(tooltip)
        tabWidget->setTabText(tabWidget->indexOf(listTab), QCoreApplication::translate("MainWindow", "\345\210\227\350\241\250\350\247\206\345\233\276", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(calendarTab), QCoreApplication::translate("MainWindow", "\346\227\245\345\216\206\350\247\206\345\233\276", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tagTab), QCoreApplication::translate("MainWindow", "\346\240\207\347\255\276\350\247\206\345\233\276", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "\346\226\207\344\273\266", nullptr));
        menuView->setTitle(QCoreApplication::translate("MainWindow", "\350\247\206\345\233\276", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
