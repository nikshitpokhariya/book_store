#ifndef SIGNUPWINDOW_H
#define SIGNUPWINDOW_H

#include <QWidget>

class QLineEdit;
class QPushButton;

class SignupWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SignupWindow(QWidget *parent = nullptr);

signals:
    void signupSuccessful();

private slots:
    void handleSignup();

private:
    QLineEdit *usernameEdit;
    QLineEdit *emailEdit;
    QLineEdit *passwordEdit;
    QLineEdit *confirmPasswordEdit;

    QPushButton *signupButton;
    QPushButton *backButton;
};

#endif // SIGNUPWINDOW_H