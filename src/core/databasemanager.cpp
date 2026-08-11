#include "databasemanager.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QDebug>
#include <QSet>

namespace {
constexpr int kMaxBackups = 12;
}

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

bool DatabaseManager::isOpen() const
{
    return m_open && m_db.isOpen();
}

bool DatabaseManager::initialize()
{
    // 标准应用数据目录（避免放在 exe 同级目录，只读权限下会静默写失败）
    QString dataRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataRoot.isEmpty()) {
        dataRoot = QCoreApplication::applicationDirPath() + "/appdata";
    }
    QDir dir(dataRoot + "/data");
    if (!dir.exists() && !dir.mkpath(".")) {
        m_lastError = QStringLiteral("无法创建数据目录: %1").arg(dir.absolutePath());
        return false;
    }

    m_dbPath = dir.absoluteFilePath(QStringLiteral("todolist.db"));

    migrateLegacyDatabase();

    if (!openDatabase()) {
        return false;
    }
    if (!createSchema()) {
        return false;
    }
    if (!upgradeSchema()) {
        return false;
    }

    migrateFromJson();

    // 回收站滚动清理（30 天）
    purgeExpiredDeleted();

    // 启动备份（保证任何一次启动前的数据都有快照）
    backupNow(QStringLiteral("startup"));
    m_open = true;
    return true;
}

bool DatabaseManager::openDatabase()
{
    if (QSqlDatabase::contains(QStringLiteral("qt_sql_default_connection"))) {
        m_db = QSqlDatabase::database(QStringLiteral("qt_sql_default_connection"));
    } else {
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    }
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        m_lastError = QStringLiteral("无法打开数据库: %1").arg(m_db.lastError().text());
        return false;
    }

    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    pragma.exec(QStringLiteral("PRAGMA busy_timeout=3000"));
    return true;
}

bool DatabaseManager::createSchema()
{
    QSqlQuery query(m_db);

    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS folders ("
            "id TEXT PRIMARY KEY, "
            "name TEXT NOT NULL, "
            "createdTime TEXT, "
            "isPinned INTEGER DEFAULT 0, "
            "color TEXT DEFAULT '#3b82f6')"))) {
        m_lastError = QStringLiteral("创建文件夹表失败: %1").arg(query.lastError().text());
        return false;
    }

    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS items ("
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
            "tagColor TEXT DEFAULT '#3b82f6', "
            "isPinned INTEGER DEFAULT 0)"))) {
        m_lastError = QStringLiteral("创建事项表失败: %1").arg(query.lastError().text());
        return false;
    }

    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS tags ("
            "id TEXT PRIMARY KEY, "
            "name TEXT NOT NULL UNIQUE, "
            "color TEXT DEFAULT '#3b82f6')"))) {
        m_lastError = QStringLiteral("创建标签表失败: %1").arg(query.lastError().text());
        return false;
    }

    if (!query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS item_tags ("
            "itemId TEXT, "
            "tagId TEXT, "
            "PRIMARY KEY(itemId, tagId))"))) {
        m_lastError = QStringLiteral("创建标签关联表失败: %1").arg(query.lastError().text());
        return false;
    }

    return true;
}

bool DatabaseManager::upgradeSchema()
{
    // 列级增量迁移：老库缺少的列用 ALTER TABLE 补上，不动已有数据
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("PRAGMA table_info(items)"))) {
        m_lastError = QStringLiteral("读取表结构失败: %1").arg(query.lastError().text());
        return false;
    }
    QSet<QString> columns;
    while (query.next()) {
        columns.insert(query.value(1).toString());
    }

    const QList<QPair<QString, QString>> additions = {
        {QStringLiteral("remindAt"),    QStringLiteral("TEXT")},
        {QStringLiteral("deletedTime"), QStringLiteral("TEXT")},
    };
    for (const auto &col : additions) {
        if (columns.contains(col.first)) {
            continue;
        }
        QSqlQuery alter(m_db);
        if (!alter.exec(QStringLiteral("ALTER TABLE items ADD COLUMN %1 %2").arg(col.first, col.second))) {
            m_lastError = QStringLiteral("迁移列 %1 失败: %2").arg(col.first, alter.lastError().text());
            return false;
        }
    }
    return true;
}

