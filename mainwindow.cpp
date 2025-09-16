#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QStandardPaths>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_currentFolder(nullptr)
    , m_currentItem(nullptr)
    , m_desktopWidget(nullptr)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
    , m_showAction(nullptr)
    , m_exitAction(nullptr)
{
    ui->setupUi(this);
    
    // 设置窗口图标
    QIcon windowIcon(":/icons/app.ico");
    if (windowIcon.isNull()) {
        // 如果资源图标加载失败，尝试文件路径
        QString iconPath = QDir::currentPath() + "/icons/app.ico";
        windowIcon = QIcon(iconPath);
        if (windowIcon.isNull()) {
            windowIcon = QIcon("icons/app.ico");
        }
    }
    setWindowIcon(windowIcon);
    
    initializeData();
    loadData();
    updateFolderList();
    setupConnections();
    setupDesktopWidget();
    setupCalendarWidget();
    updateCalendarWidget();
    setupSystemTray();
}

MainWindow::~MainWindow()
{
    saveData();
    if (m_desktopWidget) {
        m_desktopWidget->close();
        delete m_desktopWidget;
    }
    delete ui;
}

void MainWindow::setupConnections()
{
    // 文件夹相关连接
    connect(ui->newFolderBtn, &QPushButton::clicked, this, &MainWindow::onNewFolderClicked);
    connect(ui->deleteFolderBtn, &QPushButton::clicked, this, &MainWindow::onDeleteFolderClicked);
    connect(ui->folderListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onFolderSelectionChanged);
    
    // 待办事项相关连接
    connect(ui->newTodoBtn, &QPushButton::clicked, this, &MainWindow::onNewTodoClicked);
    connect(ui->todoListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onTodoSelectionChanged);
    connect(ui->saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(ui->deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    connect(ui->completedCheckBox, &QCheckBox::toggled, this, &MainWindow::onCompletedToggled);
    connect(ui->syncBtn, &QPushButton::clicked, this, &MainWindow::onSyncClicked);
    
    // 菜单相关连接
    connect(ui->actionImport, &QAction::triggered, this, &MainWindow::onImportClicked);
    connect(ui->actionExport, &QAction::triggered, this, &MainWindow::onExportClicked);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onExitClicked);
    connect(ui->actionDesktopWidget, &QAction::triggered, this, &MainWindow::onDesktopWidgetClicked);
}

void MainWindow::initializeData()
{
    // 创建默认文件夹
    if (m_folders.isEmpty()) {
        TodoFolder defaultFolder("默认文件夹");
        m_folders.append(defaultFolder);
        
        // 添加示例待办事项
        TodoItem sampleItem("欢迎使用Todo List", "这是一个示例待办事项，您可以编辑或删除它。");
        m_folders[0].addItem(sampleItem);
    }
}

void MainWindow::onNewFolderClicked()
{
    bool ok;
    QString folderName = QInputDialog::getText(this, "新建文件夹", "请输入文件夹名称:", QLineEdit::Normal, "新建文件夹", &ok);
    
    if (ok && !folderName.isEmpty()) {
        TodoFolder newFolder(folderName);
        m_folders.append(newFolder);
        updateFolderList();
        updateCalendarWidget();
        saveData();
        
        // 选中新创建的文件夹
        ui->folderListWidget->setCurrentRow(m_folders.size() - 1);
    }
}

void MainWindow::onFolderSelectionChanged()
{
    int currentRow = ui->folderListWidget->currentRow();
    if (currentRow >= 0 && currentRow < m_folders.size()) {
        m_currentFolder = &m_folders[currentRow];
        m_currentItem = nullptr;
        updateTodoList();
        clearDetailPanel();
        
        // 启用删除按钮
        ui->deleteFolderBtn->setEnabled(true);
    } else {
        // 禁用删除按钮
        ui->deleteFolderBtn->setEnabled(false);
    }
}

void MainWindow::onDeleteFolderClicked()
{
    int currentRow = ui->folderListWidget->currentRow();
    if (currentRow >= 0 && currentRow < m_folders.size()) {
        QString folderName = m_folders[currentRow].getName();
        
        // 确认删除
        int ret = QMessageBox::question(this, "删除文件夹", 
                                      QString("确定要删除文件夹 '%1' 吗？\n此操作将删除文件夹内的所有待办事项。").arg(folderName),
                                      QMessageBox::Yes | QMessageBox::No);
        
        if (ret == QMessageBox::Yes) {
            // 删除文件夹
            m_folders.removeAt(currentRow);
            
            // 重置当前选择
            m_currentFolder = nullptr;
            m_currentItem = nullptr;
            
            // 更新界面
            updateFolderList();
            updateTodoList();
            clearDetailPanel();
            updateDesktopWidget();
            updateCalendarWidget();
            
            // 保存数据
            saveData();
            
            // 如果没有文件夹了，禁用删除按钮
            if (m_folders.isEmpty()) {
                ui->deleteFolderBtn->setEnabled(false);
            }
        }
    }
}

void MainWindow::onNewTodoClicked()
{
    if (!m_currentFolder) {
        QMessageBox::warning(this, "警告", "请先选择一个文件夹！");
        return;
    }
    
    bool ok;
    QString title = QInputDialog::getText(this, "新建待办事项", "请输入标题:", QLineEdit::Normal, "新建待办事项", &ok);
    
    if (ok && !title.isEmpty()) {
        TodoItem newItem(title);
        newItem.setPlannedDate(QDate::currentDate()); // 设置计划日期为今日
        m_currentFolder->addItem(newItem);
        updateTodoList();
        updateFolderList();
        updateCalendarWidget();
        updateDesktopWidget();
        saveData();
        
        // 选中新创建的待办事项
        ui->todoListWidget->setCurrentRow(m_currentFolder->getItemCount() - 1);
    }
}

void MainWindow::onTodoSelectionChanged()
{
    if (!m_currentFolder) return;
    
    int currentRow = ui->todoListWidget->currentRow();
    QList<TodoItem> items = m_currentFolder->getItems();
    
    if (currentRow >= 0 && currentRow < items.size()) {
        // 直接使用items中的引用，避免索引不匹配问题
        QString itemId = items[currentRow].getId();
        m_currentItem = m_currentFolder->findItem(itemId);
        updateDetailPanel();
    } else {
        m_currentItem = nullptr;
        clearDetailPanel();
    }
}

void MainWindow::onSaveClicked()
{
    if (!m_currentItem || !m_currentFolder) return;
    
    // 临时断开信号连接，防止递归调用
    ui->todoListWidget->blockSignals(true);
    
    QString currentItemId = m_currentItem->getId();
    m_currentItem->setTitle(ui->titleEdit->text());
    m_currentItem->setDetails(ui->detailsEdit->toPlainText());
    m_currentItem->setCompleted(ui->completedCheckBox->isChecked());
    
    // 更新列表但保持选中状态
    updateTodoList();
    
    // 恢复选中状态
    QList<TodoItem> items = m_currentFolder->getItems();
    for (int i = 0; i < items.size(); ++i) {
        if (items[i].getId() == currentItemId) {
            ui->todoListWidget->setCurrentRow(i);
            // 重新获取当前项目指针
            m_currentItem = m_currentFolder->findItem(currentItemId);
            break;
        }
    }
    
    // 恢复信号连接
    ui->todoListWidget->blockSignals(false);
    
    updateFolderList();
    updateCalendarWidget();
    updateDesktopWidget();
    saveData();
    
    ui->statusbar->showMessage("保存成功", 2000);
}

void MainWindow::onDeleteClicked()
{
    if (!m_currentItem || !m_currentFolder) return;
    
    int ret = QMessageBox::question(this, "确认删除", "确定要删除这个待办事项吗？", 
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_currentFolder->removeItem(m_currentItem->getId());
        m_currentItem = nullptr;
        updateTodoList();
        clearDetailPanel();
        updateFolderList();
        updateCalendarWidget();
        updateDesktopWidget();
        saveData();
        
        ui->statusbar->showMessage("删除成功", 2000);
    }
}

void MainWindow::onCompletedToggled(bool completed)
{
    if (!m_currentItem || !m_currentFolder) return;
    
    // 临时断开信号连接，防止递归调用
    ui->todoListWidget->blockSignals(true);
    
    QString currentItemId = m_currentItem->getId();
    m_currentItem->setCompleted(completed);
    
    // 更新列表但保持选中状态
    updateTodoList();
    
    // 恢复选中状态
    QList<TodoItem> items = m_currentFolder->getItems();
    for (int i = 0; i < items.size(); ++i) {
        if (items[i].getId() == currentItemId) {
            ui->todoListWidget->setCurrentRow(i);
            // 重新获取当前项目指针
            m_currentItem = m_currentFolder->findItem(currentItemId);
            break;
        }
    }
    
    // 恢复信号连接
    ui->todoListWidget->blockSignals(false);
    
    // 更新详情面板以反映完成状态变化
    if (m_currentItem) {
        updateDetailPanel();
    }
    
    updateFolderList();
    updateCalendarWidget();
    updateDesktopWidget();
    saveData();
}

void MainWindow::onSyncClicked()
{
    // TODO: 实现云端同步功能
    QMessageBox::information(this, "同步", "同步功能将在后续版本中实现");
}

void MainWindow::onImportClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "导入数据", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开文件");
        return;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject rootObj = doc.object();
    
    m_folders.clear();
    QJsonArray foldersArray = rootObj["folders"].toArray();
    for (const QJsonValue &value : foldersArray) {
        TodoFolder folder(value.toObject());
        m_folders.append(folder);
    }
    
    m_currentFolder = nullptr;
    m_currentItem = nullptr;
    updateFolderList();
    clearDetailPanel();
    
    ui->statusbar->showMessage("导入成功", 2000);
}

