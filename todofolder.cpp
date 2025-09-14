#include "todofolder.h"
#include <QJsonArray>

TodoFolder::TodoFolder()
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_name("新建文件夹")
    , m_createdTime(QDateTime::currentDateTime())
{
}

TodoFolder::TodoFolder(const QString &name)
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_name(name)
    , m_createdTime(QDateTime::currentDateTime())
{
}

TodoFolder::TodoFolder(const QJsonObject &json)
{
    fromJson(json);
}

int TodoFolder::getCompletedCount() const
{
    int count = 0;
    for (const TodoItem &item : m_items) {
        if (item.isCompleted()) {
            count++;
        }
    }
    return count;
}

int TodoFolder::getPendingCount() const
{
    int count = 0;
    for (const TodoItem &item : m_items) {
        if (!item.isCompleted()) {
            count++;
        }
    }
    return count;
}

void TodoFolder::addItem(const TodoItem &item)
{
    // 设置待办事项的文件夹ID
    TodoItem newItem = item;
    newItem.setFolderId(m_id);
    m_items.append(newItem);
}

void TodoFolder::removeItem(const QString &itemId)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].getId() == itemId) {
            m_items.removeAt(i);
            break;
        }
    }
}

void TodoFolder::updateItem(const TodoItem &item)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].getId() == item.getId()) {
            m_items[i] = item;
            break;
        }
    }
}

TodoItem* TodoFolder::findItem(const QString &itemId)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].getId() == itemId) {
            return &m_items[i];
        }
    }
    return nullptr;
}

void TodoFolder::clearCompletedItems()
{
    for (int i = m_items.size() - 1; i >= 0; --i) {
        if (m_items[i].isCompleted()) {
            m_items.removeAt(i);
        }
    }
}

QJsonObject TodoFolder::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["name"] = m_name;
    json["createdTime"] = m_createdTime.toString(Qt::ISODate);
    
    QJsonArray itemsArray;
    for (const TodoItem &item : m_items) {
        itemsArray.append(item.toJson());
    }
    json["items"] = itemsArray;
    
    return json;
}

void TodoFolder::fromJson(const QJsonObject &json)
{
    m_id = json["id"].toString();
    m_name = json["name"].toString();
    m_createdTime = QDateTime::fromString(json["createdTime"].toString(), Qt::ISODate);
    
    // 如果ID为空，生成新的ID
    if (m_id.isEmpty()) {
        m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    // 如果创建时间无效，设置为当前时间
    if (!m_createdTime.isValid()) {
        m_createdTime = QDateTime::currentDateTime();
    }
    
    // 加载待办事项
    m_items.clear();
    QJsonArray itemsArray = json["items"].toArray();
    for (const QJsonValue &value : itemsArray) {
        TodoItem item(value.toObject());
        item.setFolderId(m_id); // 确保文件夹ID正确
        m_items.append(item);
    }
}