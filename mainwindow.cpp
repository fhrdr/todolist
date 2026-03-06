#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "messageutils.h"
#include <QStandardPaths>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QVBoxLayout>
#include <QMenu>
#include <QTimer>
#include <QStyledItemDelegate>
#include <QPainter>

class TodoItemDelegate : public QStyledItemDelegate
{
public:
    TodoItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        
        bool isSelected = option.state & QStyle::State_Selected;
        if (isSelected) {
            painter->fillRect(option.rect, QColor(239, 246, 255));
        }
        
        QString text = index.data(Qt::DisplayRole).toString();
        QStringList lines = text.split('\n');
        QString titleLine = lines.value(0);
        QString subLine = lines.value(1, "");
        
        QString title;
        QString date;
        int tabPos = titleLine.indexOf('\t');
        if (tabPos > 0) {
            title = titleLine.left(tabPos);
            date = titleLine.mid(tabPos + 1);
        } else {
            title = titleLine;
        }
        
        bool isCompleted = index.data(Qt::ForegroundRole).isValid() && 
                          index.data(Qt::ForegroundRole).value<QColor>() == QColor(156, 163, 175);
        
        QRect rect = option.rect.adjusted(12, 6, -12, -6);
        
        QFont titleFont;
        titleFont.setPixelSize(14);
        titleFont.setBold(true);
        if (isCompleted) {
            titleFont.setStrikeOut(true);
        }
        painter->setFont(titleFont);
        
        QColor titleColor = isCompleted ? QColor(156, 163, 175) : QColor(30, 41, 59);
        painter->setPen(titleColor);
        
        QFontMetrics fm(titleFont);
        int dateWidth = date.isEmpty() ? 0 : fm.horizontalAdvance(date) + 20;
        int titleWidth = rect.width() - dateWidth - 10;
        QString elidedTitle = fm.elidedText(title, Qt::ElideRight, titleWidth);
        painter->drawText(QRect(rect.left(), rect.top(), titleWidth, 22), Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);
        
        if (!date.isEmpty()) {
            QFont dateFont;
            dateFont.setPixelSize(12);
            painter->setFont(dateFont);
            painter->setPen(QColor(100, 116, 139));
            painter->drawText(QRect(rect.right() - dateWidth + 10, rect.top(), dateWidth, 22), Qt::AlignRight | Qt::AlignVCenter, date);
        }
        
        if (!subLine.isEmpty()) {
            QFont subFont;
            subFont.setPixelSize(11);
            painter->setFont(subFont);
            painter->setPen(QColor(148, 163, 184));
            
            QFontMetrics subFm(subFont);
            QString elidedSub = subFm.elidedText(subLine, Qt::ElideRight, rect.width());
            painter->drawText(QRect(rect.left(), rect.top() + 24, rect.width(), 18), Qt::AlignLeft | Qt::AlignVCenter, elidedSub);
        }
        
        painter->restore();
    }
    
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(option)
        Q_UNUSED(index)
        return QSize(200, 52);
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_currentFolder(nullptr)
    , m_currentItem(nullptr)
    , m_desktopWidget(nullptr)
    , m_tagWidget(nullptr)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
    , m_showAction(nullptr)
    , m_exitAction(nullptr)
    , m_mainSplitter(nullptr)
{
    ui->setupUi(this);
    
    QIcon windowIcon(":/icons/app.ico");
    if (windowIcon.isNull()) {
        QString iconPath = QDir::currentPath() + "/icons/app.ico";
        windowIcon = QIcon(iconPath);
        if (windowIcon.isNull()) {
            windowIcon = QIcon("icons/app.ico");
        }
    }
    setWindowIcon(windowIcon);
    
    initDatabase();
    migrateFromJson();
    loadData();
    updateFolderList();
    setupConnections();
    setupSplitter();
    setupDesktopWidget();
    setupCalendarWidget();
    setupTagWidget();
    updateCalendarWidget();
    updateTagWidget();
    setupSystemTray();
    loadSplitterState();
    
    ui->todoListWidget->setItemDelegate(new TodoItemDelegate(this));
    
    if (m_desktopWidget) {
        m_desktopWidget->show();
    }
}

MainWindow::~MainWindow()
{
    saveSplitterState();
    saveData();
    if (m_desktopWidget) {
        m_desktopWidget->close();
        delete m_desktopWidget;
    }
    if (m_db.isOpen()) {
        m_db.close();
    }
    delete ui;
}

