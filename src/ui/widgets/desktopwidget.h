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
#include <QSize>
#include <QVector>
#include <QElapsedTimer>
#include "todoitem.h"
#include "todofolder.h"

class QPainter;
class QNetworkAccessManager;
class QNetworkReply;

// 桌面便利贴（暗夜霓虹玻璃风）：
// 半透明深色玻璃纸 + 内部流动光斑 + 霓虹进度条 + 天气贴片 + 透明度/置顶
class DesktopWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DesktopWidget(QWidget *parent = nullptr);
    ~DesktopWidget();

    void updateTodoData(const QList<TodoFolder> &folders);
    void refreshDisplay();

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
    void setNoteOpacity(qreal opacity);
    void setAlwaysOnTop(bool onTop);

    // ---- 绘制 ----
    QRect noteRect() const;
    void drawNote(QPainter &p);

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
    QWidget *m_progressBar;

    // ---- 数据 ----
    QList<TodoFolder> m_folders;
    QList<TodoItem> m_displayItems;
    int m_quoteOffset = 0;

    // ---- 外观状态 ----
    bool m_alwaysOnTop = true;

    // ---- 光斑动画 ----
    struct NoteParticle { qreal x, y, vx, vy, size; int colorIdx; };
    void initNoteParticles();
    void advanceNoteParticles(qreal dt);

    QElapsedTimer m_clock;
    QTimer *m_animTimer = nullptr;
    QVector<NoteParticle> m_particles;
    qint64 m_lastTick = 0;
    QSize m_particleArea;           // 粒子初始化时的区域尺寸（变化则重建）

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