void DatabaseManager::migrateLegacyDatabase()
{
    // 旧版本数据库位于 exe 同级 data/ 目录；迁移到标准应用数据目录
    QString legacyDir = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
    QString legacyDb = legacyDir + QStringLiteral("/todolist.db");

    if (!QFile::exists(legacyDb) || QFile::exists(m_dbPath)) {
        return;
    }

    if (QFile::copy(legacyDb, m_dbPath)) {
        // 保留旧文件作为一次性快照，重命名而不是删除，防止误丢数据
        QString snapshot = legacyDb + QStringLiteral(".migrated-%1.bak")
                .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
        QFile::rename(legacyDb, snapshot);
    }
}

void DatabaseManager::migrateFromJson()
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString jsonPath = dataPath + QStringLiteral("/todolist.json");

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

    QJsonArray foldersArray = doc.object()[QStringLiteral("folders")].toArray();

    if (m_db.transaction()) {
        QSqlQuery query(m_db);
        for (const QJsonValue &folderVal : foldersArray) {
            QJsonObject folderObj = folderVal.toObject();

            query.prepare(QStringLiteral("INSERT OR REPLACE INTO folders (id, name, createdTime, isPinned, color) VALUES (?, ?, ?, ?, ?)"));
            query.addBindValue(folderObj[QStringLiteral("id")].toString());
            query.addBindValue(folderObj[QStringLiteral("name")].toString());
            query.addBindValue(folderObj[QStringLiteral("createdTime")].toString());
            query.addBindValue(folderObj[QStringLiteral("isPinned")].toBool() ? 1 : 0);
            query.addBindValue(folderObj[QStringLiteral("color")].toString(QStringLiteral("#3b82f6")));
            query.exec();

            QJsonArray itemsArray = folderObj[QStringLiteral("items")].toArray();
            for (const QJsonValue &itemVal : itemsArray) {
                QJsonObject itemObj = itemVal.toObject();
                query.prepare(QStringLiteral("INSERT OR REPLACE INTO items (id, title, details, createdTime, completedTime, updatedTime, isCompleted, folderId, plannedDate, dueDate, priority, tagColor, isPinned) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
                query.addBindValue(itemObj[QStringLiteral("id")].toString());
                query.addBindValue(itemObj[QStringLiteral("title")].toString());
                query.addBindValue(itemObj[QStringLiteral("details")].toString());
                query.addBindValue(itemObj[QStringLiteral("createdTime")].toString());
                query.addBindValue(itemObj[QStringLiteral("completedTime")].toString());
                query.addBindValue(itemObj[QStringLiteral("updatedTime")].toString());
                query.addBindValue(itemObj[QStringLiteral("isCompleted")].toBool() ? 1 : 0);
                query.addBindValue(folderObj[QStringLiteral("id")].toString());
                query.addBindValue(itemObj[QStringLiteral("plannedDate")].toString());
                query.addBindValue(itemObj[QStringLiteral("dueDate")].toString());
                query.addBindValue(itemObj[QStringLiteral("priority")].toInt(0));
                query.addBindValue(itemObj[QStringLiteral("tagColor")].toString(QStringLiteral("#3b82f6")));
                query.addBindValue(itemObj[QStringLiteral("isPinned")].toBool() ? 1 : 0);
                query.exec();
            }
        }
        m_db.commit();
    }

    QString backupPath = jsonPath + QStringLiteral(".migrated.bak");
    QFile::remove(backupPath);
    QFile::rename(jsonPath, backupPath);
}

QList<TodoFolder> DatabaseManager::loadAll()
{
    QList<TodoFolder> folders;
    if (!isOpen()) {
        return folders;
    }

    QSqlQuery folderQuery(m_db);
    if (!folderQuery.exec(QStringLiteral("SELECT id, name, createdTime, isPinned, color FROM folders ORDER BY isPinned DESC, createdTime DESC"))) {
        m_lastError = folderQuery.lastError().text();
        return folders;
    }

    QSqlQuery itemQuery(m_db);
    QSqlQuery tagQuery(m_db);

    while (folderQuery.next()) {
        TodoFolder folder;
        folder.setId(folderQuery.value(0).toString());
        folder.setName(folderQuery.value(1).toString());
        folder.setCreatedTime(QDateTime::fromString(folderQuery.value(2).toString(), Qt::ISODate));
        folder.setPinned(folderQuery.value(3).toInt() == 1);
        folder.setColor(folderQuery.value(4).toString());

        itemQuery.prepare(QStringLiteral("SELECT id, title, details, createdTime, completedTime, updatedTime, isCompleted, folderId, plannedDate, dueDate, priority, tagColor, isPinned, remindAt FROM items WHERE folderId = ? AND deletedTime IS NULL ORDER BY isPinned DESC, createdTime DESC"));
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
            item.setRemindAt(QDateTime::fromString(itemQuery.value(13).toString(), Qt::ISODate));

            tagQuery.prepare(QStringLiteral("SELECT t.name FROM tags t JOIN item_tags it ON t.id = it.tagId WHERE it.itemId = ?"));
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

        folders.append(folder);
    }

    return folders;
}

bool DatabaseManager::upsertFolder(const TodoFolder &folder)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO folders (id, name, createdTime, isPinned, color) VALUES (?, ?, ?, ?, ?)"));
    query.addBindValue(folder.getId());
    query.addBindValue(folder.getName());
    query.addBindValue(folder.getCreatedTime().toString(Qt::ISODate));
    query.addBindValue(folder.isPinned() ? 1 : 0);
    query.addBindValue(folder.getColor());
    return execChecked(query, QStringLiteral("保存文件夹"));
}

