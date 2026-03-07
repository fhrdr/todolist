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
#include <QDialogButtonBox>
#include <QDialog>
#include <QLabel>
#include <QPushButton>

namespace {
    QString showInputDialog(QWidget *parent, const QString &title, const QString &label, QString defaultValue = "")
    {
        QDialog dialog(parent);
        dialog.setWindowTitle(title);
        dialog.setMinimumWidth(350);
        dialog.setStyleSheet(R"(
            QDialog {
                background-color: #ffffff;
            }
            QLabel {
                color: #334155;
                font-size: 13px;
            }
            QLineEdit {
                border: 1px solid #e2e8f0;
                border-radius: 6px;
                padding: 8px 12px;
                background-color: #ffffff;
                color: #334155;
                font-size: 13px;
            }
            QLineEdit:focus {
                border-color: #3b82f6;
            }
            QPushButton {
                background-color: #ffffff;
                border: 2px solid #3b82f6;
                border-radius: 6px;
                color: #3b82f6;
                padding: 8px 20px;
                font-size: 13px;
                font-weight: 500;
                min-width: 70px;
            }
            QPushButton:hover {
                background-color: rgba(59, 130, 246, 0.1);
            }
            QPushButton:pressed {
                background-color: rgba(59, 130, 246, 0.2);
            }
        )");
        
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(16);
        
        QLabel *labelWidget = new QLabel(label);
        layout->addWidget(labelWidget);
        
        QLineEdit *lineEdit = new QLineEdit(defaultValue);
        layout->addWidget(lineEdit);
        
        QDialogButtonBox *buttonBox = new QDialogButtonBox();
        QPushButton *okBtn = new QPushButton("确定");
        QPushButton *cancelBtn = new QPushButton("取消");
        okBtn->setDefault(true);
        buttonBox->addButton(okBtn, QDialogButtonBox::AcceptRole);
        buttonBox->addButton(cancelBtn, QDialogButtonBox::RejectRole);
        layout->addWidget(buttonBox);
        
        QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
        QObject::connect(lineEdit, &QLineEdit::returnPressed, &dialog, &QDialog::accept);
        
        lineEdit->setFocus();
        
        if (dialog.exec() == QDialog::Accepted) {
            return lineEdit->text();
        }
        return QString();
    }
    
    QString showItemDialog(QWidget *parent, const QString &title, const QString &label, const QStringList &items, int current = 0, bool editable = true)
    {
        QDialog dialog(parent);
        dialog.setWindowTitle(title);
        dialog.setMinimumWidth(350);
        dialog.setStyleSheet(R"(
            QDialog {
                background-color: #ffffff;
            }
            QLabel {
                color: #334155;
                font-size: 13px;
            }
            QComboBox {
                border: 1px solid #e2e8f0;
                border-radius: 6px;
                padding: 8px 30px 8px 12px;
                background-color: #ffffff;
                color: #334155;
            }
            QComboBox::drop-down {
                border: none;
                width: 24px;
            }
            QPushButton {
                background-color: #ffffff;
                border: 2px solid #3b82f6;
                border-radius: 6px;
                color: #3b82f6;
                padding: 8px 20px;
                font-size: 13px;
                font-weight: 500;
                min-width: 70px;
            }
            QPushButton:hover {
                background-color: rgba(59, 130, 246, 0.1);
            }
        )");
        
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(16);
        
        QLabel *labelWidget = new QLabel(label);
        layout->addWidget(labelWidget);
        
        QComboBox *comboBox = new QComboBox();
        comboBox->addItems(items);
        comboBox->setCurrentIndex(current);
        comboBox->setEditable(editable);
        layout->addWidget(comboBox);
        
        QDialogButtonBox *buttonBox = new QDialogButtonBox();
        QPushButton *okBtn = new QPushButton("确定");
        QPushButton *cancelBtn = new QPushButton("取消");
        okBtn->setDefault(true);
        buttonBox->addButton(okBtn, QDialogButtonBox::AcceptRole);
        buttonBox->addButton(cancelBtn, QDialogButtonBox::RejectRole);
        layout->addWidget(buttonBox);
        
        QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
        
        comboBox->setFocus();
        
        if (dialog.exec() == QDialog::Accepted) {
            return comboBox->currentText();
        }
        return QString();
    }
}

