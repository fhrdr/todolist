#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QSqlDatabase>
#include "todoitem.h"
#include "todofolder.h"

// 数据库存取与备份的统一入口。
// 设计原则：所有写操作均为增量 SQL（替代旧的"全表删除+重建"），
// 事务返回值全部校验，失败即回滚；启动/导入前自动备份数据库文件。
class DatabaseManager
{
public:
    static DatabaseManager& instance();

    bool initialize();                       // 打开数据库、建表、迁移、启动备份
    bool isOpen() const;

    QList<TodoFolder> loadAll();             // 加载全部文件夹及其事项（含标签）

    // 增量写操作（全部带事务校验）
    bool upsertFolder(const TodoFolder &folder);
    bool deleteFolder(const QString &folderId);
    bool upsertItem(const TodoItem &item);   // 要求 item.folderId 已设置
    bool deleteItem(const QString &itemId);  // 软删除：移入回收站
    bool moveItem(const QString &itemId, const QString &targetFolderId);
    bool setItemTags(const QString &itemId, const QStringList &tags);

    // 回收站（软删除，30 天保留）
    QList<TodoItem> loadDeleted();           // 回收站内的事项（含原文件夹名）
    QString deletedItemFolderName(const QString &folderId);
    bool restoreItem(const QString &itemId);
    bool hardDeleteItem(const QString &itemId);
    int  purgeExpiredDeleted();              // 清理超过 30 天的回收站项，返回清理数

    // 标签库
    QStringList allTagNames();
    bool addTag(const QString &name);
    bool removeTag(const QString &name);

    // 导入：原子替换全部数据，任何一步失败即回滚，绝不产生半写状态
    bool replaceAll(const QList<TodoFolder> &folders);

    // 备份：复制数据库文件到 backups/，滚动保留最近 maxBackups 份
    bool backupNow(const QString &reason);
    QString backupDir() const;

    QString lastError() const { return m_lastError; }

private:
    DatabaseManager() = default;
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool openDatabase();
    bool createSchema();
    bool upgradeSchema();                    // 列级增量迁移（remindAt / deletedTime）
    void migrateLegacyDatabase();            // 从旧的 exe 同级 data/ 目录迁移
    void migrateFromJson();                  // 从旧的 JSON 存储迁移
    bool execChecked(class QSqlQuery &query, const QString &what);

    QSqlDatabase m_db;
    QString m_dbPath;
    QString m_lastError;
    bool m_open = false;
};

#endif // DATABASEMANAGER_H
