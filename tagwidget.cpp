#include "tagwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QScrollBar>
#include <QStyle>
#include <QRandomGenerator>

TagCloudItem::TagCloudItem(const QString &tag, int count, QWidget *parent)
    : QWidget(parent)
    , m_tag(tag)
    , m_count(count)
{
    int baseSize = 12;
    int sizeIncrement = qMin(count, 5) * 2;
    int fontSize = baseSize + sizeIncrement;
    
    QFont font;
    font.setPixelSize(fontSize);
    font.setWeight(count > 3 ? QFont::DemiBold : QFont::Normal);
    
    QFontMetrics fm(font);
    int width = fm.horizontalAdvance(tag) + 24;
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

void TagCloudItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor bgColor(238, 242, 255);
    QColor borderColor(165, 180, 252);
    QColor textColor(79, 70, 229);
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(rect(), 16, 16);
    
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 16, 16);
    
    QFont font = painter.font();
    int baseSize = 12;
    int sizeIncrement = qMin(m_count, 5) * 2;
    font.setPixelSize(baseSize + sizeIncrement);
    font.setWeight(m_count > 3 ? QFont::DemiBold : QFont::Normal);
    painter.setFont(font);
    painter.setPen(textColor);
    
    painter.drawText(rect(), Qt::AlignCenter, m_tag);
}

TagListItem::TagListItem(const QString &tag, int count, QWidget *parent)
    : QWidget(parent)
    , m_tag(tag)
    , m_count(count)
{
    setMinimumHeight(40);
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

void TagListItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    painter.fillRect(rect(), QColor(255, 255, 255));
    
    QColor tagColor(99, 102, 241);
    painter.setPen(Qt::NoPen);
    painter.setBrush(tagColor);
    painter.drawRoundedRect(16, 12, 4, height() - 24, 2, 2);
    
    QFont tagFont;
    tagFont.setPixelSize(13);
    painter.setFont(tagFont);
    painter.setPen(QColor(30, 41, 59));
    painter.drawText(QRect(28, 0, width() - 80, height()), Qt::AlignLeft | Qt::AlignVCenter, m_tag);
    
    QFont countFont;
    countFont.setPixelSize(12);
    painter.setFont(countFont);
    painter.setPen(QColor(100, 116, 139));
    QString countText = QString("(%1)").arg(m_count);
    painter.drawText(QRect(width() - 60, 0, 40, height()), Qt::AlignRight | Qt::AlignVCenter, countText);
    
    m_deleteRect = QRect(width() - 28, (height() - 16) / 2, 16, 16);
    painter.setPen(QColor(148, 163, 184));
    painter.drawText(m_deleteRect, Qt::AlignCenter, "×");
}