void MainWindow::initDatabase()
{
    try {
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(dataPath);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                MessageUtils::showError(this, "初始化失败", "无法创建数据目录");
                return;
            }
        }
        
        QString dbPath = dataPath + "/todolist.db";
        
        if (QSqlDatabase::contains("qt_sql_default_connection")) {
            m_db = QSqlDatabase::database("qt_sql_default_connection");
        } else {
            m_db = QSqlDatabase::addDatabase("QSQLITE");
        }
        m_db.setDatabaseName(dbPath);
        
        if (!m_db.open()) {
            MessageUtils::showError(this, "数据库错误", "无法打开数据库: " + m_db.lastError().text());
            return;
        }
        
        QSqlQuery query;
        
        if (!query.exec("CREATE TABLE IF NOT EXISTS folders ("
                   "id TEXT PRIMARY KEY, "
                   "name TEXT NOT NULL, "
                   "createdTime TEXT, "
                   "isPinned INTEGER DEFAULT 0, "
                   "color TEXT DEFAULT '#2563eb')")) {
            MessageUtils::showError(this, "数据库错误", "创建文件夹表失败: " + query.lastError().text());
            return;
        }
        
        if (!query.exec("CREATE TABLE IF NOT EXISTS items ("
                   "id TEXT PRIMARY KEY, "
                   "title TEXT NOT NULL, "
                   "details TEXT, "
                   "createdTime TEXT, "
                   "completedTime TEXT, "
                   "updatedTime TEXT, "
                   "isCompleted INTEGER DEFAULT 0, "
                   "folderId TEXT, "
                   "plannedDate TEXT, "
                   "dueDate TEXT, "
                   "priority INTEGER DEFAULT 0, "
                   "tagColor TEXT DEFAULT '#2563eb', "
                   "isPinned INTEGER DEFAULT 0, "
                   "FOREIGN KEY(folderId) REFERENCES folders(id))")) {
            MessageUtils::showError(this, "数据库错误", "创建事项表失败: " + query.lastError().text());
            return;
        }
        
        if (!query.exec("CREATE TABLE IF NOT EXISTS tags ("
                   "id TEXT PRIMARY KEY, "
                   "name TEXT NOT NULL UNIQUE, "
                   "color TEXT DEFAULT '#2563eb')")) {
            MessageUtils::showError(this, "数据库错误", "创建标签表失败: " + query.lastError().text());
            return;
        }
        
        if (!query.exec("CREATE TABLE IF NOT EXISTS item_tags ("
                   "itemId TEXT, "
                   "tagId TEXT, "
                   "PRIMARY KEY(itemId, tagId), "
                   "FOREIGN KEY(itemId) REFERENCES items(id), "
                   "FOREIGN KEY(tagId) REFERENCES tags(id))")) {
            MessageUtils::showError(this, "数据库错误", "创建标签关联表失败: " + query.lastError().text());
            return;
        }
    } catch (const std::exception &e) {
        MessageUtils::showError(this, "初始化异常", QString("数据库初始化发生异常: %1").arg(e.what()));
    } catch (...) {
        MessageUtils::showError(this, "初始化异常", "数据库初始化发生未知异常");
    }
}

void MainWindow::migrateFromJson()
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString jsonPath = dataPath + "/todolist.json";
    
    if (!QFile::exists(jsonPath)) {
        return;
    }
    
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray foldersArray = root["folders"].toArray();
    
    QSqlQuery query;
    
    for (const QJsonValue &folderVal : foldersArray) {
        QJsonObject folderObj = folderVal.toObject();
        
        query.prepare("INSERT OR REPLACE INTO folders (id, name, createdTime, isPinned, color) VALUES (?, ?, ?, ?, ?)");
        query.addBindValue(folderObj["id"].toString());
        query.addBindValue(folderObj["name"].toString());
        query.addBindValue(folderObj["createdTime"].toString());
        query.addBindValue(folderObj["isPinned"].toBool() ? 1 : 0);
        query.addBindValue(folderObj["color"].toString("#2563eb"));
        query.exec();
        
        QJsonArray itemsArray = folderObj["items"].toArray();
        for (const QJsonValue &itemVal : itemsArray) {
            QJsonObject itemObj = itemVal.toObject();
            
            query.prepare("INSERT OR REPLACE INTO items (id, title, details, createdTime, completedTime, updatedTime, isCompleted, folderId, plannedDate, dueDate, priority, tagColor, isPinned) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            query.addBindValue(itemObj["id"].toString());
            query.addBindValue(itemObj["title"].toString());
            query.addBindValue(itemObj["details"].toString());
            query.addBindValue(itemObj["createdTime"].toString());
            query.addBindValue(itemObj["completedTime"].toString());
            query.addBindValue(itemObj["updatedTime"].toString());
            query.addBindValue(itemObj["isCompleted"].toBool() ? 1 : 0);
            query.addBindValue(folderObj["id"].toString());
            query.addBindValue(itemObj["plannedDate"].toString());
            query.addBindValue(itemObj["dueDate"].toString());
            query.addBindValue(itemObj["priority"].toInt(0));
            query.addBindValue(itemObj["tagColor"].toString("#2563eb"));
            query.addBindValue(itemObj["isPinned"].toBool() ? 1 : 0);
            query.exec();
        }
    }
    
    QString backupPath = jsonPath + ".backup";
    QFile::rename(jsonPath, backupPath);
}

