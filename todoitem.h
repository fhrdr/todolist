#ifndef TODOITEM_H
#define TODOITEM_H

#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QUuid>

class TodoItem
{
public:
    TodoItem();
    TodoItem(const QString &title, const QString &details = "");
    TodoItem(const QJsonObject &json);
    
    // Getters
    QString getId() const { return m_id; }
    QString getTitle() const { return m_title; }
    QString getDetails() const { return m_details; }
    QDateTime getCreatedTime() const { return m_createdTime; }
    QDateTime getCompletedTime() const { return m_completedTime; }
    bool isCompleted() const { return m_isCompleted; }
    QString getFolderId() const { return m_folderId; }
    
    // Setters
    void setTitle(const QString &title) { m_title = title; }
    void setDetails(const QString &details) { m_details = details; }
    void setCompleted(bool completed);
    void setFolderId(const QString &folderId) { m_folderId = folderId; }
    
    // JSON serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    
private:
    QString m_id;              // 唯一标识符
    QString m_title;           // 标题
    QString m_details;         // 详情
    QDateTime m_createdTime;   // 创建时间
    QDateTime m_completedTime; // 完成时间
    bool m_isCompleted;        // 是否完成
    QString m_folderId;        // 所属文件夹ID
};

#endif // TODOITEM_H