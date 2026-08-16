#include "tagwidget.h"
#include "../theme.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QScrollBar>
#include <QStyle>

namespace {
    // 标签配色：按标签名哈希取色，保证标签云/列表/待办色条颜色一致且稳定
    QColor tagColorFor(const QString &tag)
    {
        static const QList<QColor> colors = {
            QColor(96, 165, 250),
            QColor(74, 222, 128),
            QColor(251, 146, 60),
            QColor(248, 113, 113),
            QColor(167, 139, 250),
            QColor(34, 211, 238),
            QColor(250, 204, 21),
            QColor(129, 140, 248),
        };
        return colors[qHash(tag) % colors.size()];
    }
}

TagCloudItem::TagCloudItem(const QString &tag, int count, QWidget *parent)
    : QWidget(parent)
    , m_tag(tag)
    , m_count(count)
{
    m_bgColor = tagColorFor(tag);

    int baseSize = 12;
    int sizeIncrement = qMin(count, 5) * 2;
    int fontSize = baseSize + sizeIncrement;

    QFont font;
    font.setPixelSize(fontSize);
    font.setWeight(count > 3 ? QFont::DemiBold : QFont::Normal);

    QFontMetrics fm(font);
    int width = fm.horizontalAdvance(tag) + 28;
    int height = fm.height() + 12;

    setMinimumSize(width, height);
    setCursor(Qt::PointingHandCursor);
}

void TagCloudItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_tag);
    }
}

void TagCloudItem::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void TagCloudItem::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

void TagCloudItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QFont font = painter.font();
    int baseSize = 12;
    int sizeIncrement = qMin(m_count, 5) * 2;
    font.setPixelSize(baseSize + sizeIncrement);
    font.setWeight(m_count > 3 ? QFont::DemiBold : QFont::Normal);
    painter.setFont(font);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = r.height() / 2.0;

    if (Theme::isDark()) {
        // 暗色：低饱和半透明底 + 同色描边 + 提亮文字，避免刺眼
        QColor bg = m_bgColor;
        bg.setAlpha(m_hovered ? 66 : 40);
        painter.setPen(Qt::NoPen);
        painter.setBrush(bg);
        painter.drawRoundedRect(r, radius, radius);

        QColor edge = m_bgColor;
        edge.setAlpha(m_hovered ? 220 : 140);
        painter.setPen(QPen(edge, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(r, radius, radius);

        painter.setPen(Theme::mix(m_bgColor, QColor(255, 255, 255), m_hovered ? 0.45 : 0.30));
    } else {
        // 浅色：实心底 + 白字，悬停微加深
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_hovered ? m_bgColor.darker(112) : m_bgColor);
        painter.drawRoundedRect(r, radius, radius);
        painter.setPen(QColor(255, 255, 255));
    }

    painter.drawText(rect(), Qt::AlignCenter, m_tag);
}

TagListItem::TagListItem(const QString &tag, int count, QWidget *parent)
    : QWidget(parent)
    , m_tag(tag)
    , m_count(count)
{
    m_bgColor = tagColorFor(tag);
    setMinimumHeight(44);
    setCursor(Qt::PointingHandCursor);
    m_deleteRect = QRect(0, 0, 0, 0);
}

void TagListItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_deleteRect.contains(event->pos())) {
            emit deleteRequested(m_tag);
        } else {
            emit clicked(m_tag);
        }
    }
}

void TagListItem::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    update();
    QWidget::enterEvent(event);
}

void TagListItem::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QWidget::leaveEvent(event);
}

void TagListItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 卡片：圆角 + 内边距，悬停时主题色微光
    const QRectF card = QRectF(rect()).adjusted(3, 3, -3, -3);
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_hovered ? Theme::withAlpha(Theme::primary(), Theme::isDark() ? 26 : 16)
                               : Theme::glassBg());
    painter.drawRoundedRect(card, 9, 9);
    if (m_hovered) {
        painter.setPen(QPen(Theme::withAlpha(Theme::primary(), 110), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(card.adjusted(0.5, 0.5, -0.5, -0.5), 8.5, 8.5);
    }

    // 左侧色条
    painter.setPen(Qt::NoPen);
    painter.setBrush(m_bgColor);
    painter.drawRoundedRect(QRectF(card.left() + 10, card.top() + (card.height() - 20) / 2, 4, 20), 2, 2);

    // 标签名（暗色下稍微提亮）
    QFont tagFont;
    tagFont.setPixelSize(13);
    tagFont.setWeight(QFont::Medium);
    painter.setFont(tagFont);
    painter.setPen(Theme::isDark() ? Theme::mix(Theme::textPrimary(), m_bgColor, 0.18)
                                   : Theme::textPrimary());
    painter.drawText(QRectF(card.left() + 24, card.top(), card.width() - 110, card.height()),
                     Qt::AlignLeft | Qt::AlignVCenter, m_tag);

    // 右侧删除按钮（先定位，徽标贴其左）
    const int btnSize = 18;
    const int btnLeft = static_cast<int>(card.right()) - 10 - btnSize;
    const int btnTop = static_cast<int>(card.top() + (card.height() - btnSize) / 2);
    m_deleteRect = QRect(btnLeft, btnTop, btnSize, btnSize);

    // 数量徽标：标签色半透明胶囊
    QFont countFont;
    countFont.setPixelSize(11);
    painter.setFont(countFont);
    const QFontMetrics cfm(countFont);
    const QString countText = QString::number(m_count);
    const int badgeW = qMax(22, cfm.horizontalAdvance(countText) + 12);
    const int badgeH = 18;
    const QRectF badge(btnLeft - 8 - badgeW, card.top() + (card.height() - badgeH) / 2, badgeW, badgeH);

    QColor badgeBg = m_bgColor;
    badgeBg.setAlpha(Theme::isDark() ? 42 : 30);
    painter.setPen(Qt::NoPen);
    painter.setBrush(badgeBg);
    painter.drawRoundedRect(badge, badgeH / 2.0, badgeH / 2.0);
    painter.setPen(Theme::isDark() ? Theme::mix(m_bgColor, QColor(255, 255, 255), 0.30)
                                   : m_bgColor.darker(115));
    painter.drawText(badge, Qt::AlignCenter, countText);

    // 删除按钮：默认低调，悬停整行时显现
    painter.setPen(Qt::NoPen);
    painter.setBrush(Theme::withAlpha(Theme::danger(), m_hovered ? (Theme::isDark() ? 70 : 46)
                                                                 : (Theme::isDark() ? 34 : 22)));
    painter.drawRoundedRect(m_deleteRect, 9, 9);

    painter.setPen(QPen(Theme::withAlpha(Theme::danger(), m_hovered ? 255 : 170), 2));
    int margin = 5;
    painter.drawLine(m_deleteRect.left() + margin, m_deleteRect.top() + margin,
                     m_deleteRect.right() - margin, m_deleteRect.bottom() - margin);
    painter.drawLine(m_deleteRect.right() - margin, m_deleteRect.top() + margin,
                     m_deleteRect.left() + margin, m_deleteRect.bottom() - margin);
}

TodoItemWidget::TodoItemWidget(const TodoItem &item, const QString &folderName, QWidget *parent)
    : QWidget(parent)
    , m_todoId(item.getId())
    , m_folderId(item.getFolderId())
    , m_title(item.getTitle())
    , m_details(item.getDetails())
    , m_folderName(folderName)
    , m_completed(item.isCompleted())
    , m_dueDate(item.getDueDate())
    , m_tagColor(item.getTagColor())
    , m_tags(item.getTags())
{
    setMinimumHeight(72);
    setCursor(Qt::PointingHandCursor);
}

void TodoItemWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QRect checkRect(8, (height() - 18) / 2, 18, 18);
        if (checkRect.contains(event->pos())) {
            emit toggled(m_todoId, !m_completed);
        } else {
            emit clicked(m_todoId, m_folderId);
        }
    }
}

void TodoItemWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_todoId, m_folderId);
    }
}

void TodoItemWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect contentRect = rect();

    // 条目底用更实的玻璃色，文字对比更清晰
    if (m_completed) {
        if (!m_tagColor.isEmpty()) {
            QColor baseColor(m_tagColor);
            QColor lightColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(),
                                       Theme::isDark() ? 55 : 25);
            QColor whiteColor = Theme::glassBgStrong();
            QLinearGradient gradient(contentRect.left(), contentRect.top(),
                                     contentRect.right(), contentRect.top());
            gradient.setColorAt(0, lightColor);
            gradient.setColorAt(1, whiteColor);
            painter.fillRect(contentRect, gradient);
        } else {
            painter.fillRect(contentRect, Theme::glassBgStrong());
        }
    } else if (!m_tagColor.isEmpty()) {
        QColor baseColor(m_tagColor);
        QColor lightColor = QColor(baseColor.red(), baseColor.green(), baseColor.blue(),
                                   Theme::isDark() ? 55 : 40);
        QColor whiteColor = Theme::glassBgStrong();
        QLinearGradient gradient(contentRect.left(), contentRect.top(),
                                 contentRect.right(), contentRect.top());
        gradient.setColorAt(0, lightColor);
        gradient.setColorAt(1, whiteColor);
        painter.fillRect(contentRect, gradient);
    } else {
        painter.fillRect(contentRect, Theme::glassBgStrong());
    }

    QColor checkColor = m_completed ? Theme::textMuted() : Theme::borderStrong();
    QColor checkBg = Theme::glassBgStrong();

    QRect checkRect(16, 14, 18, 18);
    painter.setPen(Qt::NoPen);
    painter.setBrush(checkBg);
    painter.drawRoundedRect(checkRect, 4, 4);
    painter.setPen(QPen(checkColor, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(checkRect, 4, 4);

    if (m_completed) {
        QPainterPath checkPath;
        checkPath.moveTo(checkRect.left() + 4, checkRect.center().y());
        checkPath.lineTo(checkRect.center().x(), checkRect.bottom() - 4);
        checkPath.lineTo(checkRect.right() - 4, checkRect.top() + 4);
        painter.setPen(QPen(Theme::textSecondary(), 2));
        painter.drawPath(checkPath);
    }

    QFont titleFont;
    titleFont.setPixelSize(15);
    titleFont.setBold(!m_completed);
    titleFont.setStrikeOut(m_completed);
    painter.setFont(titleFont);
    painter.setPen(m_completed ? Theme::textMuted() : Theme::textPrimary());

    QFontMetrics fm(titleFont);
    QString elidedTitle = fm.elidedText(m_title, Qt::ElideRight, width() - 120);
    painter.drawText(QRect(44, 10, width() - 120, 20), Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);

    QFont detailFont;
    detailFont.setPixelSize(11);
    painter.setFont(detailFont);
    painter.setPen(Theme::textSecondary());

    QFontMetrics detailFm(detailFont);
    QString detailText = m_details.isEmpty() ? QString::fromUtf8("还没有写任何内容呢~") : m_details;
    detailText = detailText.split('\n').first();
    QString elidedDetail = detailFm.elidedText(detailText, Qt::ElideRight, width() - 120);
    painter.drawText(QRect(44, 30, width() - 120, 18), Qt::AlignLeft | Qt::AlignVCenter, elidedDetail);

    QFont infoFont;
    infoFont.setPixelSize(11);
    painter.setFont(infoFont);
    painter.setPen(Theme::textSecondary());

    QString infoText;
    if (m_dueDate.isValid()) {
        infoText = m_dueDate.toString("MM-dd");
    }
    if (!m_folderName.isEmpty()) {
        if (!infoText.isEmpty()) infoText += " · ";
        infoText += m_folderName;
    }
    painter.drawText(QRect(44, 50, width() - 120, 16), Qt::AlignLeft | Qt::AlignVCenter, infoText);

    if (!m_tagColor.isEmpty()) {
        QColor tagColor(m_tagColor);
        if (m_completed) {
            tagColor = QColor(tagColor.red(), tagColor.green(), tagColor.blue(),
                              Theme::isDark() ? 170 : 100);
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(tagColor);
        painter.drawRoundedRect(width() - 40, (height() - 6) / 2, 24, 6, 3, 3);
    }
}

TagWidget::TagWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setupConnections();
}

TagWidget::~TagWidget()
{
}

void TagWidget::setupUI()
{
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(16, 16, 16, 16);
    m_mainLayout->setSpacing(16);
    
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(16);
    
    m_cloudPanel = new QWidget();
    m_cloudLayout = new QVBoxLayout(m_cloudPanel);
    m_cloudLayout->setContentsMargins(0, 0, 0, 0);
    m_cloudLayout->setSpacing(0);

    m_cloudHeader = new QWidget();
    m_cloudHeader->setFixedHeight(40);
    QHBoxLayout *cloudHeaderLayout = new QHBoxLayout(m_cloudHeader);
    cloudHeaderLayout->setContentsMargins(20, 8, 20, 8);

    m_cloudTitle = new QLabel("标签云");
    cloudHeaderLayout->addWidget(m_cloudTitle);
    m_cloudLayout->addWidget(m_cloudHeader);

    m_cloudContainer = new QWidget();
    m_cloudContainer->setStyleSheet("background-color: transparent;");
    m_cloudFlow = new FlowLayout(m_cloudContainer, 8, 12, 12);
    m_cloudLayout->addWidget(m_cloudContainer);

    leftLayout->addWidget(m_cloudPanel, 1);

    m_listPanel = new QWidget();
    m_listLayout = new QVBoxLayout(m_listPanel);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(0);

    m_listHeader = new QWidget();
    m_listHeader->setFixedHeight(40);
    QHBoxLayout *listHeaderLayout = new QHBoxLayout(m_listHeader);
    listHeaderLayout->setContentsMargins(20, 8, 20, 8);

    m_listTitle = new QLabel("标签列表");
    listHeaderLayout->addWidget(m_listTitle);
    listHeaderLayout->addStretch();
    m_listLayout->addWidget(m_listHeader);

    m_tagScrollArea = new QScrollArea();
    m_tagScrollArea->setWidgetResizable(true);
    m_tagScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tagScrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");

    m_tagContainer = new QWidget();
    m_tagContainer->setStyleSheet("background-color: transparent;");
    m_tagListLayout = new QVBoxLayout(m_tagContainer);
    m_tagListLayout->setContentsMargins(12, 0, 12, 12);
    m_tagListLayout->setSpacing(4);
    m_tagListLayout->addStretch();

    m_tagScrollArea->setWidget(m_tagContainer);
    m_listLayout->addWidget(m_tagScrollArea, 1);

    m_addPanel = new QWidget();
    m_addLayout = new QHBoxLayout(m_addPanel);
    m_addLayout->setContentsMargins(16, 12, 16, 12);
    m_addLayout->setSpacing(8);

    m_addLineEdit = new QLineEdit();
    m_addLineEdit->setPlaceholderText("新建标签...");
    m_addLayout->addWidget(m_addLineEdit, 1);

    m_addButton = new QPushButton("添加");
    m_addLayout->addWidget(m_addButton);

    m_listLayout->addWidget(m_addPanel);

    leftLayout->addWidget(m_listPanel, 1);

    m_mainLayout->addWidget(leftPanel, 1);

    m_todoPanel = new QWidget();
    m_todoPanel->setMinimumWidth(400);
    QVBoxLayout *todoMainLayout = new QVBoxLayout(m_todoPanel);
    todoMainLayout->setContentsMargins(0, 0, 0, 0);
    todoMainLayout->setSpacing(0);

    m_todoHeader = new QWidget();
    QVBoxLayout *todoHeaderLayout = new QVBoxLayout(m_todoHeader);
    todoHeaderLayout->setContentsMargins(20, 16, 20, 12);
    todoHeaderLayout->setSpacing(6);

    m_todoTitle = new QLabel("待办事项");
    todoHeaderLayout->addWidget(m_todoTitle);

    m_selectedTagLabel = new QLabel("选择一个标签查看相关待办事项");
    todoHeaderLayout->addWidget(m_selectedTagLabel);

    todoMainLayout->addWidget(m_todoHeader);

    m_todoScrollArea = new QScrollArea();
    m_todoScrollArea->setWidgetResizable(true);
    m_todoScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_todoScrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");

    m_todoContainer = new QWidget();
    m_todoContainer->setStyleSheet("background-color: transparent;");
    m_todoListLayout = new QVBoxLayout(m_todoContainer);
    m_todoListLayout->setContentsMargins(12, 12, 12, 12);
    m_todoListLayout->setSpacing(6);
    m_todoListLayout->addStretch();

    m_todoScrollArea->setWidget(m_todoContainer);
    todoMainLayout->addWidget(m_todoScrollArea, 1);

    m_mainLayout->addWidget(m_todoPanel, 2);

    refreshTheme();
}

void TagWidget::refreshTheme()
{
    const QString panelStyle = QStringLiteral(
            "background-color: %1; border: 1px solid %2; border-radius: 12px;")
                               .arg(Theme::glassBg().name(QColor::HexArgb),
                                    Theme::glassBorder().name(QColor::HexArgb));
    const QString headerStyle = QStringLiteral(
            "background-color: transparent; border-bottom: 1px solid %1;"
            "border-top-left-radius: 12px; border-top-right-radius: 12px;")
                                .arg(Theme::glassBorder().name(QColor::HexArgb));
    const QString titleStyle = QStringLiteral("font-size: 14px; font-weight: 600; color: %1; border: none;")
                               .arg(Theme::textPrimary().name());

    m_cloudPanel->setStyleSheet(panelStyle);
    m_cloudHeader->setStyleSheet(headerStyle);
    m_cloudTitle->setStyleSheet(titleStyle);

    m_listPanel->setStyleSheet(panelStyle);
    m_listHeader->setStyleSheet(headerStyle);
    m_listTitle->setStyleSheet(titleStyle);

    m_todoPanel->setStyleSheet(panelStyle);
    m_todoHeader->setStyleSheet(headerStyle);
    m_todoTitle->setStyleSheet(titleStyle);

    m_selectedTagLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: %1; border: none;")
                                      .arg(Theme::textSecondary().name()));

    m_addPanel->setStyleSheet(QStringLiteral("background-color: transparent; border-top: 1px solid %1;")
                              .arg(Theme::border().name()));

    m_addLineEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { background-color: %1; border: 1px solid %2; border-radius: 8px; "
        "padding: 8px 12px; color: %3; font-size: 13px; }"
        "QLineEdit:focus { border-color: %4; background-color: %5; }"
        "QLineEdit::placeholder { color: %6; }")
        .arg(Theme::surfaceAlt().name(), Theme::border().name(), Theme::textPrimary().name(),
             Theme::textMuted().name(), Theme::surface().name(), Theme::textMuted().name()));

    m_addButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; border: 2px solid %2; border-radius: 8px; "
        "padding: 8px 16px; color: %2; font-size: 13px; font-weight: 500; }"
        "QPushButton:hover { background-color: %3; }"
        "QPushButton:pressed { background-color: %4; }")
        .arg(Theme::surface().name(), Theme::primary().name(),
             Theme::withAlpha(Theme::primary(), 25).name(QColor::HexArgb),
             Theme::withAlpha(Theme::primary(), 50).name(QColor::HexArgb)));
}