void MainWindow::loadData()
{
    try {
        if (!m_db.isOpen()) {
            if (!m_db.open()) {
                MessageUtils::showError(this, "数据库错误", "无法打开数据库加载数据");
                return;
            }
        }
        
        m_folders.clear();
        
        QSqlQuery folderQuery;
        if (!folderQuery.exec("SELECT id, name, createdTime, isPinned, color FROM folders ORDER BY isPinned DESC, createdTime DESC")) {
            MessageUtils::showError(this, "加载错误", "加载文件夹失败: " + folderQuery.lastError().text());
            return;
        }
        
        while (folderQuery.next()) {
            TodoFolder folder;
            folder.setId(folderQuery.value(0).toString());
            folder.setName(folderQuery.value(1).toString());
            folder.setCreatedTime(QDateTime::fromString(folderQuery.value(2).toString(), Qt::ISODate));
            folder.setPinned(folderQuery.value(3).toInt() == 1);
            folder.setColor(folderQuery.value(4).toString());
            
            QSqlQuery itemQuery;
            itemQuery.prepare("SELECT id, title, details, createdTime, completedTime, updatedTime, isCompleted, folderId, plannedDate, dueDate, priority, tagColor, isPinned FROM items WHERE folderId = ? ORDER BY isPinned DESC, createdTime DESC");
            itemQuery.addBindValue(folder.getId());
            if (!itemQuery.exec()) {
                continue;
            }
            
            while (itemQuery.next()) {
                TodoItem item;
                item.setId(itemQuery.value(0).toString());
                item.setTitle(itemQuery.value(1).toString());
                item.setDetails(itemQuery.value(2).toString());
                item.setCreatedTime(QDateTime::fromString(itemQuery.value(3).toString(), Qt::ISODate));
                item.setCompletedTime(QDateTime::fromString(itemQuery.value(4).toString(), Qt::ISODate));
                item.setUpdatedTime(QDateTime::fromString(itemQuery.value(5).toString(), Qt::ISODate));
                item.setCompleted(itemQuery.value(6).toInt() == 1);
                item.setFolderId(itemQuery.value(7).toString());
                item.setPlannedDate(QDate::fromString(itemQuery.value(8).toString(), Qt::ISODate));
                item.setDueDate(QDate::fromString(itemQuery.value(9).toString(), Qt::ISODate));
                item.setPriority(itemQuery.value(10).toInt());
                item.setTagColor(itemQuery.value(11).toString());
                item.setPinned(itemQuery.value(12).toInt() == 1);
                
                QSqlQuery tagQuery;
                tagQuery.prepare("SELECT t.name FROM tags t JOIN item_tags it ON t.id = it.tagId WHERE it.itemId = ?");
                tagQuery.addBindValue(item.getId());
                if (tagQuery.exec()) {
                    QStringList tags;
                    while (tagQuery.next()) {
                        tags.append(tagQuery.value(0).toString());
                    }
                    item.setTags(tags);
                }
                
                folder.addItem(item);
            }
            
            m_folders.append(folder);
        }
        
        if (m_folders.isEmpty()) {
            TodoFolder defaultFolder("默认文件夹");
            m_folders.append(defaultFolder);
            
            TodoItem sampleItem("欢迎使用Todo List", "这是一个示例待办事项，您可以编辑或删除它。");
            m_folders[0].addItem(sampleItem);
        }
    } catch (const std::exception &e) {
        MessageUtils::showError(this, "加载异常", QString("加载数据发生异常: %1").arg(e.what()));
    } catch (...) {
        MessageUtils::showError(this, "加载异常", "加载数据发生未知异常");
    }
}

void MainWindow::saveData()
{
    try {
        if (!m_db.isOpen()) {
            if (!m_db.open()) {
                MessageUtils::showError(this, "保存错误", "无法打开数据库保存数据");
                return;
            }
        }
        
        QSqlQuery query;
        
        if (!query.exec("DELETE FROM item_tags")) {
            qWarning() << "清除item_tags失败:" << query.lastError().text();
        }
        if (!query.exec("DELETE FROM items")) {
            qWarning() << "清除items失败:" << query.lastError().text();
        }
        if (!query.exec("DELETE FROM folders")) {
            qWarning() << "清除folders失败:" << query.lastError().text();
        }
        
        for (const TodoFolder &folder : m_folders) {
            query.prepare("INSERT INTO folders (id, name, createdTime, isPinned, color) VALUES (?, ?, ?, ?, ?)");
            query.addBindValue(folder.getId());
            query.addBindValue(folder.getName());
            query.addBindValue(folder.getCreatedTime().toString(Qt::ISODate));
            query.addBindValue(folder.isPinned() ? 1 : 0);
            query.addBindValue(folder.getColor());
            if (!query.exec()) {
                qWarning() << "插入文件夹失败:" << query.lastError().text();
                continue;
            }
            
            for (const TodoItem &item : folder.getItems()) {
                query.prepare("INSERT INTO items (id, title, details, createdTime, completedTime, updatedTime, isCompleted, folderId, plannedDate, dueDate, priority, tagColor, isPinned) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
                query.addBindValue(item.getId());
                query.addBindValue(item.getTitle());
                query.addBindValue(item.getDetails());
                query.addBindValue(item.getCreatedTime().toString(Qt::ISODate));
                query.addBindValue(item.getCompletedTime().toString(Qt::ISODate));
                query.addBindValue(item.getUpdatedTime().toString(Qt::ISODate));
                query.addBindValue(item.isCompleted() ? 1 : 0);
                query.addBindValue(folder.getId());
                query.addBindValue(item.getPlannedDate().toString(Qt::ISODate));
                query.addBindValue(item.getDueDate().toString(Qt::ISODate));
                query.addBindValue(item.getPriority());
                query.addBindValue(item.getTagColor());
                query.addBindValue(item.isPinned() ? 1 : 0);
                if (!query.exec()) {
                    qWarning() << "插入事项失败:" << query.lastError().text();
                    continue;
                }
                
                for (const QString &tag : item.getTags()) {
                    query.prepare("INSERT OR IGNORE INTO tags (id, name) VALUES (?, ?)");
                    query.addBindValue(QUuid::createUuid().toString(QUuid::WithoutBraces));
                    query.addBindValue(tag);
                    query.exec();
                    
                    query.prepare("INSERT INTO item_tags (itemId, tagId) SELECT ?, id FROM tags WHERE name = ?");
                    query.addBindValue(item.getId());
                    query.addBindValue(tag);
                    query.exec();
                }
            }
        }
    } catch (const std::exception &e) {
        MessageUtils::showError(this, "保存异常", QString("保存数据发生异常: %1").arg(e.what()));
    } catch (...) {
        MessageUtils::showError(this, "保存异常", "保存数据发生未知异常");
    }
}

void MainWindow::setupConnections()
{
    connect(ui->newFolderBtn, &QPushButton::clicked, this, &MainWindow::onNewFolderClicked);
    connect(ui->deleteFolderBtn, &QPushButton::clicked, this, &MainWindow::onDeleteFolderClicked);
    connect(ui->folderListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onFolderSelectionChanged);
    connect(ui->folderListWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onFolderDoubleClicked);
    ui->folderListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->folderListWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::onFolderContextMenu);
    
    connect(ui->newTodoBtn, &QPushButton::clicked, this, &MainWindow::onNewTodoClicked);
    connect(ui->todoListWidget, &QListWidget::currentRowChanged, this, &MainWindow::onTodoSelectionChanged);
    ui->todoListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->todoListWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::onTodoContextMenu);
    connect(ui->todoListWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onTodoDoubleClicked);
    connect(ui->saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(ui->deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    connect(ui->completedCheckBox, &QCheckBox::toggled, this, &MainWindow::onCompletedToggled);
    connect(ui->syncBtn, &QPushButton::clicked, this, &MainWindow::onSyncClicked);
    
    connect(ui->actionImport, &QAction::triggered, this, &MainWindow::onImportClicked);
    connect(ui->actionExport, &QAction::triggered, this, &MainWindow::onExportClicked);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onExitClicked);
    connect(ui->actionDesktopWidget, &QAction::triggered, this, &MainWindow::onDesktopWidgetClicked);
}

