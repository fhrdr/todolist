#ifndef MESSAGEUTILS_H
#define MESSAGEUTILS_H

#include "../theme.h"

#include <QWidget>
#include <QString>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QPainter>
#include <QMouseEvent>

// 无边框圆角对话框基类：自定义标题栏（可拖拽 + 关闭按钮），
// 文字/输入框/按钮颜色全部显式指定，深浅色模式均清晰可见
class StyledDialog : public QDialog
{
public:
    explicit StyledDialog(QWidget *parent, const QString &title,
                          const QColor &accent = Theme::primary(), int minWidth = 360)
        : QDialog(parent)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setModal(true);
        setMinimumWidth(minWidth + 24);   // 外圈留阴影边距

        QVBoxLayout *outer = new QVBoxLayout(this);
        outer->setContentsMargins(12, 12, 12, 12);
        outer->setSpacing(0);

        m_panel = new QWidget(this);
        m_panel->setObjectName(QStringLiteral("styledDialogPanel"));
        m_panel->setStyleSheet(QStringLiteral(
            "QWidget#styledDialogPanel {"
            "  background-color: %1;"
            "  border: 1px solid %2;"
            "  border-radius: 12px;"
            "}")
            .arg(Theme::surface().name(),
                 Theme::glassBorder().name(QColor::HexArgb)));
        outer->addWidget(m_panel);

        QVBoxLayout *panelLayout = new QVBoxLayout(m_panel);
        panelLayout->setContentsMargins(20, 14, 20, 18);
        panelLayout->setSpacing(14);

        // ---- 标题栏：主题色圆点 + 标题 + 关闭按钮 ----
        QHBoxLayout *titleRow = new QHBoxLayout();
        titleRow->setContentsMargins(0, 0, 0, 0);
        titleRow->setSpacing(10);

        QLabel *dot = new QLabel(m_panel);
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 5px;")
                               .arg(accent.name()));
        titleRow->addWidget(dot, 0, Qt::AlignVCenter);

        QLabel *titleLabel = new QLabel(title, m_panel);
        titleLabel->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 15px; font-weight: 600; background: transparent;")
            .arg(Theme::textPrimary().name()));
        titleRow->addWidget(titleLabel, 1);

        QPushButton *closeBtn = new QPushButton(QStringLiteral("×"), m_panel);
        closeBtn->setFixedSize(26, 26);
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setStyleSheet(QStringLiteral(
            "QPushButton { color: %1; background: transparent; border: none;"
            "  border-radius: 13px; font-size: 16px; padding: 0; }"
            "QPushButton:hover { background-color: %2; color: %3; }")
            .arg(Theme::textMuted().name(),
                 Theme::hoverGlow().name(QColor::HexArgb),
                 Theme::textPrimary().name()));
        titleRow->addWidget(closeBtn, 0, Qt::AlignVCenter);
        QObject::connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

        panelLayout->addLayout(titleRow);

        // ---- 内容区 ----
        m_body = new QVBoxLayout();
        m_body->setContentsMargins(0, 2, 0, 0);
        m_body->setSpacing(14);
        panelLayout->addLayout(m_body, 1);
    }

    QVBoxLayout *body() const { return m_body; }

    // 创建内容文字标签（显式主题色，任何模式下可读）
    QLabel *makeLabel(const QString &text)
    {
        QLabel *label = new QLabel(text, m_panel);
        label->setWordWrap(true);
        label->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 14px; background: transparent;")
            .arg(Theme::textPrimary().name()));
        return label;
    }

    // 标准按钮行：右对齐 [确定(主按钮)] [取消(描边按钮)]，与全局按钮风格统一
    void addStandardButtons(QPushButton *&okBtn, QPushButton *&cancelBtn,
                            const QString &okText = QStringLiteral("确定"))
    {
        QHBoxLayout *row = new QHBoxLayout();
        row->setContentsMargins(0, 4, 0, 0);
        row->setSpacing(10);
        row->addStretch(1);

        okBtn = new QPushButton(okText, m_panel);
        okBtn->setProperty("variant", "primary");
        okBtn->setDefault(true);
        okBtn->setMinimumWidth(84);
        row->addWidget(okBtn);

        cancelBtn = new QPushButton(QStringLiteral("取消"), m_panel);
        cancelBtn->setMinimumWidth(84);
        row->addWidget(cancelBtn);

        m_body->addLayout(row);

        QObject::connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
        QObject::connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    }

