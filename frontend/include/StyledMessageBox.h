#ifndef STYLEDMESSAGEBOX_H
#define STYLEDMESSAGEBOX_H

#include <QDialog>
#include <QString>

class QLabel;
class QPushButton;

class StyledMessageBox : public QDialog
{
    Q_OBJECT

public:
    enum Type {
        Info,
        Success,
        Warning,
        Error,
        Question
    };

    explicit StyledMessageBox(Type type,
                             const QString &title,
                             const QString &message,
                             QWidget *parent = nullptr,
                             bool hasCancel = false,
                             const QString &okText = "OK",
                             const QString &cancelText = "Cancel");

    static void information(QWidget *parent, const QString &title, const QString &message);
    static void success(QWidget *parent, const QString &title, const QString &message);
    static void warning(QWidget *parent, const QString &title, const QString &message);
    static void critical(QWidget *parent, const QString &title, const QString &message);
    static bool question(QWidget *parent,
                         const QString &title,
                         const QString &message,
                         const QString &okText = "Confirm",
                         const QString &cancelText = "Cancel");
    static bool confirm(QWidget *parent, const QString &title, const QString &message);

private:
    void setupUI(Type type,
                 const QString &title,
                 const QString &message,
                 bool hasCancel,
                 const QString &okText,
                 const QString &cancelText);
};

#endif // STYLEDMESSAGEBOX_H