void MainWindow::onExportClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出数据", "todolist_backup.json", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;
    
    QJsonObject rootObj;
    QJsonArray foldersArray;
    
    for (const TodoFolder &folder : m_folders) {
        foldersArray.append(folder.toJson());
    }
    
    rootObj["folders"] = foldersArray;
    rootObj["exportTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(rootObj);
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        ui->statusbar->showMessage("导出成功", 2000);
    } else {
        QMessageBox::warning(this, "错误", "无法保存文件");
    }
}

void MainWindow::onExitClicked()
{
    close();
}

void MainWindow::onDesktopWidgetClicked()
{
    if (m_desktopWidget) {
        if (m_desktopWidget->isVisible()) {
            // 隐藏桌面小窗口
            m_desktopWidget->hide();
            ui->actionDesktopWidget->setText("显示桌面小贴士");
        } else {
            // 显示桌面小窗口
            m_desktopWidget->show();
            ui->actionDesktopWidget->setText("隐藏桌面小贴士");
        }
    }
}

void MainWindow::setupDesktopWidget()
{
    m_desktopWidget = new DesktopWidget();
    
    // 连接桌面小贴士的信号
    connect(m_desktopWidget, &DesktopWidget::newTodoRequested, this, &MainWindow::onDesktopNewTodo);
    connect(m_desktopWidget, &DesktopWidget::todoItemToggled, this, &MainWindow::onDesktopTodoToggled);
    connect(m_desktopWidget, &DesktopWidget::showMainWindowRequested, this, &MainWindow::onShowMainWindow);
    
    // 初始化数据
    updateDesktopWidget();
    
    // 显示桌面小贴士
    m_desktopWidget->show();
    
    // 初始化菜单文本
    ui->actionDesktopWidget->setText("隐藏桌面小贴士");
}

