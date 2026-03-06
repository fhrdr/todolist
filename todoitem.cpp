#include "todoitem.h"

TodoItem::TodoItem()
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_title("")
    , m_details("")
    , m_createdTime(QDateTime::currentDateTime())
    , m_updatedTime(QDateTime::currentDateTime())
    , m_isCompleted(false)
    , m_folderId("")
    , m_plannedDate(QDate::currentDate())
    , m_priority(0)
    , m_tagColor("#2563eb")
    , m_isPinned(false)
{
}

TodoItem::TodoItem(const QString &title, const QString &details)
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_title(title)
    , m_details(details)
    , m_createdTime(QDateTime::currentDateTime())
    , m_updatedTime(QDateTime::currentDateTime())
    , m_isCompleted(false)
    , m_folderId("")
    , m_plannedDate(QDate::currentDate())
    , m_priority(0)
    , m_tagColor("#2563eb")
    , m_isPinned(false)
{
}

TodoItem::TodoItem(const QJsonObject &json)
{
    fromJson(json);
}

void TodoItem::setCompleted(bool completed)
{
    m_isCompleted = completed;
    m_updatedTime = QDateTime::currentDateTime();
    if (completed && m_completedTime.isNull()) {
        m_completedTime = QDateTime::currentDateTime();
    } else if (!completed) {
        m_completedTime = QDateTime();
    }
}

QJsonObject TodoItem::toJson() const
{
    QJsonObject json;
    json["id"] = m_id;
    json["title"] = m_title;
    json["details"] = m_details;
    json["createdTime"] = m_createdTime.toString(Qt::ISODate);
    json["completedTime"] = m_completedTime.toString(Qt::ISODate);
    json["updatedTime"] = m_updatedTime.toString(Qt::ISODate);
    json["isCompleted"] = m_isCompleted;
    json["folderId"] = m_folderId;
    json["plannedDate"] = m_plannedDate.toString(Qt::ISODate);
    json["dueDate"] = m_dueDate.toString(Qt::ISODate);
    json["priority"] = m_priority;
    json["tags"] = QJsonArray::fromStringList(m_tags);
    json["tagColor"] = m_tagColor;
    json["isPinned"] = m_isPinned;
    return json;
}

void TodoItem::fromJson(const QJsonObject &json)
{
    m_id = json["id"].toString();
    m_title = json["title"].toString();
    m_details = json["details"].toString();
    m_createdTime = QDateTime::fromString(json["createdTime"].toString(), Qt::ISODate);
    m_completedTime = QDateTime::fromString(json["completedTime"].toString(), Qt::ISODate);
    m_updatedTime = QDateTime::fromString(json["updatedTime"].toString(), Qt::ISODate);
    m_isCompleted = json["isCompleted"].toBool();
    m_folderId = json["folderId"].toString();
    m_plannedDate = QDate::fromString(json["plannedDate"].toString(), Qt::ISODate);
    m_dueDate = QDate::fromString(json["dueDate"].toString(), Qt::ISODate);
    m_priority = json["priority"].toInt(0);
    m_tagColor = json["tagColor"].toString("#2563eb");
    m_isPinned = json["isPinned"].toBool(false);
    
    QJsonArray tagsArray = json["tags"].toArray();
    m_tags.clear();
    for (const QJsonValue &tag : tagsArray) {
        m_tags.append(tag.toString());
    }
    
    if (m_id.isEmpty()) {
        m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    if (!m_createdTime.isValid()) {
        m_createdTime = QDateTime::currentDateTime();
    }
    
    if (!m_updatedTime.isValid()) {
        m_updatedTime = m_createdTime;
    }
    
    if (!m_plannedDate.isValid()) {
        m_plannedDate = QDate::currentDate();
    }
}
