#include "loginwindow.h"

#include "signupwindow.h"
#include "homewindow.h"
#include "browsewindow.h"
#include "bookdetailswindow.h"
#include "sellwindow.h"
#include "sellbookwindow.h"
#include "listingswindow.h"
#include "orderswindow.h"
#include "windows/CartWindow.h"
#include "windows/CheckoutWindow.h"
#include "windows/OrderDetailWindow.h"
#include "windows/ExchangeWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include "AppStyle.h"
#include "StyledMessageBox.h"

#include <QJsonObject>
#include <QNetworkReply>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent),
    emailEdit(nullptr),
    passwordEdit(nullptr),
    loginButton(nullptr),
    signupButton(nullptr),
    logoLabel(nullptr),
    taglineLabel(nullptr),
    descriptionLabel(nullptr),
    welcomeLabel(nullptr),
    loginSubtitleLabel(nullptr),
    accountLabel(nullptr)
{
    setWindowTitle("BookBazzar - Login");
    resize(980, 620);
    setMinimumSize(850, 520);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(0);

    // Left Panel (Brand / Value presentation)
    QFrame *leftPanel = new QFrame(this);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setStyleSheet(R"(
        QFrame#leftPanel {
            background-color: #0F172A;
            border-top-left-radius: 16px;
            border-bottom-left-radius: 16px;
        }
        QLabel {
            background: transparent;
            border: none;
        }
    )");

    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(48, 48, 48, 48);
    leftLayout->setSpacing(16);

    QLabel *badge = new QLabel("COMMUNITY MARKETPLACE");
    badge->setStyleSheet("background: transparent; border: none; color: #818CF8; font-size: 11px; font-weight: 800; letter-spacing: 1px;");

    logoLabel = new QLabel("BookBazzar");
    logoLabel->setStyleSheet("background: transparent; border: none; color: #FFFFFF; font-size: 32px; font-weight: 800;");

    taglineLabel = new QLabel("Buy, sell, and exchange books with readers everywhere.");
    taglineLabel->setStyleSheet("background: transparent; border: none; color: #94A3B8; font-size: 15px; line-height: 1.4;");
    taglineLabel->setWordWrap(true);

    QLabel *bullet1 = new QLabel("✓  Direct peer-to-peer textbook & novel exchange");
    bullet1->setStyleSheet("background: transparent; border: none; color: #CBD5E1; font-size: 13px; font-weight: 600;");
    QLabel *bullet2 = new QLabel("✓  Instant order tracking and delivery status");
    bullet2->setStyleSheet("background: transparent; border: none; color: #CBD5E1; font-size: 13px; font-weight: 600;");
    QLabel *bullet3 = new QLabel("✓  Verified reader community & authentic reviews");
    bullet3->setStyleSheet("background: transparent; border: none; color: #CBD5E1; font-size: 13px; font-weight: 600;");

    leftLayout->addStretch();
    leftLayout->addWidget(badge);
    leftLayout->addWidget(logoLabel);
    leftLayout->addWidget(taglineLabel);
    leftLayout->addSpacing(28);
    leftLayout->addWidget(bullet1);
    leftLayout->addWidget(bullet2);
    leftLayout->addWidget(bullet3);
    leftLayout->addStretch();

    // Right Panel (Form)
    QFrame *rightPanel = new QFrame(this);
    rightPanel->setObjectName("rightPanel");
    rightPanel->setStyleSheet(R"(
        QFrame#rightPanel {
            background-color: #FFFFFF;
            border-top-right-radius: 16px;
            border-bottom-right-radius: 16px;
            border: 1px solid #E2E8F0;
            border-left: none;
        }
        QLabel {
            background: transparent;
            border: none;
        }
    )");

    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(56, 48, 56, 48);
    rightLayout->setSpacing(12);

    welcomeLabel = new QLabel("Welcome Back");
    welcomeLabel->setStyleSheet("color: #0F172A; font-size: 26px; font-weight: 800;");

    loginSubtitleLabel = new QLabel("Enter your email and password to access your account");
    loginSubtitleLabel->setStyleSheet("color: #64748B; font-size: 13px;");

    QLabel *emailTag = new QLabel("Email Address");
    emailTag->setStyleSheet("color: #334155; font-size: 13px; font-weight: 600;");

    emailEdit = new QLineEdit();
    emailEdit->setObjectName("emailEdit");
    emailEdit->setPlaceholderText("name@example.com");
    emailEdit->setStyleSheet(AppStyle::inputStyle());
    emailEdit->setMinimumHeight(44);

    QLabel *passwordTag = new QLabel("Password");
    passwordTag->setStyleSheet("color: #334155; font-size: 13px; font-weight: 600;");

    passwordEdit = new QLineEdit();
    passwordEdit->setObjectName("passwordEdit");
    passwordEdit->setPlaceholderText("••••••••");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setStyleSheet(AppStyle::inputStyle());
    passwordEdit->setMinimumHeight(44);

    loginButton = new QPushButton("Sign In");
    loginButton->setObjectName("loginButton");
    loginButton->setStyleSheet(AppStyle::primaryButtonStyle());
    loginButton->setMinimumHeight(46);
    loginButton->setCursor(Qt::PointingHandCursor);

    QHBoxLayout *signupPrompt = new QHBoxLayout();
    accountLabel = new QLabel("Don't have an account?");
    accountLabel->setStyleSheet("color: #64748B; font-size: 13px;");

    signupButton = new QPushButton("Create Account");
    signupButton->setStyleSheet(AppStyle::ghostButtonStyle());
    signupButton->setCursor(Qt::PointingHandCursor);

    signupPrompt->addStretch();
    signupPrompt->addWidget(accountLabel);
    signupPrompt->addWidget(signupButton);
    signupPrompt->addStretch();

    rightLayout->addStretch();
    rightLayout->addWidget(welcomeLabel);
    rightLayout->addWidget(loginSubtitleLabel);
    rightLayout->addSpacing(16);
    rightLayout->addWidget(emailTag);
    rightLayout->addWidget(emailEdit);
    rightLayout->addSpacing(4);
    rightLayout->addWidget(passwordTag);
    rightLayout->addWidget(passwordEdit);
    rightLayout->addSpacing(12);
    rightLayout->addWidget(loginButton);
    rightLayout->addSpacing(8);
    rightLayout->addLayout(signupPrompt);
    rightLayout->addStretch();

    mainLayout->addWidget(leftPanel, 1);
    mainLayout->addWidget(rightPanel, 1);

    AppStyle::applyElevation(leftPanel, 30, 8, 30);
    AppStyle::applyElevation(rightPanel, 30, 8, 30);

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    connect(signupButton, &QPushButton::clicked, this, &LoginWindow::openSignup);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginWindow::handleLogin);
    connect(emailEdit, &QLineEdit::returnPressed, this, [this]() {
        passwordEdit->setFocus();
    });

    emailEdit->setFocus();
}