void MainWindow::updateDesktopWidget()
{
    if (m_desktopWidget) {
        // 确保m_folders不为空且有效
        if (!m_folders.isEmpty()) {
            m_desktopWidget->updateTodoData(m_folders);
        }
    }
}

void MainWindow::onDesktopNewTodo(const QString &title)
{
    // 添加到当前选中的文件夹，如果没有选中则添加到第一个文件夹
    TodoFolder* targetFolder = m_currentFolder;
    if (!targetFolder && !m_folders.isEmpty()) {
        targetFolder = &m_folders[0];
    }
    
    if (targetFolder) {
        TodoItem newItem(title);
        newItem.setPlannedDate(QDate::currentDate()); // 设置计划日期为今日
        targetFolder->addItem(newItem);
        
        // 更新界面
        updateTodoList();
        updateFolderList();
        updateDesktopWidget();
        saveData();
    }
}

void MainWindow::onDesktopTodoToggled(const QString &itemId, bool completed)
{
    // 在所有文件夹中查找并更新对应的待办事项
    for (TodoFolder &folder : m_folders) {
        QList<TodoItem> items = folder.getItems();
        for (int i = 0; i < items.size(); ++i) {
            if (items[i].getId() == itemId) {
                items[i].setCompleted(completed);
                folder.updateItem(items[i]);
                
                // 更新界面
                updateTodoList();
                updateFolderList();
                updateDesktopWidget();
                saveData();
                return;
            }
        }
    }
}