bool DatabaseManager::deleteFolder(const QString &folderId)
{
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("无法开启事务");
        return false;
    }

    QSqlQuery query(m_db);

    // 文件夹下的事项软删除进回收站（保留 30 天），而不是直接抹掉
    query.prepare(QStringLiteral("UPDATE items SET deletedTime = ? WHERE folderId = ? AND deletedTime IS NULL"));
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(folderId);
    if (!execChecked(query, QStringLiteral("文件夹事项移入回收站"))) { m_db.rollback(); return false; }

    query.prepare(QStringLiteral("DELETE FROM folders WHERE id = ?"));
    query.addBindValue(folderId);
    if (!execChecked(query, QStringLiteral("删除文件夹"))) { m_db.rollback(); return false; }

    if (!m_db.commit()) {
        m_lastError = QStringLiteral("提交失败");
        m_db.rollback();
        return false;
    }
    return true;
}

bool DatabaseManager::upsertItem(const TodoItem &item)
{
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("无法开启事务");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT OR REPLACE INTO items (id, title, details, createdTime, completedTime, updatedTime, isCompleted, folderId, plannedDate, dueDate, priority, tagColor, isPinned, remindAt) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(item.getId());
    query.addBindValue(item.getTitle());
    query.addBindValue(item.getDetails());
    query.addBindValue(item.getCreatedTime().toString(Qt::ISODate));
    query.addBindValue(item.getCompletedTime().toString(Qt::ISODate));
    query.addBindValue(item.getUpdatedTime().toString(Qt::ISODate));
    query.addBindValue(item.isCompleted() ? 1 : 0);
    query.addBindValue(item.getFolderId());
    query.addBindValue(item.getPlannedDate().toString(Qt::ISODate));
    query.addBindValue(item.getDueDate().toString(Qt::ISODate));
    query.addBindValue(item.getPriority());
    query.addBindValue(item.getTagColor());
    query.addBindValue(item.isPinned() ? 1 : 0);
    query.addBindValue(item.getRemindAt().isValid() ? item.getRemindAt().toString(Qt::ISODate) : QVariant());
    if (!execChecked(query, QStringLiteral("保存事项"))) { m_db.rollback(); return false; }

    // 同步标签关联
    query.prepare(QStringLiteral("DELETE FROM item_tags WHERE itemId = ?"));
    query.addBindValue(item.getId());
    if (!execChecked(query, QStringLiteral("更新事项标签"))) { m_db.rollback(); return false; }

    for (const QString &tag : item.getTags()) {
        query.prepare(QStringLiteral("INSERT OR IGNORE INTO tags (id, name) VALUES (?, ?)"));
        query.addBindValue(QUuid::createUuid().toString(QUuid::WithoutBraces));
        query.addBindValue(tag);
        if (!execChecked(query, QStringLiteral("保存标签"))) { m_db.rollback(); return false; }

        query.prepare(QStringLiteral("INSERT OR IGNORE INTO item_tags (itemId, tagId) SELECT ?, id FROM tags WHERE name = ?"));
        query.addBindValue(item.getId());
        query.addBindValue(tag);
        if (!execChecked(query, QStringLiteral("关联标签"))) { m_db.rollback(); return false; }
    }

    if (!m_db.commit()) {
        m_lastError = QStringLiteral("提交失败");
        m_db.rollback();
        return false;
    }
    return true;
}

