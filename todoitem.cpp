#include "todoitem.h"

TodoItem::TodoItem()
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_title("")
    , m_details("")
    , m_createdTime(QDateTime::currentDateTime())
    , m_isCompleted(false)
    , m_folderId("")
{
}

TodoItem::TodoItem(const QString &title, const QString &details)
    : m_id(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_title(title)
    , m_details(details)
    , m_createdTime(QDateTime::currentDateTime())
    , m_isCompleted(false)
    , m_folderId("")
{
}

TodoItem::TodoItem(const QJsonObject &json)
{
    fromJson(json);
}

void TodoItem::setCompleted(bool completed)
{
    m_isCompleted = completed;
    if (completed && m_completedTime.isNull()) {
        m_completedTime = QDateTime::currentDateTime();
    } else if (!completed) {
        m_completedTime = QDateTime(); // 重置完成时间
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
    json["isCompleted"] = m_isCompleted;
    json["folderId"] = m_folderId;
    return json;
}

void TodoItem::fromJson(const QJsonObject &json)
{
    m_id = json["id"].toString();
    m_title = json["title"].toString();
    m_details = json["details"].toString();
    m_createdTime = QDateTime::fromString(json["createdTime"].toString(), Qt::ISODate);
    m_completedTime = QDateTime::fromString(json["completedTime"].toString(), Qt::ISODate);
    m_isCompleted = json["isCompleted"].toBool();
    m_folderId = json["folderId"].toString();
    
    // 如果ID为空，生成新的ID
    if (m_id.isEmpty()) {
        m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    // 如果创建时间为空，设置为当前时间
    if (!m_createdTime.isValid()) {
        m_createdTime = QDateTime::currentDateTime();
    }
}