LoginWindow::~LoginWindow()
{
}

void LoginWindow::handleLogin()
{
    QString email = emailEdit->text().trimmed();
    QString password = passwordEdit->text();

    if (email.isEmpty()) {
        StyledMessageBox::warning(this, "Login", "Please enter your email.");
        emailEdit->setFocus();
        return;
    }

    if (password.isEmpty()) {
        StyledMessageBox::warning(this, "Login", "Please enter your password.");
        passwordEdit->setFocus();
        return;
    }

    loginButton->setEnabled(false);
    loginButton->setText("Signing In...");

    QJsonObject body;
    body[QStringLiteral("email")] = email;
    body[QStringLiteral("password")] = password;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/auth/login"), body, false);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        loginButton->setEnabled(true);
        loginButton->setText("Sign In");

        if (ok) {
            const QString token = json[QStringLiteral("accessToken")].toString();
            const qint64 expiresIn = json.value(QStringLiteral("expiresIn")).toInteger(900);
            const QJsonObject userObj = json[QStringLiteral("user")].toObject();
            const QString userId = userObj[QStringLiteral("id")].toString();
            const QString userName = userObj[QStringLiteral("username")].toString();

            SessionManager::instance().setSession(token, expiresIn, userId, userName);

            HomeWindow *homeWindow = new HomeWindow(userName);
            homeWindow->setAttribute(Qt::WA_DeleteOnClose);

            // LOGOUT
            connect(homeWindow, &HomeWindow::logoutRequested, this, [this, homeWindow]() {
                homeWindow->close();
                this->show();
                this->raise();
                this->activateWindow();
                emailEdit->clear();
                passwordEdit->clear();
                emailEdit->setFocus();
            });

            homeWindow->show();
            homeWindow->raise();
            homeWindow->activateWindow();
            this->hide();
        } else {
            StyledMessageBox::warning(
                this,
                "Login Failed",
                errorMsg.isEmpty() ? "Incorrect email or password." : errorMsg
            );
            passwordEdit->clear();
            passwordEdit->setFocus();
        }
    });
}

void LoginWindow::openSignup()
{
    SignupWindow *signupWindow = new SignupWindow();
    signupWindow->setAttribute(Qt::WA_DeleteOnClose);

    this->hide();

    connect(signupWindow, &SignupWindow::signupSuccessful, this, [this]() {
        this->show();
        this->raise();
        this->activateWindow();
        emailEdit->clear();
        passwordEdit->clear();
        emailEdit->setFocus();
    });

    connect(signupWindow, &SignupWindow::backRequested, this, [this]() {
        this->show();
        this->raise();
        this->activateWindow();
    });

    signupWindow->show();
    signupWindow->raise();
    signupWindow->activateWindow();
}