bool DatabaseManager::deleteItem(const QString &itemId)
{
    // 软删除：标记 deletedTime，进入回收站（30 天内可恢复）
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE items SET deletedTime = ? WHERE id = ?"));
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(itemId);
    return execChecked(query, QStringLiteral("移入回收站"));
}

QList<TodoItem> DatabaseManager::loadDeleted()
{
    QList<TodoItem> items;
    if (!isOpen()) {
        return items;
    }

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT id, title, details, createdTime, completedTime, updatedTime, isCompleted, folderId, plannedDate, dueDate, priority, tagColor, isPinned, remindAt FROM items WHERE deletedTime IS NOT NULL ORDER BY deletedTime DESC"))) {
        m_lastError = query.lastError().text();
        return items;
    }

    while (query.next()) {
        TodoItem item;
        item.setId(query.value(0).toString());
        item.setTitle(query.value(1).toString());
        item.setDetails(query.value(2).toString());
        item.setCreatedTime(QDateTime::fromString(query.value(3).toString(), Qt::ISODate));
        item.setCompletedTime(QDateTime::fromString(query.value(4).toString(), Qt::ISODate));
        item.setUpdatedTime(QDateTime::fromString(query.value(5).toString(), Qt::ISODate));
        item.setCompleted(query.value(6).toInt() == 1);
        item.setFolderId(query.value(7).toString());
        item.setPlannedDate(QDate::fromString(query.value(8).toString(), Qt::ISODate));
        item.setDueDate(QDate::fromString(query.value(9).toString(), Qt::ISODate));
        item.setPriority(query.value(10).toInt());
        item.setTagColor(query.value(11).toString());
        item.setPinned(query.value(12).toInt() == 1);
        item.setRemindAt(QDateTime::fromString(query.value(13).toString(), Qt::ISODate));
        items.append(item);
    }
    return items;
}

QString DatabaseManager::deletedItemFolderName(const QString &folderId)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT name FROM folders WHERE id = ?"));
    query.addBindValue(folderId);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