void MainWindow::onShowMainWindow()
{
    show();
    raise();
    activateWindow();
}

void MainWindow::setupCalendarWidget()
{
    // 创建日历组件
    m_calendarWidget = new CalendarWidget(this);
    
    // 将日历组件添加到日历标签页的容器中
    QVBoxLayout* calendarLayout = new QVBoxLayout(ui->calendarContainer);
    calendarLayout->addWidget(m_calendarWidget);
    
    // 连接日历组件的信号
    connect(m_calendarWidget, &CalendarWidget::todoItemAdded, this, &MainWindow::onCalendarTodoAdded);
    connect(m_calendarWidget, &CalendarWidget::todoItemToggled, this, &MainWindow::onCalendarTodoToggled);
    connect(m_calendarWidget, &CalendarWidget::todoItemDeleted, this, &MainWindow::onCalendarTodoDeleted);
    connect(m_calendarWidget, &CalendarWidget::todoItemUpdated, this, &MainWindow::onCalendarTodoUpdated);
}

void MainWindow::updateCalendarWidget()
{
    if (m_calendarWidget) {
        // 确保m_folders不为空且有效
        if (!m_folders.isEmpty()) {
            m_calendarWidget->updateTodoData(m_folders);
        }
    }
}



void MainWindow::onCalendarTodoAdded(const QString& title, const QDate& date)
{
    // 从日历视图添加新的待办事项
    TodoItem todoItem(title);
    todoItem.setPlannedDate(date); // 设置计划日期为选中的日期
    
    // 创建指定日期文件夹名称（格式：YYYY-MM-DD）
    QString dateFolderName = date.toString("yyyy-MM-dd");
    
    // 查找是否已存在该日期文件夹
    TodoFolder* dateFolder = nullptr;
    for (TodoFolder& folder : m_folders) {
        if (folder.getName() == dateFolderName) {
            dateFolder = &folder;
            break;
        }
    }
    
    // 如果不存在该日期文件夹，创建一个
    if (!dateFolder) {
        TodoFolder newDateFolder(dateFolderName);
        m_folders.append(newDateFolder);
        dateFolder = &m_folders.last();
    }
    
    // 将待办事项添加到指定日期文件夹
    todoItem.setFolderId(dateFolder->getId());
    dateFolder->addItem(todoItem);
    
    // 保存数据并更新界面
    saveData();
    updateFolderList();
    updateTodoList();
    updateCalendarWidget();
    updateDesktopWidget();
}

void MainWindow::onCalendarTodoToggled(const QString& itemId, bool completed)
{
    // 从日历视图切换待办事项的完成状态
    for (auto& folder : m_folders) {
        QList<TodoItem> items = folder.getItems();
        for (int i = 0; i < items.size(); ++i) {
            if (items[i].getId() == itemId) {
                items[i].setCompleted(completed);
                folder.updateItem(items[i]);
                
                // 更新界面
                updateTodoList();
                updateFolderList();
                updateCalendarWidget();
                updateDesktopWidget();
                saveData();
                return;
            }
        }
    }
}

void MainWindow::updateFolderList()
{
    ui->folderListWidget->clear();
    
    for (const TodoFolder &folder : m_folders) {
        QString displayText = QString("%1 (%2/%3)")
                             .arg(folder.getName())
                             .arg(folder.getCompletedCount())
                             .arg(folder.getItemCount());
        ui->folderListWidget->addItem(displayText);
    }
}

void MainWindow::updateTodoList()
{
    ui->todoListWidget->clear();
    
    if (!m_currentFolder) return;
    
    QList<TodoItem> items = m_currentFolder->getItems();
    for (const TodoItem &item : items) {
        QListWidgetItem *listItem = new QListWidgetItem();
        
        QString displayText = item.getTitle();
        if (item.isCompleted()) {
            displayText = "✓ " + displayText;
            listItem->setForeground(QColor(128, 128, 128)); // 灰色
            QFont font = listItem->font();
            font.setStrikeOut(true);
            listItem->setFont(font);
        } else {
            displayText = "○ " + displayText;
        }
        
        listItem->setText(displayText);
        ui->todoListWidget->addItem(listItem);
    }
}