void TagWidget::setupConnections()
{
    connect(m_addButton, &QPushButton::clicked, this, &TagWidget::onAddTag);
}

void TagWidget::updateData(const QList<TodoFolder> &folders, const QStringList &allTags)
{
    m_folders = folders;
    m_allTags = allTags;
    collectAllTags();
    refreshTagCloud();
    refreshTagList();
    refreshTodoList();
}

void TagWidget::collectAllTags()
{
    m_tagCounts.clear();
    m_tagToTodos.clear();
    m_todoToFolder.clear();

    // 标签库（含尚未关联任何事项的标签）由调用方统一从 DatabaseManager 提供，
    // 本组件不再直接访问数据库，保证数据访问入口唯一。
    for (const QString &tagName : m_allTags) {
        if (!m_tagCounts.contains(tagName)) {
            m_tagCounts[tagName] = 0;
        }
    }
    
    for (const TodoFolder &folder : m_folders) {
        for (const TodoItem &item : folder.getItems()) {
            m_todoToFolder[item.getId()] = folder.getName();
            for (const QString &tag : item.getTags()) {
                m_tagCounts[tag]++;
                m_tagToTodos[tag].append(item);
            }
        }
    }
}

void TagWidget::refreshTagCloud()
{
    QLayoutItem *item;
    while ((item = m_cloudFlow->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    
    for (auto it = m_tagCounts.begin(); it != m_tagCounts.end(); ++it) {
        TagCloudItem *cloudItem = new TagCloudItem(it.key(), it.value());
        connect(cloudItem, &TagCloudItem::clicked, this, &TagWidget::onTagCloudClicked);
        m_cloudFlow->addWidget(cloudItem);
    }
}

void TagWidget::refreshTagList()
{
    while (m_tagListLayout->count() > 1) {
        QLayoutItem *item = m_tagListLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    
    for (auto it = m_tagCounts.begin(); it != m_tagCounts.end(); ++it) {
        TagListItem *listItem = new TagListItem(it.key(), it.value());
        connect(listItem, &TagListItem::clicked, this, &TagWidget::onTagListClicked);
        connect(listItem, &TagListItem::deleteRequested, this, &TagWidget::onTagDeleteRequested);
        m_tagListLayout->insertWidget(m_tagListLayout->count() - 1, listItem);
    }
}

void TagWidget::refreshTodoList()
{
    while (m_todoListLayout->count() > 1) {
        QLayoutItem *item = m_todoListLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    if (m_selectedTag.isEmpty()) {
        m_selectedTagLabel->setText("选择一个标签查看相关待办事项");
        return;
    }
    
    int count = m_tagCounts.value(m_selectedTag, 0);
    m_selectedTagLabel->setText(QString("标签 \"%1\" 共有 %2 个待办事项").arg(m_selectedTag).arg(count));
    
    if (count == 0) {
        return;
    }
    
    if (m_tagToTodos.contains(m_selectedTag)) {
        QList<TodoItem> sortedTodos = m_tagToTodos[m_selectedTag];
        std::sort(sortedTodos.begin(), sortedTodos.end(), [](const TodoItem &a, const TodoItem &b) {
            if (a.isCompleted() != b.isCompleted()) {
                return a.isCompleted() < b.isCompleted();
            }
            return a.getCreatedTime() > b.getCreatedTime();
        });
        
        for (const TodoItem &item : sortedTodos) {
            QString folderName = m_todoToFolder.value(item.getId(), "");
            TodoItemWidget *widget = new TodoItemWidget(item, folderName);
            connect(widget, &TodoItemWidget::clicked, this, &TagWidget::onTodoItemClicked);
            connect(widget, &TodoItemWidget::toggled, this, &TagWidget::onTodoItemToggled);
            m_todoListLayout->insertWidget(m_todoListLayout->count() - 1, widget);
        }
    }
}

void TagWidget::onTagCloudClicked(const QString &tag)
{
    m_selectedTag = tag;
    refreshTodoList();
    emit tagSelected(tag);
}

void TagWidget::onTagListClicked(const QString &tag)
{
    m_selectedTag = tag;
    refreshTodoList();
    emit tagSelected(tag);
}

void TagWidget::onTagDeleteRequested(const QString &tag)
{
    // 只发信号：实际删除由 MainWindow 经 DatabaseManager 在事务中级联完成
    emit tagDeleted(tag);
}

void TagWidget::onTodoItemClicked(const QString &todoId, const QString &folderId)
{
    emit todoClicked(todoId, folderId);
}

void TagWidget::onTodoItemToggled(const QString &todoId, bool completed)
{
    emit todoToggled(todoId, completed);
}

void TagWidget::onAddTag()
{
    QString tag = m_addLineEdit->text().trimmed();
    if (!tag.isEmpty()) {
        emit tagCreated(tag);
        m_addLineEdit->clear();
    }
}