bool DatabaseManager::restoreItem(const QString &itemId)
{
    // 若原文件夹已不存在，无法恢复（返回 false 由上层提示）
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE items SET deletedTime = NULL WHERE id = ? AND folderId IN (SELECT id FROM folders)"));
    query.addBindValue(itemId);
    if (!execChecked(query, QStringLiteral("恢复事项"))) {
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool DatabaseManager::hardDeleteItem(const QString &itemId)
{
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("无法开启事务");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM item_tags WHERE itemId = ?"));
    query.addBindValue(itemId);
    if (!execChecked(query, QStringLiteral("删除事项标签"))) { m_db.rollback(); return false; }

    query.prepare(QStringLiteral("DELETE FROM items WHERE id = ?"));
    query.addBindValue(itemId);
    if (!execChecked(query, QStringLiteral("彻底删除事项"))) { m_db.rollback(); return false; }

    if (!m_db.commit()) {
        m_lastError = QStringLiteral("提交失败");
        m_db.rollback();
        return false;
    }
    return true;
}

int DatabaseManager::purgeExpiredDeleted()
{
    if (!isOpen()) {
        return 0;
    }

    // 找出超过 30 天的回收站项，逐个彻底删除（保证标签关联级联清理）
    QSqlQuery query(m_db);
    const QString cutoff = QDateTime::currentDateTime().addDays(-30).toString(Qt::ISODate);
    query.prepare(QStringLiteral("SELECT id FROM items WHERE deletedTime IS NOT NULL AND deletedTime < ?"));
    query.addBindValue(cutoff);
    if (!query.exec()) {
        return 0;
    }

    QStringList expired;
    while (query.next()) {
        expired.append(query.value(0).toString());
    }
    for (const QString &id : expired) {
        hardDeleteItem(id);
    }
    return expired.size();
}

bool DatabaseManager::moveItem(const QString &itemId, const QString &targetFolderId)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE items SET folderId = ? WHERE id = ?"));
    query.addBindValue(targetFolderId);
    query.addBindValue(itemId);
    return execChecked(query, QStringLiteral("移动事项"));
}

bool DatabaseManager::setItemTags(const QString &itemId, const QStringList &tags)
{
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("无法开启事务");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM item_tags WHERE itemId = ?"));
    query.addBindValue(itemId);
    if (!execChecked(query, QStringLiteral("清除事项标签"))) { m_db.rollback(); return false; }

    for (const QString &tag : tags) {
        query.prepare(QStringLiteral("INSERT OR IGNORE INTO tags (id, name) VALUES (?, ?)"));
        query.addBindValue(QUuid::createUuid().toString(QUuid::WithoutBraces));
        query.addBindValue(tag);
        if (!execChecked(query, QStringLiteral("保存标签"))) { m_db.rollback(); return false; }

        query.prepare(QStringLiteral("INSERT OR IGNORE INTO item_tags (itemId, tagId) SELECT ?, id FROM tags WHERE name = ?"));
        query.addBindValue(itemId);
        query.addBindValue(tag);
        if (!execChecked(query, QStringLiteral("关联标签"))) { m_db.rollback(); return false; }
    }

    if (!m_db.commit()) {
        m_lastError = QStringLiteral("提交失败");
        m_db.rollback();
        return false;
    }
    return true;
}

QStringList DatabaseManager::allTagNames()
{
    QStringList names;
    if (!isOpen()) {
        return names;
    }
    QSqlQuery query(m_db);
    if (query.exec(QStringLiteral("SELECT name FROM tags ORDER BY name COLLATE NOCASE"))) {
        while (query.next()) {
            names.append(query.value(0).toString());
        }
    }
    return names;
}

bool DatabaseManager::addTag(const QString &name)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT OR IGNORE INTO tags (id, name) VALUES (?, ?)"));
    query.addBindValue(QUuid::createUuid().toString(QUuid::WithoutBraces));
    query.addBindValue(name);
    return execChecked(query, QStringLiteral("新建标签"));
}

bool DatabaseManager::removeTag(const QString &name)
{
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("无法开启事务");
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM item_tags WHERE tagId IN (SELECT id FROM tags WHERE name = ?)"));
    query.addBindValue(name);
    if (!execChecked(query, QStringLiteral("删除标签关联"))) { m_db.rollback(); return false; }

    query.prepare(QStringLiteral("DELETE FROM tags WHERE name = ?"));
    query.addBindValue(name);
    if (!execChecked(query, QStringLiteral("删除标签"))) { m_db.rollback(); return false; }

    if (!m_db.commit()) {
        m_lastError = QStringLiteral("提交失败");
        m_db.rollback();
        return false;
    }
    return true;
}

