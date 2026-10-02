#include "signupwindow.h"
#include "network/ApiClient.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QFont>
#include <QRegularExpression>
#include <QJsonObject>
#include <QNetworkReply>

SignupWindow::SignupWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("BookBazzar - Sign Up");
    resize(450, 560);

    QLabel *title = new QLabel("Create Account");
    QFont titleFont;
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);

    usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("Username (3–30 characters)");

    emailEdit = new QLineEdit();
    emailEdit->setPlaceholderText("Email Address");

    passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("Password (at least 8 characters)");
    passwordEdit->setEchoMode(QLineEdit::Password);

    confirmPasswordEdit = new QLineEdit();
    confirmPasswordEdit->setPlaceholderText("Confirm Password");
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);

    signupButton = new QPushButton("Create Account");
    backButton = new QPushButton("Back to Login");

    signupButton->setMinimumHeight(45);
    backButton->setMinimumHeight(45);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(50, 35, 50, 35);

    layout->addWidget(title);
    layout->addSpacing(15);
    layout->addWidget(usernameEdit);
    layout->addWidget(emailEdit);
    layout->addWidget(passwordEdit);
    layout->addWidget(confirmPasswordEdit);
    layout->addSpacing(10);
    layout->addWidget(signupButton);
    layout->addWidget(backButton);
    layout->addStretch();

    connect(signupButton, &QPushButton::clicked, this, &SignupWindow::handleSignup);
    connect(backButton, &QPushButton::clicked, this, [this]() {
        hide();
    });
}

void SignupWindow::handleSignup()
{
    const QString username = usernameEdit->text().trimmed();
    const QString email = emailEdit->text().trimmed();
    const QString password = passwordEdit->text();
    const QString confirmPassword = confirmPasswordEdit->text();

    if (username.length() < 3 || username.length() > 30) {
        QMessageBox::warning(this, "Sign Up", "Username must be between 3 and 30 characters.");
        usernameEdit->setFocus();
        return;
    }

    if (email.isEmpty()) {
        QMessageBox::warning(this, "Sign Up", "Please enter your email.");
        emailEdit->setFocus();
        return;
    }

    static const QRegularExpression emailRegex(QStringLiteral(R"(^[\w\.-]+@[\w\.-]+\.\w+$)"));
    if (!emailRegex.match(email).hasMatch()) {
        QMessageBox::warning(this, "Sign Up", "Please enter a valid email address.");
        emailEdit->setFocus();
        return;
    }

    if (password.length() < 8) {
        QMessageBox::warning(this, "Sign Up", "Password must contain at least 8 characters.");
        passwordEdit->setFocus();
        return;
    }

    if (password != confirmPassword) {
        QMessageBox::warning(this, "Sign Up", "Passwords do not match.");
        confirmPasswordEdit->setFocus();
        return;
    }

    signupButton->setEnabled(false);
    signupButton->setText("Creating Account...");

    QJsonObject body;
    body[QStringLiteral("username")] = username;
    body[QStringLiteral("email")] = email;
    body[QStringLiteral("password")] = password;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/auth/signup"), body, false);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        signupButton->setEnabled(true);
        signupButton->setText("Create Account");

        if (ok) {
            QMessageBox::information(
                this,
                "Account Created",
                "Your account has been created successfully! Please log in."
            );

            usernameEdit->clear();
            emailEdit->clear();
            passwordEdit->clear();
            confirmPasswordEdit->clear();

            emit signupSuccessful();
            hide();
        } else {
            QString displayErr = errorMsg.isEmpty() ? "Unable to create account." : errorMsg;
            QMessageBox::warning(this, "Sign Up Failed", displayErr);
        }
    });
}