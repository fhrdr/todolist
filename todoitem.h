#ifndef TODOITEM_H
#define TODOITEM_H

#include <QString>
#include <QDateTime>
#include <QDate>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QStringList>

class TodoItem
{
public:
    TodoItem();
    TodoItem(const QString &title, const QString &details = "");
    TodoItem(const QJsonObject &json);
    
    QString getId() const { return m_id; }
    QString getTitle() const { return m_title; }
    QString getDetails() const { return m_details; }
    QDateTime getCreatedTime() const { return m_createdTime; }
    QDateTime getCompletedTime() const { return m_completedTime; }
    QDateTime getUpdatedTime() const { return m_updatedTime; }
    bool isCompleted() const { return m_isCompleted; }
    QString getFolderId() const { return m_folderId; }
    QDate getPlannedDate() const { return m_plannedDate; }
    QDate getDueDate() const { return m_dueDate; }
    int getPriority() const { return m_priority; }
    QStringList getTags() const { return m_tags; }
    QString getTagColor() const { return m_tagColor; }
    bool isPinned() const { return m_isPinned; }
    
    void setTitle(const QString &title) { m_title = title; m_updatedTime = QDateTime::currentDateTime(); }
    void setDetails(const QString &details) { m_details = details; m_updatedTime = QDateTime::currentDateTime(); }
    void setCompleted(bool completed);
    void setFolderId(const QString &folderId) { m_folderId = folderId; }
    void setPlannedDate(const QDate &date) { m_plannedDate = date; m_updatedTime = QDateTime::currentDateTime(); }
    void setDueDate(const QDate &date) { m_dueDate = date; m_updatedTime = QDateTime::currentDateTime(); }
    void setPriority(int priority) { m_priority = priority; m_updatedTime = QDateTime::currentDateTime(); }
    void setTags(const QStringList &tags) { m_tags = tags; m_updatedTime = QDateTime::currentDateTime(); }
    void addTag(const QString &tag) { if (!m_tags.contains(tag)) { m_tags.append(tag); m_updatedTime = QDateTime::currentDateTime(); } }
    void removeTag(const QString &tag) { m_tags.removeAll(tag); m_updatedTime = QDateTime::currentDateTime(); }
    void setTagColor(const QString &color) { m_tagColor = color; }
    void setPinned(bool pinned) { m_isPinned = pinned; m_updatedTime = QDateTime::currentDateTime(); }
    void setId(const QString &id) { m_id = id; }
    void setCreatedTime(const QDateTime &time) { m_createdTime = time; }
    void setCompletedTime(const QDateTime &time) { m_completedTime = time; }
    void setUpdatedTime(const QDateTime &time) { m_updatedTime = time; }
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    
    bool operator==(const TodoItem &other) const { return m_id == other.m_id; }
    bool operator!=(const TodoItem &other) const { return m_id != other.m_id; }
    
private:
    QString m_id;
    QString m_title;
    QString m_details;
    QDateTime m_createdTime;
    QDateTime m_completedTime;
    QDateTime m_updatedTime;
    bool m_isCompleted;
    QString m_folderId;
    QDate m_plannedDate;
    QDate m_dueDate;
    int m_priority;
    QStringList m_tags;
    QString m_tagColor;
    bool m_isPinned;
};

#endif // TODOITEM_H