bool DatabaseManager::replaceAll(const QList<TodoFolder> &folders)
{
    // 原子替换：事务内先清后插，任何失败整体回滚
    if (!m_db.transaction()) {
        m_lastError = QStringLiteral("无法开启事务");
        return false;
    }

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("DELETE FROM item_tags")) ||
        !query.exec(QStringLiteral("DELETE FROM items")) ||
        !query.exec(QStringLiteral("DELETE FROM folders"))) {
        m_lastError = query.lastError().text();
        m_db.rollback();
        return false;
    }

    for (const TodoFolder &folder : folders) {
        query.prepare(QStringLiteral("INSERT INTO folders (id, name, createdTime, isPinned, color) VALUES (?, ?, ?, ?, ?)"));
        query.addBindValue(folder.getId());
        query.addBindValue(folder.getName());
        query.addBindValue(folder.getCreatedTime().toString(Qt::ISODate));
        query.addBindValue(folder.isPinned() ? 1 : 0);
        query.addBindValue(folder.getColor());
        if (!execChecked(query, QStringLiteral("导入文件夹"))) { m_db.rollback(); return false; }

        for (const TodoItem &item : folder.getItems()) {
            query.prepare(QStringLiteral("INSERT INTO items (id, title, details, createdTime, completedTime, updatedTime, isCompleted, folderId, plannedDate, dueDate, priority, tagColor, isPinned, remindAt) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
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
            query.addBindValue(item.getRemindAt().isValid() ? item.getRemindAt().toString(Qt::ISODate) : QVariant());
            if (!execChecked(query, QStringLiteral("导入事项"))) { m_db.rollback(); return false; }

            for (const QString &tag : item.getTags()) {
                query.prepare(QStringLiteral("INSERT OR IGNORE INTO tags (id, name) VALUES (?, ?)"));
                query.addBindValue(QUuid::createUuid().toString(QUuid::WithoutBraces));
                query.addBindValue(tag);
                if (!execChecked(query, QStringLiteral("导入标签"))) { m_db.rollback(); return false; }

                query.prepare(QStringLiteral("INSERT OR IGNORE INTO item_tags (itemId, tagId) SELECT ?, id FROM tags WHERE name = ?"));
                query.addBindValue(item.getId());
                query.addBindValue(tag);
                if (!execChecked(query, QStringLiteral("导入标签关联"))) { m_db.rollback(); return false; }
            }
        }
    }

    if (!m_db.commit()) {
        m_lastError = QStringLiteral("提交失败");
        m_db.rollback();
        return false;
    }
    return true;
}

QString DatabaseManager::backupDir() const
{
    QFileInfo dbInfo(m_dbPath);
    return dbInfo.absoluteDir().absolutePath() + QStringLiteral("/../backups");
}

bool DatabaseManager::backupNow(const QString &reason)
{
    if (m_dbPath.isEmpty() || !QFile::exists(m_dbPath)) {
        return false;
    }

    // 先 checkpoint，确保 WAL 内容落入主文件，备份才完整
    if (m_db.isOpen()) {
        QSqlQuery q(m_db);
        q.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE)"));
    }

    QString dirPath = backupDir();
    QDir dir(dirPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        m_lastError = QStringLiteral("无法创建备份目录");
        return false;
    }

    QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    QString target = dir.absoluteFilePath(QStringLiteral("todolist_%1_%2.db").arg(stamp, reason));
    if (!QFile::copy(m_dbPath, target)) {
        m_lastError = QStringLiteral("备份失败");
        return false;
    }

    // 滚动清理：仅保留最近 kMaxBackups 份
    QStringList files = dir.entryList({QStringLiteral("todolist_*.db")}, QDir::Files, QDir::Name);
    while (files.size() > kMaxBackups) {
        QFile::remove(dir.absoluteFilePath(files.first()));
        files.removeFirst();
    }
    return true;
}

bool DatabaseManager::execChecked(QSqlQuery &query, const QString &what)
{
    if (!query.exec()) {
        m_lastError = QStringLiteral("%1失败: %2").arg(what, query.lastError().text());
        qWarning() << "[DatabaseManager]" << m_lastError;
        return false;
    }
    return true;
}
