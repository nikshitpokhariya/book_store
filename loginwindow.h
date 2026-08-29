#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;

class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

private slots:
    void handleLogin();
    void openSignup();

private:
    QLineEdit *emailEdit;
    QLineEdit *passwordEdit;

    QPushButton *loginButton;
    QPushButton *signupButton;

    QLabel *logoLabel;
    QLabel *taglineLabel;
    QLabel *descriptionLabel;
    QLabel *welcomeLabel;
    QLabel *loginSubtitleLabel;
    QLabel *accountLabel;
};

#endif // LOGINWINDOW_H