#ifndef TAGWIDGET_H
#define TAGWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QScrollArea>
#include <QLayout>
#include <QStyle>
#include <QPainterPath>
#include <QMap>
#include "todoitem.h"
#include "todofolder.h"

class TagCloudItem : public QWidget
{
    Q_OBJECT

public:
    explicit TagCloudItem(const QString &tag, int count, QWidget *parent = nullptr);
    QString getTag() const { return m_tag; }
    
signals:
    void clicked(const QString &tag);
    
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    
private:
    QString m_tag;
    int m_count;
};

class TagListItem : public QWidget
{
    Q_OBJECT

public:
    explicit TagListItem(const QString &tag, int count, QWidget *parent = nullptr);
    QString getTag() const { return m_tag; }
    
signals:
    void clicked(const QString &tag);
    void deleteRequested(const QString &tag);
    
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    
private:
    QString m_tag;
    int m_count;
    QRect m_deleteRect;
};

class TodoItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TodoItemWidget(const TodoItem &item, const QString &folderName, QWidget *parent = nullptr);
    QString getTodoId() const { return m_todoId; }
    
signals:
    void clicked(const QString &todoId, const QString &folderId);
    void toggled(const QString &todoId, bool completed);
    
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    
private:
    QString m_todoId;
    QString m_folderId;
    QString m_title;
    QString m_details;
    QString m_folderName;
    bool m_completed;
    QDate m_plannedDate;
    QString m_tagColor;
    QStringList m_tags;
};

class QFlowLayout : public QLayout
{
    Q_OBJECT

public:
    explicit QFlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);
    explicit QFlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1);
    ~QFlowLayout() override;

    void addItem(QLayoutItem *item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

private:
    int doLayout(const QRect &rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> m_itemList;
    int m_hSpace;
    int m_vSpace;
};

class TagWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TagWidget(QWidget *parent = nullptr);
    ~TagWidget();
    
    void updateData(const QList<TodoFolder> &folders);
    
signals:
    void tagSelected(const QString &tag);
    void todoClicked(const QString &todoId, const QString &folderId);
    void todoToggled(const QString &todoId, bool completed);
    void tagCreated(const QString &tag);
    void tagDeleted(const QString &tag);
    
private slots:
    void onTagCloudClicked(const QString &tag);
    void onTagListClicked(const QString &tag);
    void onTagDeleteRequested(const QString &tag);
    void onTodoItemClicked(const QString &todoId, const QString &folderId);
    void onTodoItemToggled(const QString &todoId, bool completed);
    void onAddTag();
    
private:
    void setupUI();
    void setupConnections();
    void refreshTagCloud();
    void refreshTagList();
    void refreshTodoList();
    void collectAllTags();
    
    QHBoxLayout *m_mainLayout;
    
    QWidget *m_cloudPanel;
    QVBoxLayout *m_cloudLayout;
    QLabel *m_cloudTitle;
    QWidget *m_cloudContainer;
    QFlowLayout *m_cloudFlow;
    
    QWidget *m_listPanel;
    QVBoxLayout *m_listLayout;
    QLabel *m_listTitle;
    QScrollArea *m_tagScrollArea;
    QWidget *m_tagContainer;
    QVBoxLayout *m_tagListLayout;
    
    QWidget *m_addPanel;
    QHBoxLayout *m_addLayout;
    QLineEdit *m_addLineEdit;
    QPushButton *m_addButton;
    
    QWidget *m_todoPanel;
    QVBoxLayout *m_todoLayout;
    QLabel *m_todoTitle;
    QLabel *m_selectedTagLabel;
    QScrollArea *m_todoScrollArea;
    QWidget *m_todoContainer;
    QVBoxLayout *m_todoListLayout;
    
    QList<TodoFolder> m_folders;
    QString m_selectedTag;
    QMap<QString, int> m_tagCounts;
    QMap<QString, QList<TodoItem>> m_tagToTodos;
    QMap<QString, QString> m_todoToFolder;
};

#endif // TAGWIDGET_H
