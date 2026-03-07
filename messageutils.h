#ifndef MESSAGEUTILS_H
#define MESSAGEUTILS_H

#include <QMessageBox>
#include <QWidget>
#include <QString>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

class MessageUtils
{
public:
    enum MessageType {
        Info,
        Success,
        Warning,
        Error
    };
    
    static void showInfo(QWidget *parent, const QString &title, const QString &message)
    {
        QMessageBox msgBox(parent);
        msgBox.setWindowTitle(title);
        msgBox.setText(message);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet(getStyleSheet());
        msgBox.exec();
    }
    
    static void showSuccess(QWidget *parent, const QString &title, const QString &message)
    {
        QMessageBox msgBox(parent);
        msgBox.setWindowTitle(title);
        msgBox.setText(message);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet(getStyleSheet());
        msgBox.exec();
    }
    
    static void showWarning(QWidget *parent, const QString &title, const QString &message)
    {
        QMessageBox msgBox(parent);
        msgBox.setWindowTitle(title);
        msgBox.setText(message);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStyleSheet(getStyleSheet());
        msgBox.exec();
    }
    
    static void showError(QWidget *parent, const QString &title, const QString &message)
    {
        QMessageBox msgBox(parent);
        msgBox.setWindowTitle(title);
        msgBox.setText(message);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStyleSheet(getStyleSheet());
        msgBox.exec();
    }
    
    static bool showConfirm(QWidget *parent, const QString &title, const QString &message)
    {
        QDialog dialog(parent);
        dialog.setWindowTitle(title);
        dialog.setMinimumWidth(320);
        dialog.setStyleSheet(R"(
            QDialog {
                background-color: #ffffff;
                font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
            }
            QLabel {
                color: #334155;
                font-size: 14px;
            }
            QPushButton {
                background-color: #ffffff;
                border: 2px solid #3b82f6;
                border-radius: 6px;
                color: #3b82f6;
                padding: 8px 20px;
                font-size: 13px;
                font-weight: 500;
                min-width: 70px;
            }
            QPushButton:hover {
                background-color: rgba(59, 130, 246, 0.1);
            }
            QPushButton:pressed {
                background-color: rgba(59, 130, 246, 0.2);
            }
        )");
        
        QVBoxLayout *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(20);
        
        QLabel *messageLabel = new QLabel(message);
        messageLabel->setWordWrap(true);
        layout->addWidget(messageLabel);
        
        QDialogButtonBox *buttonBox = new QDialogButtonBox();
        QPushButton *okBtn = new QPushButton("确定");
        QPushButton *cancelBtn = new QPushButton("取消");
        okBtn->setDefault(true);
        buttonBox->addButton(okBtn, QDialogButtonBox::AcceptRole);
        buttonBox->addButton(cancelBtn, QDialogButtonBox::RejectRole);
        layout->addWidget(buttonBox);
        
        QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
        QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
        
        return dialog.exec() == QDialog::Accepted;
    }
    
private:
    static QString getStyleSheet()
    {
        return R"(
            QMessageBox {
                background-color: #ffffff;
                font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
                font-size: 13px;
            }
            QMessageBox QLabel {
                color: #1f2937;
                font-size: 13px;
                min-width: 250px;
            }
            QPushButton {
                background-color: #4f46e5;
                color: white;
                border: none;
                border-radius: 6px;
                padding: 8px 20px;
                font-size: 13px;
                font-weight: 500;
                min-width: 80px;
            }
            QPushButton:hover {
                background-color: #4338ca;
            }
            QPushButton:pressed {
                background-color: #3730a3;
            }
        )";
    }
};

#endif // MESSAGEUTILS_H