TodoItemWidget::TodoItemWidget(const TodoItem &item, const QString &folderName, QWidget *parent)
    : QWidget(parent)
    , m_todoId(item.getId())
    , m_folderId(item.getFolderId())
    , m_title(item.getTitle())
    , m_details(item.getDetails())
    , m_folderName(folderName)
    , m_completed(item.isCompleted())
    , m_plannedDate(item.getPlannedDate())
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
    
    if (m_completed) {
        painter.fillRect(rect(), QColor(249, 250, 251));
    } else {
        painter.fillRect(rect(), QColor(255, 255, 255));
    }
    
    QColor checkColor = m_completed ? QColor(34, 197, 94) : QColor(203, 213, 225);
    QColor checkBg = QColor(255, 255, 255);
    
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
        painter.setPen(QPen(checkColor, 2));
        painter.drawPath(checkPath);
    }
    
    QFont titleFont;
    titleFont.setPixelSize(15);
    titleFont.setBold(!m_completed);
    painter.setFont(titleFont);
    painter.setPen(m_completed ? QColor(100, 116, 139) : QColor(30, 41, 59));
    
    QFontMetrics fm(titleFont);
    QString elidedTitle = fm.elidedText(m_title, Qt::ElideRight, width() - 120);
    painter.drawText(QRect(44, 10, width() - 120, 20), Qt::AlignLeft | Qt::AlignVCenter, elidedTitle);
    
    QFont detailFont;
    detailFont.setPixelSize(11);
    painter.setFont(detailFont);
    painter.setPen(QColor(148, 163, 184));
    
    QString detailText = m_details.isEmpty() ? QString::fromUtf8("还没有写任何内容呢~") : m_details;
    QString elidedDetail = fm.elidedText(detailText, Qt::ElideRight, width() - 120);
    painter.drawText(QRect(44, 30, width() - 120, 18), Qt::AlignLeft | Qt::AlignVCenter, elidedDetail);
    
    QFont infoFont;
    infoFont.setPixelSize(10);
    painter.setFont(infoFont);
    painter.setPen(QColor(100, 116, 139));
    
    QString infoText;
    if (m_plannedDate.isValid()) {
        infoText = m_plannedDate.toString("MM-dd");
    }
    if (!m_folderName.isEmpty()) {
        if (!infoText.isEmpty()) infoText += " · ";
        infoText += m_folderName;
    }
    painter.drawText(QRect(44, 50, width() - 120, 16), Qt::AlignLeft | Qt::AlignVCenter, infoText);
    
    if (!m_tagColor.isEmpty()) {
        QColor tagColor(m_tagColor);
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
    m_cloudPanel->setStyleSheet("background-color: transparent;");
    m_cloudLayout = new QVBoxLayout(m_cloudPanel);
    m_cloudLayout->setContentsMargins(0, 0, 0, 0);
    m_cloudLayout->setSpacing(8);
    
    m_cloudTitle = new QLabel("标签云");
    m_cloudTitle->setStyleSheet("font-size: 14px; font-weight: 600; color: #1e293b;");
    m_cloudTitle->setFixedHeight(24);
    m_cloudLayout->addWidget(m_cloudTitle);
    
    m_cloudContainer = new QWidget();
    m_cloudContainer->setStyleSheet("background-color: transparent;");
    m_cloudFlow = new QFlowLayout(m_cloudContainer, 8, 8, 8);
    m_cloudLayout->addWidget(m_cloudContainer);
    
    leftLayout->addWidget(m_cloudPanel, 1);
    
    m_listPanel = new QWidget();
    m_listPanel->setStyleSheet("background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px;");
    m_listLayout = new QVBoxLayout(m_listPanel);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(0);
    
    QWidget *listHeader = new QWidget();
    listHeader->setStyleSheet("background-color: transparent;");
    listHeader->setFixedHeight(40);
    QHBoxLayout *listHeaderLayout = new QHBoxLayout(listHeader);
    listHeaderLayout->setContentsMargins(20, 8, 20, 8);
    
    m_listTitle = new QLabel("标签列表");
    m_listTitle->setStyleSheet("font-size: 14px; font-weight: 600; color: #1e293b;");
    listHeaderLayout->addWidget(m_listTitle);
    listHeaderLayout->addStretch();
    m_listLayout->addWidget(listHeader);
    
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
    m_addPanel->setStyleSheet("background-color: transparent; border-top: 1px solid #f1f5f9;");
    m_addLayout = new QHBoxLayout(m_addPanel);
    m_addLayout->setContentsMargins(16, 12, 16, 12);
    m_addLayout->setSpacing(8);
    
    m_addLineEdit = new QLineEdit();
    m_addLineEdit->setPlaceholderText("新建标签...");
    m_addLineEdit->setStyleSheet(
        "QLineEdit { background-color: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px; "
        "padding: 8px 12px; color: #334155; font-size: 13px; }"
        "QLineEdit:focus { border-color: #6366f1; background-color: #ffffff; }"
        "QLineEdit::placeholder { color: #94a3b8; }"
    );
    m_addLayout->addWidget(m_addLineEdit, 1);
    
    m_addButton = new QPushButton("添加");
    m_addButton->setObjectName("newTodoBtn");
    m_addLayout->addWidget(m_addButton);
    
    m_listLayout->addWidget(m_addPanel);
    
    leftLayout->addWidget(m_listPanel, 1);
    
    m_mainLayout->addWidget(leftPanel, 1);
    
    m_todoPanel = new QWidget();
    m_todoPanel->setStyleSheet("background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 12px;");
    m_todoPanel->setMinimumWidth(400);
    QVBoxLayout *todoMainLayout = new QVBoxLayout(m_todoPanel);
    todoMainLayout->setContentsMargins(0, 0, 0, 0);
    todoMainLayout->setSpacing(0);
    
    QWidget *todoHeader = new QWidget();
    todoHeader->setStyleSheet("background-color: transparent; border-bottom: 1px solid #f1f5f9;");
    QVBoxLayout *todoHeaderLayout = new QVBoxLayout(todoHeader);
    todoHeaderLayout->setContentsMargins(20, 16, 20, 12);
    todoHeaderLayout->setSpacing(6);
    
    m_todoTitle = new QLabel("待办事项");
    m_todoTitle->setStyleSheet("font-size: 14px; font-weight: 600; color: #1e293b;");
    todoHeaderLayout->addWidget(m_todoTitle);
    
    m_selectedTagLabel = new QLabel("选择一个标签查看相关待办事项");
    m_selectedTagLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    todoHeaderLayout->addWidget(m_selectedTagLabel);
    
    todoMainLayout->addWidget(todoHeader);
    
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
}