void MainWindow::onNewFolderClicked()
{
    try {
        bool ok;
        QString folderName = QInputDialog::getText(this, "新建文件夹", "请输入文件夹名称:", QLineEdit::Normal, "新建文件夹", &ok);
        
        if (ok && !folderName.isEmpty()) {
            TodoFolder newFolder(folderName);
            m_folders.append(newFolder);
            
            saveData();
            updateFolderList();
            updateCalendarWidget();
            updateTagWidget();
            
            for (int i = 0; i < m_folders.size(); ++i) {
                if (m_folders[i].getId() == newFolder.getId()) {
                    ui->folderListWidget->setCurrentRow(i);
                    break;
                }
            }
        }
    } catch (const std::exception &e) {
        MessageUtils::showError(this, "创建失败", QString("创建文件夹时发生错误: %1").arg(e.what()));
    } catch (...) {
        MessageUtils::showError(this, "创建失败", "创建文件夹时发生未知错误");
    }
}

void MainWindow::onFolderSelectionChanged()
{
    int row = ui->folderListWidget->currentRow();
    if (row >= 0 && row < m_folders.size()) {
        m_currentFolder = &m_folders[row];
        ui->deleteFolderBtn->setEnabled(true);
        updateTodoList();
        updateDetailPanel();
    } else {
        m_currentFolder = nullptr;
        ui->deleteFolderBtn->setEnabled(false);
        ui->todoListWidget->clear();
        clearDetailPanel();
    }
}

void MainWindow::onDeleteFolderClicked()
{
    try {
        if (!m_currentFolder) {
            MessageUtils::showInfo(this, "提示", "请先选择一个文件夹。");
            return;
        }
        
        QString folderName = m_currentFolder->getName();
        QString folderId = m_currentFolder->getId();
        
        if (MessageUtils::showConfirm(this, "确认删除", 
            QString("确定要删除文件夹 \"%1\" 及其所有待办事项吗？").arg(folderName))) {
            
            for (int i = 0; i < m_folders.size(); ++i) {
                if (m_folders[i].getId() == folderId) {
                    m_folders.removeAt(i);
                    break;
                }
            }
            
            m_currentFolder = nullptr;
            m_currentItem = nullptr;
            
            saveData();
            updateFolderList();
            updateCalendarWidget();
            updateTagWidget();
            updateDesktopWidget();
            
            ui->todoListWidget->clear();
            clearDetailPanel();
            
            if (!m_folders.isEmpty()) {
                ui->folderListWidget->setCurrentRow(0);
            }
        }
    } catch (const std::exception &e) {
        MessageUtils::showError(this, "删除失败", QString("删除文件夹时发生错误: %1").arg(e.what()));
    } catch (...) {
        MessageUtils::showError(this, "删除失败", "删除文件夹时发生未知错误");
    }
}

void MainWindow::onPinFolderClicked()
{
    if (!m_currentFolder) return;
    
    m_currentFolder->setPinned(!m_currentFolder->isPinned());
    updateFolderList();
    saveData();
}

void MainWindow::onFolderDoubleClicked(QListWidgetItem* item)
{
    int row = ui->folderListWidget->row(item);
    if (row >= 0 && row < m_folders.size()) {
        bool ok;
        QString newName = QInputDialog::getText(this, "重命名文件夹", "请输入新的文件夹名称:", 
            QLineEdit::Normal, m_folders[row].getName(), &ok);
        
        if (ok && !newName.isEmpty()) {
            m_folders[row].setName(newName);
            updateFolderList();
            updateCalendarWidget();
            saveData();
        }
    }
}

void MainWindow::onFolderContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->folderListWidget->itemAt(pos);
    if (!item) return;
    
    int row = ui->folderListWidget->row(item);
    if (row < 0 || row >= m_folders.size()) return;
    
    m_currentFolder = &m_folders[row];
    
    QMenu menu(this);
    QAction *pinAction = menu.addAction(m_currentFolder->isPinned() ? "取消置顶" : "置顶");
    QAction *renameAction = menu.addAction("重命名");
    QAction *deleteAction = menu.addAction("删除");
    
    connect(pinAction, &QAction::triggered, this, &MainWindow::onPinFolderClicked);
    connect(renameAction, &QAction::triggered, [this, item]() { onFolderDoubleClicked(item); });
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteFolderClicked);
    
    menu.exec(ui->folderListWidget->mapToGlobal(pos));
}

void MainWindow::onNewTodoClicked()
{
    try {
        if (!m_currentFolder) {
            MessageUtils::showInfo(this, "提示", "请先选择一个文件夹。");
            return;
        }
        
        bool ok;
        QString title = QInputDialog::getText(this, "新建待办事项", "请输入待办事项标题:", QLineEdit::Normal, "", &ok);
        
        if (ok && !title.isEmpty()) {
            TodoItem newItem(title);
            newItem.setPlannedDate(QDate::currentDate());
            QString newItemId = newItem.getId();
            m_currentFolder->addItem(newItem);
            
            saveData();
            updateTodoList();
            updateFolderList();
            updateCalendarWidget();
            updateTagWidget();
            updateDesktopWidget();
            
            QList<TodoItem> items = m_currentFolder->getItems();
            for (int i = 0; i < items.size(); ++i) {
                if (items[i].getId() == newItemId) {
                    ui->todoListWidget->setCurrentRow(i);
                    break;
                }
            }
        }
    } catch (const std::exception &e) {
        MessageUtils::showError(this, "创建失败", QString("创建待办事项时发生错误: %1").arg(e.what()));
    } catch (...) {
        MessageUtils::showError(this, "创建失败", "创建待办事项时发生未知错误");
    }
}