class TodoItemDelegate : public QStyledItemDelegate
{
public:
    TodoItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        
        bool isSelected = option.state & QStyle::State_Selected;
        bool isCompleted = index.data(Qt::ForegroundRole).isValid() && 
                          index.data(Qt::ForegroundRole).value<QColor>() == QColor(156, 163, 175);
        QString tagColor = index.data(Qt::UserRole + 1).toString();
        
        QRect contentRect = option.rect.adjusted(4, 2, -4, -2);
        
        painter->setPen(Qt::NoPen);
        
        if (isCompleted) {
            if (!tagColor.isEmpty()) {
                QColor baseColor(tagColor);
                QColor lightColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 25);
                QColor whiteColor = QColor(255, 255, 255, 255);
                QLinearGradient gradient(contentRect.right(), contentRect.top(), 
                                         contentRect.left(), contentRect.top());
                gradient.setColorAt(0, lightColor);
                gradient.setColorAt(1, whiteColor);
                painter->setBrush(gradient);
                painter->drawRoundedRect(contentRect, 8, 8);
            } else {
                painter->setBrush(QColor(252, 252, 253));
                painter->drawRoundedRect(contentRect, 8, 8);
            }
        } else if (!tagColor.isEmpty()) {
            QColor baseColor(tagColor);
            QColor lightColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(), 40);
            QColor whiteColor = QColor(255, 255, 255, 255);
            QLinearGradient gradient(contentRect.right(), contentRect.top(), 
                                     contentRect.left(), contentRect.top());
            gradient.setColorAt(0, lightColor);
            gradient.setColorAt(1, whiteColor);
            painter->setBrush(gradient);
            painter->drawRoundedRect(contentRect, 8, 8);
        } else if (isSelected) {
            painter->setBrush(QColor(239, 246, 255));
            painter->drawRoundedRect(contentRect, 8, 8);
        } else {
            painter->setBrush(QColor(255, 255, 255));
            painter->drawRoundedRect(contentRect, 8, 8);
        }
        
        if (isCompleted) {
            QColor checkBgColor = tagColor.isEmpty() ? QColor(200, 210, 220) : QColor(tagColor);
            checkBgColor = QColor(checkBgColor.red(), checkBgColor.green(), checkBgColor.blue(), 150);
            
            painter->setPen(Qt::NoPen);
            painter->setBrush(checkBgColor);
            QRect checkRect(contentRect.left() + 8, contentRect.top() + (contentRect.height() - 16) / 2, 16, 16);
            painter->drawRoundedRect(checkRect, 3, 3);
            
            QPainterPath checkPath;
            checkPath.moveTo(checkRect.left() + 3, checkRect.center().y());
            checkPath.lineTo(checkRect.center().x(), checkRect.bottom() - 3);
            checkPath.lineTo(checkRect.right() - 3, checkRect.top() + 3);
            painter->setPen(QPen(QColor(255, 255, 255), 2));
            painter->drawPath(checkPath);
        } else {
            painter->setPen(QPen(QColor(203, 213, 225), 2));
            painter->setBrush(Qt::NoBrush);
            QRect checkRect(contentRect.left() + 8, contentRect.top() + (contentRect.height() - 16) / 2, 16, 16);
            painter->drawRoundedRect(checkRect, 3, 3);
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
        
        QRect rect = contentRect.adjusted(32, 6, -8, -6);
        
        QFont titleFont;
        titleFont.setPixelSize(14);
        titleFont.setBold(!isCompleted);
        painter->setFont(titleFont);
        
        QColor titleColor = isCompleted ? QColor(180, 185, 190) : QColor(30, 41, 59);
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
            painter->setPen(isCompleted ? QColor(156, 163, 175) : QColor(100, 116, 139));
            painter->drawText(QRect(rect.right() - dateWidth, rect.top(), dateWidth, 22), Qt::AlignRight | Qt::AlignVCenter, date);
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
        return QSize(200, 56);
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
    
    connect(ui->tagColorComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        QStringList colors = {"#3b82f6", "#10b981", "#ef4444", "#f59e0b", "#8b5cf6", "#06b6d4"};
        QString color = (index >= 0 && index < colors.size()) ? colors[index] : "#ffffff";
        ui->tagColorComboBox->setStyleSheet(
            QString("QComboBox { background-color: %1; border: 1px solid #e2e8f0; border-radius: 6px; "
                    "padding: 8px 12px; color: %2; }"
                    "QComboBox:hover { border-color: #94a3b8; }"
                    "QComboBox::drop-down { border: none; width: 24px; }"
                    "QComboBox QAbstractItemView { background-color: #ffffff; color: #334155; }")
            .arg(color)
            .arg(index == 0 ? "#334155" : "#ffffff")
        );
    });
    
    connect(ui->addTagBtn, &QPushButton::clicked, this, [this]() {
        if (!m_currentItem) return;
        
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
        
        QString newTag = showItemDialog(this, "添加标签", "选择或输入标签:", sortedTags, 0, true);
        
        if (!newTag.trimmed().isEmpty()) {
            m_currentItem->addTag(newTag.trimmed());
            updateTodoTags();
            updateTagWidget();
            saveData();
        }
    });
    
    connect(ui->actionImport, &QAction::triggered, this, &MainWindow::onImportClicked);
    connect(ui->actionExport, &QAction::triggered, this, &MainWindow::onExportClicked);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onExitClicked);
    connect(ui->actionDesktopWidget, &QAction::triggered, this, &MainWindow::onDesktopWidgetClicked);
}

