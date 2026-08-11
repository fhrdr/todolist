#ifndef DESKTOPWIDGET_H
#define DESKTOPWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTimer>
#include <QPoint>
#include "todoitem.h"
#include "todofolder.h"

class QPainter;
class QNetworkAccessManager;
class QNetworkReply;

// 桌面便利贴（仿小黄条风格）：
// 便利贴纸张 + 和纸胶带 + 卡通猫 + 手绘复选框 + 换肤/透明度/置顶
class DesktopWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DesktopWidget(QWidget *parent = nullptr);
    ~DesktopWidget();

    void updateTodoData(const QList<TodoFolder> &folders);
    void refreshDisplay();

    enum PaperTheme { Lemon = 0, Sakura, Mint, Sky, Cream };
    enum Character { CharCat = 0, CharRabbit, CharShiba };

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onAddTodoClicked();
    void onTodoItemClicked(QListWidgetItem *item);
    void onRefreshTimer();

signals:
    void todoItemToggled(const QString &itemId, bool completed);
    void newTodoRequested(const QString &title);
    void showMainWindowRequested();
    void editTodoRequested(const QString &itemId);
    void deleteTodoRequested(const QString &itemId);

private:
    // ---- UI / 数据 ----
    void setupUI();
    void setupConnections();
    void updateTodoList();
    void loadPendingItems();
    void updateHeader();
    void updateQuote();

    // ---- 外观 ----
    void applyTheme();
    void setPaperTheme(PaperTheme theme);
    void setCharacter(Character character);
    void setNoteOpacity(qreal opacity);
    void setAlwaysOnTop(bool onTop);
    QColor paperTop() const;
    QColor paperBottom() const;
    QColor accentColor() const;

    // ---- 绘制 ----
    QRect noteRect() const;
    void drawPaper(QPainter &p);
    void drawTape(QPainter &p);
    void drawStarSticker(QPainter &p);
    void drawCharacter(QPainter &p);
    void drawCat(QPainter &p);
    void drawRabbit(QPainter &p);
    void drawShiba(QPainter &p);
    void drawProgressRing(QPainter &p);
    void scheduleBlink();

    // ---- 天气贴片 ----
    QRect weatherRect() const;
    QString weatherText() const;
    void drawWeather(QPainter &p);
    void fetchWeather();
    void onWeatherReply(QNetworkReply *reply);
    void promptSetCity();

    // ---- 窗口行为 ----
    void updateCursor(const QPoint &pos);
    bool isOnResizeArea(const QPoint &pos);
    void saveWindowPosition();
    void loadWindowPosition();
    void saveWindowSize();
    void loadWindowSize();
    void saveAppearance();
    void loadAppearance();

    // ---- 控件 ----
    QLabel *m_dateLabel;
    QLabel *m_countLabel;
    QLabel *m_quoteLabel;
    QLabel *m_dueLabel;
    QPushButton *m_pinButton;
    QListWidget *m_todoListWidget;
    QLineEdit *m_addLineEdit;

    // ---- 数据 ----
    QList<TodoFolder> m_folders;
    QList<TodoItem> m_displayItems;
    int m_quoteOffset = 0;
    bool m_dueUrgent = false;

    // ---- 外观状态 ----
    PaperTheme m_theme = Lemon;
    Character m_character = CharCat;
    bool m_alwaysOnTop = true;
    bool m_blink = false;

    // ---- 天气状态 ----
    QNetworkAccessManager *m_netManager = nullptr;
    QTimer *m_weatherTimer = nullptr;
    QString m_weatherCity = QStringLiteral("北京");
    QString m_weatherDesc;
    int m_weatherTemp = 0;
    int m_weatherCode = -1;   // -1 = 尚未获取
    bool m_weatherLoading = false;

    // ---- 拖拽 / 缩放 ----
    QPoint m_dragPosition;
    bool m_dragging = false;
    bool m_resizing = false;
    int m_resizeEdge = 0;
    QPoint m_resizeStartPos;
    QSize m_resizeStartSize;
    QPoint m_resizeStartWindowPos;

    QTimer *m_refreshTimer;
};

#endif // DESKTOPWIDGET_H