void MainWindow::onTodoSelectionChanged()
{
    if (!m_currentFolder) return;
    
    int row = ui->todoListWidget->currentRow();
    QList<TodoItem> items = m_currentFolder->getItems();
    
    if (row >= 0 && row < items.size()) {
        m_currentItem = m_currentFolder->findItem(items[row].getId());
        updateDetailPanel();
    } else {
        m_currentItem = nullptr;
        clearDetailPanel();
    }
}

void MainWindow::onTodoDoubleClicked(QListWidgetItem* item)
{
    Q_UNUSED(item)
    if (m_currentItem) {
        ui->tabWidget->setCurrentIndex(0);
    }
}

void MainWindow::onTodoContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = ui->todoListWidget->itemAt(pos);
    if (!item || !m_currentFolder) return;
    
    int row = ui->todoListWidget->row(item);
    QList<TodoItem> items = m_currentFolder->getItems();
    if (row < 0 || row >= items.size()) return;
    
    m_currentItem = m_currentFolder->findItem(items[row].getId());
    
    QMenu menu(this);
    QAction *pinAction = menu.addAction(m_currentItem->isPinned() ? "取消置顶" : "置顶");
    menu.addSeparator();
    QMenu *tagMenu = menu.addMenu("添加标签");
    
    QSet<QString> existingTags;
    for (const TodoFolder &folder : m_folders) {
        for (const TodoItem &item : folder.getItems()) {
            for (const QString &tag : item.getTags()) {
                existingTags.insert(tag);
            }
        }
    }
    
    QStringList sortedTags = existingTags.values();
    std::sort(sortedTags.begin(), sortedTags.end());
    
    if (sortedTags.isEmpty()) {
        QAction *noTagAction = tagMenu->addAction("(暂无标签)");
        noTagAction->setEnabled(false);
    } else {
        for (const QString &tag : sortedTags) {
            QAction *tagAction = tagMenu->addAction(tag);
            connect(tagAction, &QAction::triggered, [this, tag]() {
                if (m_currentItem) {
                    m_currentItem->addTag(tag);
                    updateTodoList();
                    updateTagWidget();
                    saveData();
                }
            });
        }
    }
    
    QAction *deleteAction = menu.addAction("删除");
    
    connect(pinAction, &QAction::triggered, [this]() {
        if (m_currentItem) {
            m_currentItem->setPinned(!m_currentItem->isPinned());
            updateTodoList();
            saveData();
        }
    });
    
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteClicked);
    
    menu.exec(ui->todoListWidget->mapToGlobal(pos));
}

void MainWindow::onSaveClicked()
{
    if (!m_currentItem) {
        return;
    }
    
    m_currentItem->setTitle(ui->titleEdit->text());
    m_currentItem->setDetails(ui->detailsEdit->toPlainText());
    m_currentItem->setCompleted(ui->completedCheckBox->isChecked());
    
    updateTodoList();
    updateFolderList();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();
    saveData();
}

void MainWindow::onDeleteClicked()
{
    if (!m_currentItem || !m_currentFolder) {
        QMessageBox::information(this, "提示", "请先选择一个待办事项。");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认删除", 
        QString("确定要删除待办事项 \"%1\" 吗？").arg(m_currentItem->getTitle()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_currentFolder->removeItem(m_currentItem->getId());
        m_currentItem = nullptr;
        
        updateTodoList();
        updateFolderList();
        updateCalendarWidget();
        updateTagWidget();
        updateDesktopWidget();
        saveData();
        
        clearDetailPanel();
    }
}

void MainWindow::onCompletedToggled(bool completed)
{
    if (m_currentItem) {
        m_currentItem->setCompleted(completed);
        updateTodoList();
        updateFolderList();
        updateCalendarWidget();
        updateTagWidget();
        updateDesktopWidget();
        saveData();
    }
}

void MainWindow::onSyncClicked()
{
    saveData();
    updateDesktopWidget();
    updateCalendarWidget();
    updateTagWidget();
    QMessageBox::information(this, "同步", "数据已同步！");
}

void MainWindow::onTagSelected(const QString &tag)
{
    m_selectedTag = tag;
}

void MainWindow::onTodoTagAdded(const QString &todoId, const QString &tag)
{
    for (TodoFolder &folder : m_folders) {
        TodoItem *item = folder.findItem(todoId);
        if (item) {
            item->addTag(tag);
            saveData();
            updateTodoList();
            updateTagWidget();
            break;
        }
    }
}

void MainWindow::onTodoTagRemoved(const QString &todoId, const QString &tag)
{
    for (TodoFolder &folder : m_folders) {
        TodoItem *item = folder.findItem(todoId);
        if (item) {
            item->removeTag(tag);
            saveData();
            updateTodoList();
            updateTagWidget();
            break;
        }
    }
}

void MainWindow::updateFolderList()
{
    QString currentFolderId;
    if (m_currentFolder) {
        currentFolderId = m_currentFolder->getId();
    }
    
    ui->folderListWidget->clear();
    
    std::sort(m_folders.begin(), m_folders.end(), [](const TodoFolder &a, const TodoFolder &b) {
        if (a.isPinned() != b.isPinned()) {
            return a.isPinned() > b.isPinned();
        }
        return a.getCreatedTime() > b.getCreatedTime();
    });
    
    int selectRow = -1;
    for (int i = 0; i < m_folders.size(); ++i) {
        const TodoFolder &folder = m_folders[i];
        QString displayText = folder.getName();
        if (folder.isPinned()) {
            displayText = "📌 " + displayText;
        }
        displayText += QString(" (%1/%2)").arg(folder.getCompletedCount()).arg(folder.getItemCount());
        
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, folder.getId());
        ui->folderListWidget->addItem(item);
        
        if (folder.getId() == currentFolderId) {
            selectRow = i;
            m_currentFolder = &m_folders[i];
        }
    }
    
    if (selectRow >= 0) {
        ui->folderListWidget->setCurrentRow(selectRow);
    }
}

