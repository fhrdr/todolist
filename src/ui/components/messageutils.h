#ifndef MESSAGEUTILS_H
#define MESSAGEUTILS_H

#include "../theme.h"

#include <QMessageBox>
#include <QWidget>
#include <QString>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>

// 统一的消息/输入对话框工具，样式与全局主题一致
class MessageUtils
{
public:
    static void showInfo(QWidget *parent, const QString &title, const QString &message)
    {
        showBox(parent, title, message, QMessageBox::Information);
    }

    static void showSuccess(QWidget *parent, const QString &title, const QString &message)
    {
        showBox(parent, title, message, QMessageBox::Information);
    }

    static void showWarning(QWidget *parent, const QString &title, const QString &message)
    {
        showBox(parent, title, message, QMessageBox::Warning);
    }

    static void showError(QWidget *parent, const QString &title, const QString &message)
    {
        showBox(parent, title, message, QMessageBox::Critical);
    }

    static bool showConfirm(QWidget *parent, const QString &title, const QString &message)
    {
        QDialog dialog(parent);
        dialog.setWindowTitle(title);
        dialog.setMinimumWidth(340);
        dialog.setStyleSheet(dialogStyle());

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(24, 24, 24, 20);
        layout->setSpacing(20);

        QLabel *messageLabel = new QLabel(message);
        messageLabel->setWordWrap(true);
        layout->addWidget(messageLabel);

        QDialogButtonBox *buttonBox = new QDialogButtonBox();
        QPushButton *okBtn = new QPushButton(QStringLiteral("确定"));
        QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
        okBtn->setDefault(true);
        buttonBox->addButton(okBtn, QDialogButtonBox::AcceptRole);
        buttonBox->addButton(cancelBtn, QDialogButtonBox::RejectRole);
        layout->addWidget(buttonBox);

        QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

        return dialog.exec() == QDialog::Accepted;
    }

    // 文本输入对话框；取消时返回空字符串
    static QString getText(QWidget *parent, const QString &title, const QString &label,
                           const QString &defaultValue = QString())
    {
        QDialog dialog(parent);
        dialog.setWindowTitle(title);
        dialog.setMinimumWidth(360);
        dialog.setStyleSheet(dialogStyle());

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(24, 24, 24, 20);
        layout->setSpacing(16);

        layout->addWidget(new QLabel(label));

        QLineEdit *lineEdit = new QLineEdit(defaultValue);
        layout->addWidget(lineEdit);

        QDialogButtonBox *buttonBox = new QDialogButtonBox();
        QPushButton *okBtn = new QPushButton(QStringLiteral("确定"));
        QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
        okBtn->setDefault(true);
        buttonBox->addButton(okBtn, QDialogButtonBox::AcceptRole);
        buttonBox->addButton(cancelBtn, QDialogButtonBox::RejectRole);
        layout->addWidget(buttonBox);

        QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
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
        QDialog dialog(parent);
        dialog.setWindowTitle(title);
        dialog.setMinimumWidth(360);
        dialog.setStyleSheet(dialogStyle());

        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(24, 24, 24, 20);
        layout->setSpacing(16);

        layout->addWidget(new QLabel(label));

        QComboBox *comboBox = new QComboBox();
        comboBox->addItems(items);
        comboBox->setCurrentIndex(current);
        comboBox->setEditable(editable);
        comboBox->setMaxVisibleItems(10);
        layout->addWidget(comboBox);

        QDialogButtonBox *buttonBox = new QDialogButtonBox();
        QPushButton *okBtn = new QPushButton(QStringLiteral("确定"));
        QPushButton *cancelBtn = new QPushButton(QStringLiteral("取消"));
        okBtn->setDefault(true);
        buttonBox->addButton(okBtn, QDialogButtonBox::AcceptRole);
        buttonBox->addButton(cancelBtn, QDialogButtonBox::RejectRole);
        layout->addWidget(buttonBox);

        QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

        comboBox->setFocus();

        if (dialog.exec() == QDialog::Accepted) {
            return comboBox->currentText();
        }
        return QString();
    }

private:
    static void showBox(QWidget *parent, const QString &title, const QString &message, QMessageBox::Icon icon)
    {
        QMessageBox msgBox(parent);
        msgBox.setWindowTitle(title);
        msgBox.setText(message);
        msgBox.setIcon(icon);
        msgBox.setStyleSheet(boxStyle());
        msgBox.exec();
    }

    static QString boxStyle()
    {
        return QStringLiteral(R"(
            QMessageBox {
                background-color: %1;
                font-family: "Microsoft YaHei UI", "Segoe UI", sans-serif;
                font-size: 14px;
            }
            QMessageBox QLabel {
                color: %2;
                font-size: 14px;
                min-width: 260px;
            }
            QPushButton {
                background-color: %3;
                color: #ffffff;
                border: none;
                border-radius: 8px;
                padding: 9px 22px;
                font-size: 13px;
                font-weight: 500;
                min-width: 76px;
            }
            QPushButton:hover { background-color: %4; }
            QPushButton:pressed { background-color: %4; }
        )").arg(Theme::surface().name(),
                Theme::textPrimary().name(),
                Theme::primary().name(),
                Theme::primaryHover().name());
    }

    static QString dialogStyle()
    {
        return QStringLiteral(R"(
            QDialog {
                background-color: %1;
                font-family: "Microsoft YaHei UI", "Segoe UI", sans-serif;
            }
            QLabel {
                color: %2;
                font-size: 14px;
            }
            QLineEdit, QComboBox {
                border: 1px solid %3;
                border-radius: 8px;
                padding: 10px 14px;
                background-color: %1;
                color: %2;
                font-size: 14px;
            }
            QLineEdit:focus, QComboBox:focus {
                border-color: %4;
            }
            QComboBox::drop-down { border: none; width: 26px; }
            QComboBox QAbstractItemView {
                background-color: %1;
                border: 1px solid %3;
                selection-background-color: %5;
                selection-color: %2;
                outline: none;
                padding: 4px;
            }
            QComboBox QAbstractItemView::item {
                padding: 8px 12px;
                min-height: 22px;
                border-radius: 4px;
            }
            QPushButton {
                background-color: %1;
                border: 2px solid %4;
                border-radius: 8px;
                color: %4;
                padding: 9px 22px;
                font-size: 13px;
                font-weight: 500;
                min-width: 76px;
            }
            QPushButton:hover { background-color: %5; }
            QPushButton:pressed { background-color: %6; }
        )").arg(Theme::surface().name(),
                Theme::textPrimary().name(),
                Theme::border().name(),
                Theme::primary().name(),
                Theme::primarySoft().name(),
                Theme::primarySoft2().name());
    }
};

#endif // MESSAGEUTILS_H