void MainWindow::updateDetailPanel()
{
    if (!m_currentItem) {
        clearDetailPanel();
        return;
    }
    
    // 安全检查UI控件是否存在
    if (!ui->titleEdit || !ui->detailsEdit || !ui->createdTimeLabel || 
        !ui->completedTimeLabel || !ui->completedTimeLabel_title || 
        !ui->completedCheckBox || !ui->saveBtn || !ui->deleteBtn) {
        return;
    }
    
    ui->titleEdit->setText(m_currentItem->getTitle());
    ui->detailsEdit->setPlainText(m_currentItem->getDetails());
    ui->createdTimeLabel->setText(m_currentItem->getCreatedTime().toString("yyyy-MM-dd hh:mm:ss"));
    
    // 显示完成时间
    if (m_currentItem->isCompleted() && !m_currentItem->getCompletedTime().isNull()) {
        ui->completedTimeLabel->setText(m_currentItem->getCompletedTime().toString("yyyy-MM-dd hh:mm:ss"));
        ui->completedTimeLabel_title->setVisible(true);
        ui->completedTimeLabel->setVisible(true);
    } else {
        ui->completedTimeLabel->setText("-");
        ui->completedTimeLabel_title->setVisible(false);
        ui->completedTimeLabel->setVisible(false);
    }
    
    ui->completedCheckBox->setChecked(m_currentItem->isCompleted());
    
    // 启用编辑控件
    ui->titleEdit->setEnabled(true);
    ui->detailsEdit->setEnabled(true);
    ui->completedCheckBox->setEnabled(true);
    ui->saveBtn->setEnabled(true);
    ui->deleteBtn->setEnabled(true);
}

void MainWindow::clearDetailPanel()
{
    // 安全检查UI控件是否存在
    if (!ui->titleEdit || !ui->detailsEdit || !ui->createdTimeLabel || 
        !ui->completedTimeLabel || !ui->completedTimeLabel_title || 
        !ui->completedCheckBox || !ui->saveBtn || !ui->deleteBtn) {
        return;
    }
    
    ui->titleEdit->clear();
    ui->detailsEdit->clear();
    ui->createdTimeLabel->setText("-");
    ui->completedTimeLabel->setText("-");
    ui->completedTimeLabel_title->setVisible(false);
    ui->completedTimeLabel->setVisible(false);
    ui->completedCheckBox->setChecked(false);
    
    // 禁用编辑控件
    ui->titleEdit->setEnabled(false);
    ui->detailsEdit->setEnabled(false);
    ui->completedCheckBox->setEnabled(false);
    ui->saveBtn->setEnabled(false);
    ui->deleteBtn->setEnabled(false);
}

void MainWindow::loadData()
{
    // QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // QDir().mkpath(dataPath);
    // QString filePath = dataPath + "/todolist.json";
    QString filePath = "todolist.json";
    
    QFile file(filePath);
    if (!file.exists()) return;
    
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close(); // 确保文件被正确关闭
        
        // 检查数据是否为空
        if (data.isEmpty()) {
            qDebug() << "Warning: todolist.json is empty";
            return;
        }
        
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        
        // 检查JSON解析是否成功
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << parseError.errorString();
            return;
        }
        
        QJsonObject rootObj = doc.object();
        
        // 安全地清空现有数据
        m_folders.clear();
        
        QJsonArray foldersArray = rootObj["folders"].toArray();
        for (const QJsonValue &value : foldersArray) {
            if (value.isObject()) {
                TodoFolder folder(value.toObject());
                m_folders.append(folder);
            }
        }
        
        // 注意：不在这里调用updateCalendarWidget，因为此时CalendarWidget可能还未创建
    }
}