void MainWindow::updateTodoList()
{
    QString currentItemId;
    if (m_currentItem) {
        currentItemId = m_currentItem->getId();
    }
    
    ui->todoListWidget->clear();
    
    if (!m_currentFolder) return;
    
    QList<TodoItem> items = m_currentFolder->getItems();
    std::sort(items.begin(), items.end(), [](const TodoItem &a, const TodoItem &b) {
        if (a.isPinned() != b.isPinned()) {
            return a.isPinned() > b.isPinned();
        }
        if (a.isCompleted() != b.isCompleted()) {
            return a.isCompleted() < b.isCompleted();
        }
        return a.getCreatedTime() > b.getCreatedTime();
    });
    
    int selectRow = -1;
    for (int i = 0; i < items.size(); ++i) {
        const TodoItem &item = items[i];
        QString displayText;
        if (item.isPinned()) {
            displayText = "📌 ";
        }
        displayText += item.getTitle();
        
        QString subText;
        if (!item.getDetails().isEmpty()) {
            subText = item.getDetails().left(30);
            if (item.getDetails().length() > 30) subText += "...";
        } else {
            subText = QString::fromUtf8("还没有写任何内容呢~");
        }
        
        QString dateText;
        if (item.getPlannedDate().isValid()) {
            dateText = item.getPlannedDate().toString("MM-dd");
        }
        
        QString fullText = displayText + "\t" + dateText + "\n" + subText;
        
        QListWidgetItem *listItem = new QListWidgetItem();
        listItem->setText(fullText);
        
        if (item.isCompleted()) {
            listItem->setForeground(QColor(156, 163, 175));
        }
        
        listItem->setData(Qt::UserRole, item.getId());
        ui->todoListWidget->addItem(listItem);
        
        if (item.getId() == currentItemId) {
            selectRow = i;
            m_currentItem = m_currentFolder->findItem(item.getId());
        }
    }
    
    if (selectRow >= 0) {
        ui->todoListWidget->setCurrentRow(selectRow);
    }
}

void MainWindow::updateDetailPanel()
{
    if (!m_currentItem) {
        clearDetailPanel();
        return;
    }
    
    if (!ui->titleEdit || !ui->detailsEdit || !ui->createdTimeLabel || 
        !ui->completedTimeLabel || !ui->completedTimeLabel_title || 
        !ui->completedCheckBox || !ui->saveBtn || !ui->deleteBtn) {
        return;
    }
    
    ui->titleEdit->setText(m_currentItem->getTitle());
    ui->detailsEdit->setPlainText(m_currentItem->getDetails());
    ui->createdTimeLabel->setText(m_currentItem->getCreatedTime().toString("yyyy-MM-dd hh:mm:ss"));
    
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
    
    ui->titleEdit->setEnabled(true);
    ui->detailsEdit->setEnabled(true);
    ui->completedCheckBox->setEnabled(true);
    ui->saveBtn->setEnabled(true);
    ui->deleteBtn->setEnabled(true);
}

void MainWindow::clearDetailPanel()
{
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
    
    ui->titleEdit->setEnabled(false);
    ui->detailsEdit->setEnabled(false);
    ui->completedCheckBox->setEnabled(false);
    ui->saveBtn->setEnabled(false);
    ui->deleteBtn->setEnabled(false);
}

void MainWindow::setupDesktopWidget()
{
    m_desktopWidget = new DesktopWidget();
    m_desktopWidget->updateTodoData(m_folders);
    
    connect(m_desktopWidget, &DesktopWidget::newTodoRequested, this, &MainWindow::onDesktopNewTodo);
    connect(m_desktopWidget, &DesktopWidget::todoItemToggled, this, &MainWindow::onDesktopTodoToggled);
    connect(m_desktopWidget, &DesktopWidget::showMainWindowRequested, this, &MainWindow::onShowMainWindow);
    connect(m_desktopWidget, &DesktopWidget::editTodoRequested, this, [this](const QString &itemId) {
        try {
            for (int i = 0; i < m_folders.size(); ++i) {
                TodoItem *item = m_folders[i].findItem(itemId);
                if (item) {
                    m_currentFolder = &m_folders[i];
                    m_currentItem = item;
                    
                    ui->tabWidget->setCurrentIndex(0);
                    show();
                    activateWindow();
                    raise();
                    
                    updateFolderList();
                    updateTodoList();
                    updateDetailPanel();
                    break;
                }
            }
        } catch (...) {
            MessageUtils::showError(this, "错误", "打开待办事项失败");
        }
    });
    connect(m_desktopWidget, &DesktopWidget::deleteTodoRequested, this, [this](const QString &itemId) {
        try {
            deleteTodoItem(itemId);
        } catch (...) {
            MessageUtils::showError(this, "错误", "删除待办事项失败");
        }
    });
}

void MainWindow::updateDesktopWidget()
{
    if (m_desktopWidget) {
        m_desktopWidget->updateTodoData(m_folders);
    }
}

