#include "signupwindow.h"
#include "database.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QFont>
#include <QRegularExpression>

SignupWindow::SignupWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("BookMarket - Sign Up");

    resize(450, 600);

    QLabel *title =
        new QLabel("Create Account");

    QFont titleFont;
    titleFont.setPointSize(24);
    titleFont.setBold(true);

    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);

    nameEdit =
        new QLineEdit();

    nameEdit->setPlaceholderText("Full Name");

    emailEdit =
        new QLineEdit();

    emailEdit->setPlaceholderText("Email");

    phoneEdit =
        new QLineEdit();

    phoneEdit->setPlaceholderText("Phone Number");

    passwordEdit =
        new QLineEdit();

    passwordEdit->setPlaceholderText("Password");
    passwordEdit->setEchoMode(QLineEdit::Password);

    confirmPasswordEdit =
        new QLineEdit();

    confirmPasswordEdit->setPlaceholderText(
        "Confirm Password"
        );

    confirmPasswordEdit->setEchoMode(
        QLineEdit::Password
        );

    signupButton =
        new QPushButton("Create Account");

    backButton =
        new QPushButton("Back to Login");

    signupButton->setMinimumHeight(45);
    backButton->setMinimumHeight(45);

    QVBoxLayout *layout =
        new QVBoxLayout(this);

    layout->setSpacing(12);

    layout->setContentsMargins(
        50, 35, 50, 35
        );

    layout->addWidget(title);

    layout->addSpacing(15);

    layout->addWidget(nameEdit);
    layout->addWidget(emailEdit);
    layout->addWidget(phoneEdit);
    layout->addWidget(passwordEdit);
    layout->addWidget(confirmPasswordEdit);

    layout->addSpacing(10);

    layout->addWidget(signupButton);
    layout->addWidget(backButton);

    layout->addStretch();

    connect(
        signupButton,
        &QPushButton::clicked,
        this,
        &SignupWindow::handleSignup
        );

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            hide();
        }
        );
}

void SignupWindow::handleSignup()
{
    QString name =
        nameEdit->text().trimmed();

    QString email =
        emailEdit->text().trimmed();

    QString phone =
        phoneEdit->text().trimmed();

    QString password =
        passwordEdit->text();

    QString confirmPassword =
        confirmPasswordEdit->text();

    if (name.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Sign Up",
            "Please enter your name."
            );

        return;
    }

    if (email.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Sign Up",
            "Please enter your email."
            );

        return;
    }

    QRegularExpression emailRegex(
        R"(^[\w\.-]+@[\w\.-]+\.\w+$)"
        );

    if (!emailRegex.match(email).hasMatch())
    {
        QMessageBox::warning(
            this,
            "Sign Up",
            "Please enter a valid email."
            );

        return;
    }

    if (phone.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Sign Up",
            "Please enter your phone number."
            );

        return;
    }

    if (password.length() < 6)
    {
        QMessageBox::warning(
            this,
            "Sign Up",
            "Password must contain at least 6 characters."
            );

        return;
    }

    if (password != confirmPassword)
    {
        QMessageBox::warning(
            this,
            "Sign Up",
            "Passwords do not match."
            );

        return;
    }

    Database &database =
        Database::instance();

    if (database.emailExists(email))
    {
        QMessageBox::warning(
            this,
            "Sign Up",
            "An account with this email already exists."
            );

        return;
    }

    bool success =
        database.registerUser(
            name,
            email,
            phone,
            password
            );

    if (success)
    {
        QMessageBox::information(
            this,
            "Account Created",
            "Your account has been created successfully!"
            );

        nameEdit->clear();
        emailEdit->clear();
        phoneEdit->clear();
        passwordEdit->clear();
        confirmPasswordEdit->clear();

        emit signupSuccessful();

        hide();
    }
    else
    {
        QMessageBox::critical(
            this,
            "Error",
            "Unable to create account."
            );
    }
}