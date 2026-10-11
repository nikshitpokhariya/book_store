#include "signupwindow.h"
#include "network/ApiClient.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "StyledMessageBox.h"
#include <QFrame>
#include <QRegularExpression>
#include <QJsonObject>
#include <QNetworkReply>

SignupWindow::SignupWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("BookBazzar - Create Account");
    resize(480, 620);
    setMinimumSize(420, 560);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);

    QFrame *card = new QFrame(this);
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 24, 6, 20);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(36, 36, 36, 36);
    layout->setSpacing(14);

    QLabel *brand = new QLabel("BookBazzar");
    brand->setStyleSheet(QString("background: transparent; border: none; color: %1; font-size: 14px; font-weight: 700;").arg(AppStyle::Primary));
    brand->setAlignment(Qt::AlignCenter);

    QLabel *title = new QLabel("Create Account");
    title->setStyleSheet(QString("background: transparent; border: none; color: %1; font-size: 24px; font-weight: 800;").arg(AppStyle::TextPrimary));
    title->setAlignment(Qt::AlignCenter);

    QLabel *subtitle = new QLabel("Join our community of book lovers today");
    subtitle->setStyleSheet(QString("background: transparent; border: none; color: %1; font-size: 13px;").arg(AppStyle::TextSecondary));
    subtitle->setAlignment(Qt::AlignCenter);

    usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("Username (3–30 characters)");
    usernameEdit->setStyleSheet(AppStyle::inputStyle());
    usernameEdit->setMinimumHeight(44);

    emailEdit = new QLineEdit();
    emailEdit->setPlaceholderText("Email Address");
    emailEdit->setStyleSheet(AppStyle::inputStyle());
    emailEdit->setMinimumHeight(44);

    passwordEdit = new QLineEdit();
    passwordEdit->setPlaceholderText("Password (at least 8 characters)");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setStyleSheet(AppStyle::inputStyle());
    passwordEdit->setMinimumHeight(44);

    confirmPasswordEdit = new QLineEdit();
    confirmPasswordEdit->setPlaceholderText("Confirm Password");
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit->setStyleSheet(AppStyle::inputStyle());
    confirmPasswordEdit->setMinimumHeight(44);

    signupButton = new QPushButton("Create Account");
    signupButton->setStyleSheet(AppStyle::primaryButtonStyle());
    signupButton->setMinimumHeight(44);
    signupButton->setCursor(Qt::PointingHandCursor);

    backButton = new QPushButton("Back to Login");
    backButton->setStyleSheet(AppStyle::secondaryButtonStyle());
    backButton->setMinimumHeight(42);
    backButton->setCursor(Qt::PointingHandCursor);

    layout->addWidget(brand);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(12);
    layout->addWidget(usernameEdit);
    layout->addWidget(emailEdit);
    layout->addWidget(passwordEdit);
    layout->addWidget(confirmPasswordEdit);
    layout->addSpacing(8);
    layout->addWidget(signupButton);
    layout->addWidget(backButton);
    layout->addStretch();

    mainLayout->addWidget(card);

    connect(signupButton, &QPushButton::clicked, this, &SignupWindow::handleSignup);
    connect(backButton, &QPushButton::clicked, this, [this]() {
        emit backRequested();
        close();
    });
}

void SignupWindow::handleSignup()
{
    const QString username = usernameEdit->text().trimmed();
    const QString email = emailEdit->text().trimmed();
    const QString password = passwordEdit->text();
    const QString confirmPassword = confirmPasswordEdit->text();

    if (username.length() < 3 || username.length() > 30) {
        StyledMessageBox::warning(this, "Sign Up", "Username must be between 3 and 30 characters.");
        usernameEdit->setFocus();
        return;
    }

    if (email.isEmpty()) {
        StyledMessageBox::warning(this, "Sign Up", "Please enter your email.");
        emailEdit->setFocus();
        return;
    }

    static const QRegularExpression emailRegex(QStringLiteral(R"(^[\w\.-]+@[\w\.-]+\.\w+$)"));
    if (!emailRegex.match(email).hasMatch()) {
        StyledMessageBox::warning(this, "Sign Up", "Please enter a valid email address.");
        emailEdit->setFocus();
        return;
    }

    if (password.length() < 8) {
        StyledMessageBox::warning(this, "Sign Up", "Password must contain at least 8 characters.");
        passwordEdit->setFocus();
        return;
    }

    if (password != confirmPassword) {
        StyledMessageBox::warning(this, "Sign Up", "Passwords do not match.");
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
            StyledMessageBox::success(
                this,
                "Account Created",
                "Your account has been created successfully! Please log in."
            );

            usernameEdit->clear();
            emailEdit->clear();
            passwordEdit->clear();
            confirmPasswordEdit->clear();

            emit signupSuccessful();
            close();
        } else {
            QString displayErr = errorMsg.isEmpty() ? "Unable to create account." : errorMsg;
            StyledMessageBox::warning(this, "Sign Up Failed", displayErr);
        }
    });
}