void TagWidget::setupConnections()
{
    connect(m_addButton, &QPushButton::clicked, this, &TagWidget::onAddTag);
}

void TagWidget::updateData(const QList<TodoFolder> &folders)
{
    m_folders = folders;
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
    
    if (m_tagToTodos.contains(m_selectedTag)) {
        for (const TodoItem &item : m_tagToTodos[m_selectedTag]) {
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

QFlowLayout::QFlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

QFlowLayout::QFlowLayout(int margin, int hSpacing, int vSpacing)
    : m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

QFlowLayout::~QFlowLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}

void QFlowLayout::addItem(QLayoutItem *item)
{
    m_itemList.append(item);
}

int QFlowLayout::horizontalSpacing() const
{
    if (m_hSpace >= 0) {
        return m_hSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
    }
}

int QFlowLayout::verticalSpacing() const
{
    if (m_vSpace >= 0) {
        return m_vSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
    }
}

int QFlowLayout::count() const
{
    return m_itemList.size();
}

QLayoutItem *QFlowLayout::itemAt(int index) const
{
    return m_itemList.value(index);
}

QLayoutItem *QFlowLayout::takeAt(int index)
{
    if (index >= 0 && index < m_itemList.size())
        return m_itemList.takeAt(index);
    return nullptr;
}

Qt::Orientations QFlowLayout::expandingDirections() const
{
    return {};
}

bool QFlowLayout::hasHeightForWidth() const
{
    return true;
}

int QFlowLayout::heightForWidth(int width) const
{
    int height = doLayout(QRect(0, 0, width, 0), true);
    return height;
}

void QFlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize QFlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize QFlowLayout::minimumSize() const
{
    QSize size;
    for (const QLayoutItem *item : std::as_const(m_itemList))
        size = size.expandedTo(item->minimumSize());
    
    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

int QFlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;
    
    for (QLayoutItem *item : std::as_const(m_itemList)) {
        const QWidget *wid = item->widget();
        int spaceX = horizontalSpacing();
        if (spaceX == -1)
            spaceX = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
        int spaceY = verticalSpacing();
        if (spaceY == -1)
            spaceY = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
        
        int nextX = x + item->sizeHint().width() + spaceX;
        if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            nextX = x + item->sizeHint().width() + spaceX;
            lineHeight = 0;
        }
        
        if (!testOnly)
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
        
        x = nextX;
        lineHeight = qMax(lineHeight, item->sizeHint().height());
    }
    
    return y + lineHeight - rect.y() + bottom;
}

int QFlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject *parent = this->parent();
    if (!parent) {
        return -1;
    } else if (parent->isWidgetType()) {
        QWidget *pw = static_cast<QWidget *>(parent);
        return pw->style()->pixelMetric(pm, nullptr, pw);
    } else {
        return static_cast<QLayout *>(parent)->spacing();
    }
}
