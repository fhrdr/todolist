#ifndef SECTIONHEADER_H
#define SECTIONHEADER_H

#include <QWidget>
#include <QString>
#include <QPropertyAnimation>

// 章节标题：文字水平居中，下方紧贴一条加粗主题色下划线。
// 首次显示时下划线从中心向两端展开（交互动效）。
class SectionHeader : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal underlineProgress READ underlineProgress WRITE setUnderlineProgress)

public:
    explicit SectionHeader(const QString &title, QWidget *parent = nullptr);

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    qreal underlineProgress() const { return m_progress; }
    void setUnderlineProgress(qreal p) { m_progress = p; update(); }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QString m_title;
    qreal m_progress = 0.0;
    bool m_animated = false;
};

#endif // SECTIONHEADER_H