void MainWindow::onDesktopNewTodo(const QString &title)
{
    TodoFolder* todayFolder = findOrCreateTodayFolder();
    
    TodoItem newItem(title);
    newItem.setPlannedDate(QDate::currentDate());
    todayFolder->addItem(newItem);
    
    saveData();
    updateFolderList();
    updateTodoList();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();
}

void MainWindow::onDesktopTodoToggled(const QString &itemId, bool completed)
{
    try {
        toggleTodoCompleted(itemId, completed);
    } catch (const std::exception &e) {
        MessageUtils::showError(this, "操作失败", QString("切换完成状态时发生错误: %1").arg(e.what()));
    } catch (...) {
        MessageUtils::showError(this, "操作失败", "切换完成状态时发生未知错误");
    }
}

void MainWindow::onShowMainWindow()
{
    show();
    activateWindow();
    raise();
}

void MainWindow::setupCalendarWidget()
{
    m_calendarWidget = new CalendarWidget(this);
    
    QVBoxLayout* calendarLayout = new QVBoxLayout(ui->calendarContainer);
    calendarLayout->setContentsMargins(0, 0, 0, 0);
    calendarLayout->addWidget(m_calendarWidget);
    
    connect(m_calendarWidget, &CalendarWidget::todoItemAdded, this, &MainWindow::onCalendarTodoAdded);
    connect(m_calendarWidget, &CalendarWidget::todoItemToggled, this, &MainWindow::onCalendarTodoToggled);
    connect(m_calendarWidget, &CalendarWidget::todoItemDeleted, this, &MainWindow::onCalendarTodoDeleted);
}

void MainWindow::updateCalendarWidget()
{
    if (m_calendarWidget) {
        m_calendarWidget->updateTodoData(m_folders);
    }
}

void MainWindow::onCalendarTodoAdded(const QString &title, const QDate &date)
{
    TodoItem todoItem(title);
    todoItem.setPlannedDate(date);
    
    QString dateFolderName = date.toString("yyyy-MM-dd");
    
    TodoFolder* dateFolder = nullptr;
    for (TodoFolder& folder : m_folders) {
        if (folder.getName() == dateFolderName) {
            dateFolder = &folder;
            break;
        }
    }
    
    if (!dateFolder) {
        TodoFolder newDateFolder(dateFolderName);
        m_folders.append(newDateFolder);
        dateFolder = &m_folders.last();
    }
    
    todoItem.setFolderId(dateFolder->getId());
    dateFolder->addItem(todoItem);
    
    saveData();
    updateFolderList();
    updateTodoList();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();
}

void MainWindow::onCalendarTodoToggled(const QString &itemId, bool completed)
{
    toggleTodoCompleted(itemId, completed);
}

void MainWindow::onCalendarTodoDeleted(const QString &itemId)
{
    deleteTodoItem(itemId);
}

void MainWindow::setupTagWidget()
{
    m_tagWidget = new TagWidget(this);
    
    QVBoxLayout* tagLayout = new QVBoxLayout(ui->tagContainer);
    tagLayout->setContentsMargins(0, 0, 0, 0);
    tagLayout->addWidget(m_tagWidget);
    
    connect(m_tagWidget, &TagWidget::tagSelected, this, &MainWindow::onTagSelected);
    connect(m_tagWidget, &TagWidget::todoClicked, [this](const QString &todoId, const QString &folderId) {
        for (int i = 0; i < m_folders.size(); ++i) {
            if (m_folders[i].getId() == folderId) {
                ui->folderListWidget->setCurrentRow(i);
                QList<TodoItem> items = m_folders[i].getItems();
                for (int j = 0; j < items.size(); ++j) {
                    if (items[j].getId() == todoId) {
                        ui->todoListWidget->setCurrentRow(j);
                        break;
                    }
                }
                break;
            }
        }
        ui->tabWidget->setCurrentIndex(0);
    });
    connect(m_tagWidget, &TagWidget::todoToggled, [this](const QString &todoId, bool completed) {
        for (TodoFolder &folder : m_folders) {
            TodoItem *item = folder.findItem(todoId);
            if (item) {
                item->setCompleted(completed);
                saveData();
                updateTodoList();
                updateFolderList();
                updateCalendarWidget();
                updateTagWidget();
                updateDesktopWidget();
                break;
            }
        }
    });
    connect(m_tagWidget, &TagWidget::tagCreated, [this](const QString &tag) {
        Q_UNUSED(tag)
        updateTagWidget();
    });
    connect(m_tagWidget, &TagWidget::tagDeleted, [this](const QString &tag) {
        for (TodoFolder &folder : m_folders) {
            for (TodoItem &item : folder.getItems()) {
                item.removeTag(tag);
            }
        }
        saveData();
        updateTagWidget();
    });
}

void MainWindow::updateTagWidget()
{
    if (m_tagWidget) {
        m_tagWidget->updateData(m_folders);
    }
}

void MainWindow::refreshAllViews()
{
    updateFolderList();
    updateTodoList();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();
}

TodoItem* MainWindow::findTodoItemById(const QString &itemId, QString &outFolderId)
{
    for (int i = 0; i < m_folders.size(); ++i) {
        TodoItem* item = m_folders[i].findItem(itemId);
        if (item) {
            outFolderId = m_folders[i].getId();
            return item;
        }
    }
    return nullptr;
}

bool MainWindow::toggleTodoCompleted(const QString &itemId, bool completed)
{
    QString folderId;
    TodoItem* item = findTodoItemById(itemId, folderId);
    
    if (!item) {
        return false;
    }
    
    item->setCompleted(completed);
    saveData();
    
    if (m_currentItem && m_currentItem->getId() == itemId) {
        m_currentItem = item;
    }
    
    refreshAllViews();
    
    return true;
}