protected:
    // 面板后的柔和阴影
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRect r = m_panel->geometry();
        const int shadowAlpha = Theme::isDark() ? 16 : 9;
        for (int i = 6; i >= 1; --i) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0, 0, 0, shadowAlpha));
            p.drawRoundedRect(r.adjusted(-i, -i + 2, i, i + 2), 14 + i, 14 + i);
        }
    }

    // 标题栏拖拽移动
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton
            && event->pos().y() < m_panel->geometry().top() + 44) {
            m_dragging = true;
            m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
            return;
        }
        QDialog::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (m_dragging) {
            move(event->globalPosition().toPoint() - m_dragPos);
            event->accept();
            return;
        }
        QDialog::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        m_dragging = false;
        QDialog::mouseReleaseEvent(event);
    }

private:
    QWidget *m_panel = nullptr;
    QVBoxLayout *m_body = nullptr;
    QPoint m_dragPos;
    bool m_dragging = false;
};

// 统一的消息/输入对话框工具，无边框圆角风格，与全局主题一致
class MessageUtils
{
public:
    static void showInfo(QWidget *parent, const QString &title, const QString &message)
    {
        showBox(parent, title, message, Theme::primary());
    }

    static void showSuccess(QWidget *parent, const QString &title, const QString &message)
    {
        showBox(parent, title, message, Theme::success());
    }

    static void showWarning(QWidget *parent, const QString &title, const QString &message)
    {
        showBox(parent, title, message, Theme::warning());
    }

    static void showError(QWidget *parent, const QString &title, const QString &message)
    {
        showBox(parent, title, message, Theme::danger());
    }

    static bool showConfirm(QWidget *parent, const QString &title, const QString &message)
    {
        StyledDialog dialog(parent, title, Theme::primary(), 340);
        dialog.body()->addWidget(dialog.makeLabel(message));

        QPushButton *okBtn, *cancelBtn;
        dialog.addStandardButtons(okBtn, cancelBtn);

        return dialog.exec() == QDialog::Accepted;
    }

    // 文本输入对话框；取消时返回空字符串
    static QString getText(QWidget *parent, const QString &title, const QString &label,
                           const QString &defaultValue = QString())
    {
        StyledDialog dialog(parent, title, Theme::primary(), 360);
        dialog.body()->addWidget(dialog.makeLabel(label));

        QLineEdit *lineEdit = new QLineEdit(defaultValue);
        dialog.body()->addWidget(lineEdit);

        QPushButton *okBtn, *cancelBtn;
        dialog.addStandardButtons(okBtn, cancelBtn);
        QObject::connect(lineEdit, &QLineEdit::returnPressed, &dialog, &QDialog::accept);

        lineEdit->setFocus();
        lineEdit->selectAll();

        if (dialog.exec() == QDialog::Accepted) {
            return lineEdit->text();
        }
        return QString();
    }

    // 下拉选择（可编辑）对话框；取消时返回空字符串
    static QString getItem(QWidget *parent, const QString &title, const QString &label,
                           const QStringList &items, int current = 0, bool editable = true)
    {
        StyledDialog dialog(parent, title, Theme::primary(), 360);
        dialog.body()->addWidget(dialog.makeLabel(label));

        QComboBox *comboBox = new QComboBox();
        comboBox->addItems(items);
        comboBox->setCurrentIndex(current);
        comboBox->setEditable(editable);
        comboBox->setMaxVisibleItems(10);
        dialog.body()->addWidget(comboBox);

        QPushButton *okBtn, *cancelBtn;
        dialog.addStandardButtons(okBtn, cancelBtn);

        comboBox->setFocus();

        if (dialog.exec() == QDialog::Accepted) {
            return comboBox->currentText();
        }
        return QString();
    }

private:
    static void showBox(QWidget *parent, const QString &title, const QString &message,
                        const QColor &accent)
    {
        StyledDialog dialog(parent, title, accent, 320);
        dialog.body()->addWidget(dialog.makeLabel(message));

        QHBoxLayout *row = new QHBoxLayout();
        row->setContentsMargins(0, 4, 0, 0);
        row->addStretch(1);
        QPushButton *okBtn = new QPushButton(QStringLiteral("确定"));
        okBtn->setProperty("variant", "primary");
        okBtn->setDefault(true);
        okBtn->setMinimumWidth(84);
        row->addWidget(okBtn);
        dialog.body()->addLayout(row);
        QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

        dialog.exec();
    }
};

#endif // MESSAGEUTILS_H