void MainWindow::onNewFolderClicked()
{
    try {
        QString folderName = showInputDialog(this, "新建文件夹", "请输入文件夹名称:", "新建文件夹");
        
        if (!folderName.isEmpty()) {
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
    QListWidgetItem* selectedItem = ui->folderListWidget->currentItem();
    if (selectedItem) {
        QString folderId = selectedItem->data(Qt::UserRole).toString();
        TodoFolder* foundFolder = nullptr;
        for (int i = 0; i < m_folders.size(); ++i) {
            if (m_folders[i].getId() == folderId) {
                foundFolder = &m_folders[i];
                break;
            }
        }
        if (foundFolder) {
            m_currentFolder = foundFolder;
            ui->deleteFolderBtn->setEnabled(true);
            updateTodoList();
            updateDetailPanel();
        } else {
            m_currentFolder = nullptr;
            ui->deleteFolderBtn->setEnabled(false);
            ui->todoListWidget->blockSignals(true);
            ui->todoListWidget->clear();
            ui->todoListWidget->blockSignals(false);
            clearDetailPanel();
        }
    } else {
        m_currentFolder = nullptr;
        ui->deleteFolderBtn->setEnabled(false);
        ui->todoListWidget->blockSignals(true);
        ui->todoListWidget->clear();
        ui->todoListWidget->blockSignals(false);
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
            
            ui->todoListWidget->blockSignals(true);
            ui->todoListWidget->clear();
            ui->todoListWidget->blockSignals(false);
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
    if (!item) return;
    
    QString folderId = item->data(Qt::UserRole).toString();
    TodoFolder* foundFolder = nullptr;
    for (int i = 0; i < m_folders.size(); ++i) {
        if (m_folders[i].getId() == folderId) {
            foundFolder = &m_folders[i];
            break;
        }
    }
    
    if (foundFolder) {
        QString newName = showInputDialog(this, "重命名文件夹", "请输入新的文件夹名称:", foundFolder->getName());
        
        if (!newName.isEmpty()) {
            foundFolder->setName(newName);
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
    
    QString folderId = item->data(Qt::UserRole).toString();
    TodoFolder* foundFolder = nullptr;
    for (int i = 0; i < m_folders.size(); ++i) {
        if (m_folders[i].getId() == folderId) {
            foundFolder = &m_folders[i];
            break;
        }
    }
    if (!foundFolder) return;
    
    m_currentFolder = foundFolder;
    
    QMenu menu(this);
    QAction *pinAction = menu.addAction(m_currentFolder->isPinned() ? "取消置顶" : "置顶");
    QAction *renameAction = menu.addAction("重命名");
    
    QMenu *colorMenu = menu.addMenu("设置颜色");
    QStringList colorNames = {"蓝色", "绿色", "红色", "橙色", "紫色", "青色"};
    QStringList colorValues = {"#3b82f6", "#10b981", "#ef4444", "#f59e0b", "#8b5cf6", "#06b6d4"};
    for (int i = 0; i < colorNames.size(); ++i) {
        QAction *colorAction = colorMenu->addAction(colorNames[i]);
        connect(colorAction, &QAction::triggered, this, [this, colorValues, i]() {
            if (m_currentFolder) {
                m_currentFolder->setColor(colorValues[i]);
                updateFolderList();
                saveData();
            }
        });
    }
    
    menu.addSeparator();
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
        
        QString title = showInputDialog(this, "新建待办事项", "请输入待办事项标题:");
        
        if (!title.isEmpty()) {
            TodoItem newItem(title);
            newItem.setPlannedDate(QDate::currentDate());
            newItem.setDueDate(QDate::currentDate());
            QString newItemId = newItem.getId();
            m_currentFolder->addItem(newItem);
            
            saveData();
            updateTodoList();
            updateFolderList();
            updateCalendarWidget();
            updateTagWidget();
            updateDesktopWidget();
            
            for (int i = 0; i < ui->todoListWidget->count(); ++i) {
                if (ui->todoListWidget->item(i)->data(Qt::UserRole).toString() == newItemId) {
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
    
    QListWidgetItem* currentItem = ui->todoListWidget->currentItem();
    if (!currentItem) {
        m_currentItem = nullptr;
        clearDetailPanel();
        return;
    }
    
    QString itemId = currentItem->data(Qt::UserRole).toString();
    m_currentItem = m_currentFolder->findItem(itemId);
    
    if (m_currentItem) {
        updateDetailPanel();
    } else {
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
    
    QString itemId = item->data(Qt::UserRole).toString();
    m_currentItem = m_currentFolder->findItem(itemId);
    
    if (!m_currentItem) return;
    
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
    
    if (ui->priorityComboBox) {
        m_currentItem->setPriority(ui->priorityComboBox->currentIndex());
    }
    if (ui->tagColorComboBox) {
        QStringList colors = {"#3b82f6", "#10b981", "#ef4444", "#f59e0b", "#8b5cf6", "#06b6d4"};
        int index = ui->tagColorComboBox->currentIndex();
        if (index >= 0 && index < colors.size()) {
            m_currentItem->setTagColor(colors[index]);
        }
    }
    
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
    
    if (MessageUtils::showConfirm(this, "确认删除", 
        QString("确定要删除待办事项 \"%1\" 吗？").arg(m_currentItem->getTitle()))) {
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
    if (!m_currentItem) return;
    
    QString currentItemId = m_currentItem->getId();
    QString currentFolderId = m_currentFolder ? m_currentFolder->getId() : "";
    
    m_currentItem->setCompleted(completed);
    saveData();
    
    updateFolderList();
    updateTodoList();
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();
    
    if (m_currentItem) {
        updateDetailPanel();
        updateTodoTags();
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
    QListWidgetItem* selectedFolderItem = ui->folderListWidget->currentItem();
    if (selectedFolderItem) {
        currentFolderId = selectedFolderItem->data(Qt::UserRole).toString();
    }
    
    QString currentItemId;
    QListWidgetItem* selectedTodoItem = ui->todoListWidget->currentItem();
    if (selectedTodoItem) {
        currentItemId = selectedTodoItem->data(Qt::UserRole).toString();
    }
    
    ui->folderListWidget->blockSignals(true);
    
    m_currentFolder = nullptr;
    m_currentItem = nullptr;
    
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
        
        QString folderColor = folder.getColor();
        if (!folderColor.isEmpty()) {
            QColor color(folderColor);
            item->setForeground(color);
        }
        
        ui->folderListWidget->addItem(item);
        
        if (folder.getId() == currentFolderId) {
            selectRow = i;
            m_currentFolder = &m_folders[i];
            if (!currentItemId.isEmpty()) {
                m_currentItem = m_currentFolder->findItem(currentItemId);
            }
        }
    }
    
    if (selectRow >= 0) {
        ui->folderListWidget->setCurrentRow(selectRow);
    }
    
    ui->folderListWidget->blockSignals(false);
}

void MainWindow::updateTodoList()
{
    QString currentItemId;
    QListWidgetItem* selectedTodoItem = ui->todoListWidget->currentItem();
    if (selectedTodoItem) {
        currentItemId = selectedTodoItem->data(Qt::UserRole).toString();
    }
    
    ui->todoListWidget->blockSignals(true);
    
    ui->todoListWidget->clear();
    
    if (!m_currentFolder) {
        ui->todoListWidget->blockSignals(false);
        return;
    }
    
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
        
        QString priorityIcon;
        int priority = item.getPriority();
        if (priority == 2) {
            priorityIcon = "🔴 ";
        } else if (priority == 1) {
            priorityIcon = "🟡 ";
        }
        
        QString createdDate = item.getCreatedTime().toString("MM-dd");
        displayText = priorityIcon + displayText + item.getTitle() + "\t" + createdDate;
        
        QString subText;
        if (!item.getDetails().isEmpty()) {
            subText = item.getDetails().left(30);
            if (item.getDetails().length() > 30) subText += "...";
        } else {
            subText = QString::fromUtf8("还没有写任何内容呢~");
        }
        
        QString fullText = displayText + "\n" + subText;
        
        QListWidgetItem *listItem = new QListWidgetItem();
        listItem->setText(fullText);
        
        if (item.isCompleted()) {
            listItem->setForeground(QColor(156, 163, 175));
        }
        
        listItem->setData(Qt::UserRole, item.getId());
        listItem->setData(Qt::UserRole + 1, item.getTagColor());
        ui->todoListWidget->addItem(listItem);
        
        if (item.getId() == currentItemId) {
            selectRow = i;
            m_currentItem = m_currentFolder->findItem(item.getId());
        }
    }
    
    if (selectRow >= 0) {
        ui->todoListWidget->setCurrentRow(selectRow);
    }
    
    ui->todoListWidget->blockSignals(false);
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
    
    if (ui->emptyStateLabel) {
        ui->emptyStateLabel->setVisible(false);
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
    
    if (ui->priorityComboBox) {
        int priority = qBound(0, m_currentItem->getPriority(), 2);
        ui->priorityComboBox->setCurrentIndex(priority);
        ui->priorityComboBox->setEnabled(true);
    }
    
    if (ui->tagColorComboBox) {
        QString tagColor = m_currentItem->getTagColor();
        int colorIndex = 0;
        if (tagColor == "#10b981") colorIndex = 1;
        else if (tagColor == "#ef4444") colorIndex = 2;
        else if (tagColor == "#f59e0b") colorIndex = 3;
        else if (tagColor == "#8b5cf6") colorIndex = 4;
        else if (tagColor == "#06b6d4") colorIndex = 5;
        ui->tagColorComboBox->setCurrentIndex(colorIndex);
        ui->tagColorComboBox->setEnabled(true);
        
        QString bgColor = tagColor.isEmpty() ? "#ffffff" : tagColor;
        ui->tagColorComboBox->setStyleSheet(
            QString("QComboBox { background-color: %1; border: 1px solid #e2e8f0; border-radius: 6px; "
                    "padding: 8px 12px; color: %2; }"
                    "QComboBox:hover { border-color: #94a3b8; }"
                    "QComboBox::drop-down { border: none; width: 24px; }"
                    "QComboBox QAbstractItemView { background-color: #ffffff; color: #334155; }")
            .arg(bgColor)
            .arg(tagColor.isEmpty() ? "#334155" : "#ffffff")
        );
    }
    
    updateTodoTags();
    
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
    
    if (ui->emptyStateLabel) {
        ui->emptyStateLabel->setVisible(true);
    }
    
    ui->titleEdit->clear();
    ui->detailsEdit->clear();
    ui->createdTimeLabel->setText("-");
    ui->completedTimeLabel->setText("-");
    ui->completedTimeLabel_title->setVisible(false);
    ui->completedTimeLabel->setVisible(false);
    ui->completedCheckBox->setChecked(false);
    
    if (ui->priorityComboBox) {
        ui->priorityComboBox->setCurrentIndex(0);
        ui->priorityComboBox->setEnabled(false);
    }
    if (ui->tagColorComboBox) {
        ui->tagColorComboBox->setCurrentIndex(0);
        ui->tagColorComboBox->setEnabled(false);
        ui->tagColorComboBox->setStyleSheet(
            "QComboBox { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 6px; "
            "padding: 8px 12px; color: #334155; }"
            "QComboBox::drop-down { border: none; width: 24px; }"
            "QComboBox QAbstractItemView { background-color: #ffffff; color: #334155; }"
        );
    }
    if (ui->tagsDisplayLabel) {
        ui->tagsDisplayLabel->setText("无标签");
    }
    if (ui->addTagBtn) {
        ui->addTagBtn->setEnabled(false);
    }
    
    ui->titleEdit->setEnabled(false);
    ui->detailsEdit->setEnabled(false);
    ui->completedCheckBox->setEnabled(false);
    ui->saveBtn->setEnabled(false);
    ui->deleteBtn->setEnabled(false);
}

void MainWindow::updateTodoTags()
{
    if (!m_currentItem || !ui->tagsDisplayLabel || !ui->addTagBtn) {
        return;
    }
    
    QStringList tags = m_currentItem->getTags();
    if (tags.isEmpty()) {
        ui->tagsDisplayLabel->setText("无标签");
        ui->tagsDisplayLabel->setStyleSheet("color: #94a3b8; font-size: 12px;");
    } else {
        QString tagsText = tags.join(", ");
        ui->tagsDisplayLabel->setText(tagsText);
        ui->tagsDisplayLabel->setStyleSheet("color: #3b82f6; font-size: 12px;");
    }
    ui->addTagBtn->setEnabled(true);
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
    todoItem.setDueDate(date);
    
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
                m_currentFolder = &m_folders[i];
                ui->folderListWidget->setCurrentRow(i);
                TodoItem* item = m_currentFolder->findItem(todoId);
                if (item) {
                    m_currentItem = item;
                    updateTodoList();
                    for (int j = 0; j < ui->todoListWidget->count(); ++j) {
                        if (ui->todoListWidget->item(j)->data(Qt::UserRole).toString() == todoId) {
                            ui->todoListWidget->setCurrentRow(j);
                            break;
                        }
                    }
                    updateDetailPanel();
                }
                break;
            }
        }
        ui->tabWidget->setCurrentIndex(0);
    });
    connect(m_tagWidget, &TagWidget::todoToggled, [this](const QString &todoId, bool completed) {
        toggleTodoCompleted(todoId, completed);
    });
    connect(m_tagWidget, &TagWidget::tagCreated, [this](const QString &tag) {
        QSqlQuery query;
        query.prepare("INSERT OR IGNORE INTO tags (id, name, color) VALUES (?, ?, ?)");
        query.addBindValue(QUuid::createUuid().toString(QUuid::WithoutBraces));
        query.addBindValue(tag);
        query.addBindValue("#3b82f6");
        if (query.exec()) {
            updateTagWidget();
        }
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
    
    bool wasCurrentItem = (m_currentItem && m_currentItem->getId() == itemId);
    QString currentFolderId = m_currentFolder ? m_currentFolder->getId() : "";
    QString currentItemId = m_currentItem ? m_currentItem->getId() : "";
    
    m_currentItem = nullptr;
    m_currentFolder = nullptr;
    
    updateFolderList();
    
    if (!currentFolderId.isEmpty()) {
        for (int i = 0; i < m_folders.size(); ++i) {
            if (m_folders[i].getId() == currentFolderId) {
                m_currentFolder = &m_folders[i];
                ui->folderListWidget->setCurrentRow(i);
                break;
            }
        }
    }
    
    updateTodoList();
    
    if (m_currentFolder && !currentItemId.isEmpty()) {
        for (int j = 0; j < ui->todoListWidget->count(); ++j) {
            if (ui->todoListWidget->item(j)->data(Qt::UserRole).toString() == currentItemId) {
                ui->todoListWidget->setCurrentRow(j);
                m_currentItem = m_currentFolder->findItem(currentItemId);
                break;
            }
        }
    }
    
    updateCalendarWidget();
    updateTagWidget();
    updateDesktopWidget();
    
    if (wasCurrentItem && m_currentItem) {
        updateDetailPanel();
    }
    
    return true;
}

bool MainWindow::deleteTodoItem(const QString &itemId)
{
    for (int i = 0; i < m_folders.size(); ++i) {
        TodoItem* item = m_folders[i].findItem(itemId);
        if (item) {
            m_folders[i].removeItem(itemId);
            
            if (m_currentItem && m_currentItem->getId() == itemId) {
                m_currentItem = nullptr;
            }
            
            saveData();
            refreshAllViews();
            return true;
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
