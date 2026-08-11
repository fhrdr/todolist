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
    
    QString getId() const { return m_id; }
    QString getName() const { return m_name; }
    QDateTime getCreatedTime() const { return m_createdTime; }
    QList<TodoItem> getItems() const { return m_items; }
    int getItemCount() const { return m_items.size(); }
    int getCompletedCount() const;
    int getPendingCount() const;
    bool isPinned() const { return m_isPinned; }
    QString getColor() const { return m_color; }
    
    void setName(const QString &name) { m_name = name; }
    void setPinned(bool pinned) { m_isPinned = pinned; }
    void setColor(const QString &color) { m_color = color; }
    void setId(const QString &id) { m_id = id; }
    void setCreatedTime(const QDateTime &time) { m_createdTime = time; }
    
    void addItem(const TodoItem &item);
    void removeItem(const QString &itemId);
    void updateItem(const TodoItem &item);
    TodoItem* findItem(const QString &itemId);
    void clearCompletedItems();
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    
    bool operator==(const TodoFolder &other) const { return m_id == other.m_id; }
    bool operator!=(const TodoFolder &other) const { return m_id != other.m_id; }
    
private:
    QString m_id;
    QString m_name;
    QDateTime m_createdTime;
    QList<TodoItem> m_items;
    bool m_isPinned;
    QString m_color;
};

#endif // TODOFOLDER_H
