#ifndef TODOFOLDER_H
#define TODOFOLDER_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QUuid>
#include <QList>
#include "todoitem.h"

class TodoFolder
{
public:
    TodoFolder();
    TodoFolder(const QString &name);
    TodoFolder(const QJsonObject &json);
    
    // Getters
    QString getId() const { return m_id; }
    QString getName() const { return m_name; }
    QDateTime getCreatedTime() const { return m_createdTime; }
    QList<TodoItem> getItems() const { return m_items; }
    int getItemCount() const { return m_items.size(); }
    int getCompletedCount() const;
    int getPendingCount() const;
    
    // Setters
    void setName(const QString &name) { m_name = name; }
    
    // Item management
    void addItem(const TodoItem &item);
    void removeItem(const QString &itemId);
    void updateItem(const TodoItem &item);
    TodoItem* findItem(const QString &itemId);
    void clearCompletedItems();
    
    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    
private:
    QString m_id;              // 唯一标识符
    QString m_name;            // 文件夹名称
    QDateTime m_createdTime;   // 创建时间
    QList<TodoItem> m_items;   // 包含的待办事项列表
};

#endif // TODOFOLDER_H