bool MainWindow::deleteTodoItem(const QString &itemId)
{
    for (int i = 0; i < m_folders.size(); ++i) {
        QList<TodoItem> items = m_folders[i].getItems();
        for (int j = 0; j < items.size(); ++j) {
            if (items[j].getId() == itemId) {
                m_folders[i].removeItem(itemId);
                
                if (m_currentItem && m_currentItem->getId() == itemId) {
                    m_currentItem = nullptr;
                }
                
                saveData();
                refreshAllViews();
                return true;
            }
        }
    }
    return false;
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

TodoFolder* MainWindow::findOrCreateTodayFolder()
{
    QString todayString = QDate::currentDate().toString("yyyy-MM-dd");
    
    for (TodoFolder &folder : m_folders) {
        if (folder.getName() == todayString) {
            return &folder;
        }
    }
    
    TodoFolder todayFolder(todayString);
    m_folders.append(todayFolder);
    
    updateFolderList();
    
    return &m_folders.last();
}

void MainWindow::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(this, "系统托盘", "系统不支持托盘图标功能。");
        return;
    }
    
    m_trayIcon = new QSystemTrayIcon(this);
    QIcon trayIcon(":/icons/app.ico");
    if (trayIcon.isNull()) {
        QString iconPath = QDir::currentPath() + "/icons/app.ico";
        trayIcon = QIcon(iconPath);
        if (trayIcon.isNull()) {
            trayIcon = QIcon("icons/app.ico");
            if (trayIcon.isNull()) {
                trayIcon = style()->standardIcon(QStyle::SP_ComputerIcon);
            }
        }
    }
    m_trayIcon->setIcon(trayIcon);
    m_trayIcon->setToolTip("Todo List - 待办事项管理");
    
    m_trayMenu = new QMenu(this);
    
    m_showAction = new QAction("显示主窗口", this);
    m_showAction->setIcon(trayIcon);
    
    m_exitAction = new QAction("退出", this);
    m_exitAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    
    m_trayMenu->addAction(m_showAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_exitAction);
    
    m_trayIcon->setContextMenu(m_trayMenu);
    
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    connect(m_showAction, &QAction::triggered, this, &MainWindow::onShowFromTray);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::onExitFromTray);
    
    m_trayIcon->show();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            show();
            activateWindow();
            raise();
            break;
        default:
            break;
    }
}

void MainWindow::onShowFromTray()
{
    show();
    activateWindow();
    raise();
}

void MainWindow::onExitFromTray()
{
    saveData();
    if (m_desktopWidget) {
        m_desktopWidget->close();
    }
    qApp->quit();
}

void MainWindow::onImportClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "导入数据", "", "JSON文件 (*.json)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "错误", "无法打开文件进行读取。");
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        QMessageBox::critical(this, "错误", "文件格式不正确。");
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray foldersArray = root["folders"].toArray();
    
    m_folders.clear();
    for (const QJsonValue &folderVal : foldersArray) {
        TodoFolder folder(folderVal.toObject());
        m_folders.append(folder);
    }
    
    saveData();
    updateFolderList();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();
    
    QMessageBox::information(this, "导入成功", "数据已成功导入！");
}

void MainWindow::onExportClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出数据", "", "JSON文件 (*.json)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    QJsonObject root;
    QJsonArray foldersArray;
    
    for (const TodoFolder &folder : m_folders) {
        foldersArray.append(folder.toJson());
    }
    
    root["folders"] = foldersArray;
    
    QJsonDocument doc(root);
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "错误", "无法创建文件进行写入。");
        return;
    }
    
    file.write(doc.toJson());
    file.close();
    
    QMessageBox::information(this, "导出成功", "数据已成功导出！");
}

void MainWindow::onExitClicked()
{
    saveData();
    if (m_desktopWidget) {
        m_desktopWidget->close();
    }
    qApp->quit();
}

void MainWindow::onDesktopWidgetClicked()
{
    if (m_desktopWidget) {
        m_desktopWidget->show();
        m_desktopWidget->activateWindow();
    } else {
        setupDesktopWidget();
        m_desktopWidget->show();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSplitterState();
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        saveData();
        event->accept();
    }
}

void MainWindow::setupSplitter()
{
    QWidget *listTab = ui->tabWidget->widget(0);
    QHBoxLayout *listTabLayout = qobject_cast<QHBoxLayout*>(listTab->layout());
    if (!listTabLayout) return;
    
    QWidget *leftPanel = ui->leftPanel;
    QWidget *middlePanel = ui->middlePanel;
    QWidget *rightPanel = ui->rightPanel;
    
    if (!leftPanel || !middlePanel || !rightPanel) return;
    
    listTabLayout->removeWidget(leftPanel);
    listTabLayout->removeWidget(middlePanel);
    listTabLayout->removeWidget(rightPanel);
    
    m_mainSplitter = new QSplitter(Qt::Horizontal, listTab);
    m_mainSplitter->setHandleWidth(1);
    m_mainSplitter->setChildrenCollapsible(false);
    
    m_mainSplitter->addWidget(leftPanel);
    m_mainSplitter->addWidget(middlePanel);
    m_mainSplitter->addWidget(rightPanel);
    
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setStretchFactor(2, 0);
    
    listTabLayout->addWidget(m_mainSplitter);
}

void MainWindow::saveSplitterState()
{
    if (!m_mainSplitter) return;
    
    QSettings settings("TodoList", "TodoListApp");
    settings.setValue("splitter/state", m_mainSplitter->saveState());
}

void MainWindow::loadSplitterState()
{
    if (!m_mainSplitter) return;
    
    QSettings settings("TodoList", "TodoListApp");
    
    if (settings.contains("splitter/state")) {
        m_mainSplitter->restoreState(settings.value("splitter/state").toByteArray());
    } else {
        QList<int> defaultSizes;
        defaultSizes << 240 << 500 << 320;
        m_mainSplitter->setSizes(defaultSizes);
    }
}
