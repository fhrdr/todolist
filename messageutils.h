#ifndef MESSAGEUTILS_H
#define MESSAGEUTILS_H

#include <QMessageBox>
#include <QWidget>
#include <QString>

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
        QMessageBox msgBox(parent);
        msgBox.setWindowTitle(title);
        msgBox.setText(message);
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setStyleSheet(getStyleSheet());
        return msgBox.exec() == QMessageBox::Yes;
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
                min-width: 280px;
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
            QPushButton[text="No"], QPushButton[text="否"] {
                background-color: #6b7280;
            }
            QPushButton[text="No"]:hover, QPushButton[text="否"]:hover {
                background-color: #4b5563;
            }
        )";
    }
};

#endif // MESSAGEUTILS_H