void MainWindow::saveData()
{
    // QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // QDir().mkpath(dataPath);
    // QString filePath = dataPath + "/todolist.json";
    QString filePath = "todolist.json";
    
    QJsonObject rootObj;
    QJsonArray foldersArray;
    
    for (const TodoFolder &folder : m_folders) {
        foldersArray.append(folder.toJson());
    }
    
    rootObj["folders"] = foldersArray;
    rootObj["saveTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(rootObj);
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        qint64 bytesWritten = file.write(doc.toJson());
        file.flush(); // 确保数据被写入磁盘
        file.close(); // 确保文件被正确关闭
        
        if (bytesWritten == -1) {
            qDebug() << "Error: Failed to write data to file";
        } else {
            qDebug() << "Data saved successfully, bytes written:" << bytesWritten;
        }
    } else {
        qDebug() << "Error: Cannot open file for writing:" << file.errorString();
    }
}

TodoFolder* MainWindow::findFolderById(const QString &folderId)
{
    for (TodoFolder &folder : m_folders) {
        if (folder.getId() == folderId) {
            return &folder;
        }
    }
    return nullptr;
}

void MainWindow::onCalendarTodoDeleted(const QString &itemId)
{
    // 从日历视图删除待办事项
    for (TodoFolder &folder : m_folders) {
        if (folder.findItem(itemId)) {
            folder.removeItem(itemId);
            // 保存数据并更新界面
            saveData();
            updateFolderList();
            updateTodoList();
            updateCalendarWidget();
            updateDesktopWidget();
            break;
        }
    }
}

void MainWindow::onCalendarTodoUpdated(const QString &itemId, const TodoItem &item)
{
    Q_UNUSED(itemId)  // 使用item.getId()来查找，不需要额外的itemId参数
    // 从日历视图更新待办事项
    for (TodoFolder &folder : m_folders) {
        if (folder.findItem(item.getId())) {
            folder.updateItem(item);
            // 保存数据并更新界面
            saveData();
            updateFolderList();
            updateTodoList();
            updateCalendarWidget();
            updateDesktopWidget();
            break;
        }
    }
}

void MainWindow::setupSystemTray()
{
    // 检查系统是否支持托盘图标
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(this, "系统托盘", "系统不支持托盘图标功能。");
        return;
    }
    
    // 创建托盘图标
    m_trayIcon = new QSystemTrayIcon(this);
    QIcon trayIcon(":/icons/app.ico");
    if (trayIcon.isNull()) {
        // 如果资源图标加载失败，尝试文件路径
        QString iconPath = QDir::currentPath() + "/icons/app.ico";
        trayIcon = QIcon(iconPath);
        if (trayIcon.isNull()) {
            // 如果文件路径也失败，尝试相对路径
            trayIcon = QIcon("icons/app.ico");
            if (trayIcon.isNull()) {
                // 如果还是失败，使用默认图标
                trayIcon = style()->standardIcon(QStyle::SP_ComputerIcon);
            }
        }
    }
    m_trayIcon->setIcon(trayIcon);
    m_trayIcon->setToolTip("Todo List - 待办事项管理");
    
    // 创建托盘菜单
    m_trayMenu = new QMenu(this);
    
    // 创建菜单项
    m_showAction = new QAction("显示主窗口", this);
    m_showAction->setIcon(trayIcon);  // 使用相同的图标
    
    m_exitAction = new QAction("退出", this);
    m_exitAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    
    // 添加菜单项到托盘菜单
    m_trayMenu->addAction(m_showAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_exitAction);
    
    // 设置托盘菜单
    m_trayIcon->setContextMenu(m_trayMenu);
    
    // 连接信号和槽
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    connect(m_showAction, &QAction::triggered, this, &MainWindow::onShowFromTray);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::onExitFromTray);
    
    // 显示托盘图标
    m_trayIcon->show();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::DoubleClick:
        onShowFromTray();
        break;
    case QSystemTrayIcon::Trigger:
        // 单击时显示提示信息
        m_trayIcon->showMessage("Todo List", "应用程序正在后台运行，双击图标显示主窗口", QSystemTrayIcon::Information, 3000);
        break;
    default:
        break;
    }
}

void MainWindow::onShowFromTray()
{
    show();
    raise();
    activateWindow();
    setWindowState(windowState() & ~Qt::WindowMinimized);
}

void MainWindow::onExitFromTray()
{
    // 保存数据后真正退出应用程序
    saveData();
    QApplication::quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        // 最小化到托盘而不是退出
        hide();
        m_trayIcon->showMessage("Todo List", "应用程序已最小化到系统托盘", QSystemTrayIcon::Information, 2000);
        event->ignore();
    } else {
        event->accept();
    }
}
