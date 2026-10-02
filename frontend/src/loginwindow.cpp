#include "loginwindow.h"

#include "signupwindow.h"
#include "homewindow.h"
#include "browsewindow.h"
#include "bookdetailswindow.h"
#include "sellwindow.h"
#include "listingswindow.h"
#include "orderswindow.h"
#include "windows/CartWindow.h"
#include "windows/CheckoutWindow.h"
#include "windows/OrderDetailWindow.h"
#include "windows/ExchangeWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include <QJsonObject>
#include <QNetworkReply>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QMessageBox>


// =========================================================
// CONSTRUCTOR
// =========================================================

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

    resize(950, 600);

    setMinimumSize(800, 500);


    // =====================================================
    // MAIN LAYOUT
    // =====================================================

    QHBoxLayout *mainLayout =
        new QHBoxLayout(this);

    mainLayout->setContentsMargins(
        30, 30, 30, 30
        );

    mainLayout->setSpacing(0);


    // =====================================================
    // LEFT PANEL
    // =====================================================

    QFrame *leftPanel =
        new QFrame(this);

    leftPanel->setObjectName(
        "leftPanel"
        );

    QVBoxLayout *leftLayout =
        new QVBoxLayout(leftPanel);

    leftLayout->setContentsMargins(
        45, 45, 45, 45
        );

    leftLayout->setSpacing(15);


    // =====================================================
    // LOGO
    // =====================================================

    logoLabel =
        new QLabel("BookBazzar");

    logoLabel->setObjectName(
        "logoLabel"
        );

    logoLabel->setAlignment(
        Qt::AlignCenter
        );


    // =====================================================
    // TAGLINE
    // =====================================================

    taglineLabel =
        new QLabel(
            "Buy • Sell • Discover Books"
            );

    taglineLabel->setObjectName(
        "taglineLabel"
        );

    taglineLabel->setAlignment(
        Qt::AlignCenter
        );


    // =====================================================
    // DESCRIPTION
    // =====================================================

    descriptionLabel =
        new QLabel(
            "Give your books a second chapter.\n\n"
            "Find affordable books from other readers "
            "or sell the books you no longer need."
            );

    descriptionLabel->setObjectName(
        "descriptionLabel"
        );

    descriptionLabel->setAlignment(
        Qt::AlignCenter
        );

    descriptionLabel->setWordWrap(
        true
        );


    leftLayout->addStretch();

    leftLayout->addWidget(
        logoLabel
        );

    leftLayout->addWidget(
        taglineLabel
        );

    leftLayout->addSpacing(25);

    leftLayout->addWidget(
        descriptionLabel
        );

    leftLayout->addStretch();


    // =====================================================
    // RIGHT PANEL
    // =====================================================

    QFrame *rightPanel =
        new QFrame(this);

    rightPanel->setObjectName(
        "rightPanel"
        );

    QVBoxLayout *rightLayout =
        new QVBoxLayout(rightPanel);

    rightLayout->setContentsMargins(
        55, 45, 55, 45
        );

    rightLayout->setSpacing(12);


    // =====================================================
    // WELCOME
    // =====================================================

    welcomeLabel =
        new QLabel("Welcome Back!");

    welcomeLabel->setObjectName(
        "welcomeLabel"
        );

    welcomeLabel->setAlignment(
        Qt::AlignCenter
        );


    // =====================================================
    // SUBTITLE
    // =====================================================

    loginSubtitleLabel =
        new QLabel(
            "Login to continue to BookBazzar"
            );

    loginSubtitleLabel->setObjectName(
        "loginSubtitleLabel"
        );

    loginSubtitleLabel->setAlignment(
        Qt::AlignCenter
        );


    // =====================================================
    // EMAIL
    // =====================================================

    emailEdit =
        new QLineEdit();

    emailEdit->setObjectName(
        "emailEdit"
        );

    emailEdit->setPlaceholderText(
        "Enter your email"
        );

    emailEdit->setMinimumHeight(
        45
        );


    // =====================================================
    // PASSWORD
    // =====================================================

    passwordEdit =
        new QLineEdit();

    passwordEdit->setObjectName(
        "passwordEdit"
        );

    passwordEdit->setPlaceholderText(
        "Enter your password"
        );

    passwordEdit->setEchoMode(
        QLineEdit::Password
        );

    passwordEdit->setMinimumHeight(
        45
        );


    // =====================================================
    // LOGIN BUTTON
    // =====================================================

    loginButton =
        new QPushButton("Login");

    loginButton->setObjectName(
        "loginButton"
        );

    loginButton->setMinimumHeight(
        48
        );


    // =====================================================
    // ACCOUNT LABEL
    // =====================================================

    accountLabel =
        new QLabel(
            "Don't have an account?"
            );

    accountLabel->setObjectName(
        "accountLabel"
        );

    accountLabel->setAlignment(
        Qt::AlignCenter
        );


    // =====================================================
    // SIGNUP BUTTON
    // =====================================================

    signupButton =
        new QPushButton(
            "Create New Account"
            );

    signupButton->setObjectName(
        "signupButton"
        );

    signupButton->setMinimumHeight(
        40
        );


    // =====================================================
    // ADD WIDGETS
    // =====================================================

    rightLayout->addStretch();

    rightLayout->addWidget(
        welcomeLabel
        );

    rightLayout->addWidget(
        loginSubtitleLabel
        );

    rightLayout->addSpacing(20);

    rightLayout->addWidget(
        emailEdit
        );

    rightLayout->addWidget(
        passwordEdit
        );

    rightLayout->addSpacing(8);

    rightLayout->addWidget(
        loginButton
        );

    rightLayout->addSpacing(15);

    rightLayout->addWidget(
        accountLabel
        );

    rightLayout->addWidget(
        signupButton
        );

    rightLayout->addStretch();


    // =====================================================
    // ADD PANELS
    // =====================================================

    mainLayout->addWidget(
        leftPanel,
        1
        );

    mainLayout->addWidget(
        rightPanel,
        1
        );


    // =====================================================
    // STYLING
    // =====================================================

    setStyleSheet(R"(

        QWidget {
            font-family: "Segoe UI";
        }

        #leftPanel {
            background-color: #1E293B;
            border-top-left-radius: 20px;
            border-bottom-left-radius: 20px;
        }

        #rightPanel {
            background-color: white;
            border-top-right-radius: 20px;
            border-bottom-right-radius: 20px;
        }

        #logoLabel {
            color: white;
            font-size: 36px;
            font-weight: bold;
        }

        #taglineLabel {
            color: #CBD5E1;
            font-size: 18px;
        }

        #descriptionLabel {
            color: #CBD5E1;
            font-size: 15px;
        }

        #welcomeLabel {
            color: #0F172A;
            font-size: 30px;
            font-weight: bold;
        }

        #loginSubtitleLabel {
            color: #64748B;
            font-size: 15px;
        }

        #emailEdit,
        #passwordEdit {

            background-color: #F8FAFC;
            color: #0F172A;

            border: 1px solid #CBD5E1;
            border-radius: 10px;

            padding-left: 14px;
            padding-right: 14px;

            font-size: 15px;
        }

        #emailEdit:focus,
        #passwordEdit:focus {

            border: 2px solid #2563EB;
            background-color: white;
        }

        #loginButton {

            background-color: #2563EB;
            color: white;

            border: none;
            border-radius: 10px;

            font-size: 16px;
            font-weight: bold;
        }

        #loginButton:hover {
            background-color: #1D4ED8;
        }

        #loginButton:pressed {
            background-color: #1E40AF;
        }

        #signupButton {

            background-color: transparent;
            color: #2563EB;

            border: none;

            font-size: 15px;
            font-weight: bold;
        }

        #signupButton:hover {
            color: #1D4ED8;
        }

        #accountLabel {
            color: #64748B;
            font-size: 14px;
        }

    )");


    // =====================================================
    // CONNECTIONS
    // =====================================================

    connect(
        loginButton,
        &QPushButton::clicked,
        this,
        &LoginWindow::handleLogin
        );


    connect(
        signupButton,
        &QPushButton::clicked,
        this,
        &LoginWindow::openSignup
        );


    connect(
        passwordEdit,
        &QLineEdit::returnPressed,
        this,
        &LoginWindow::handleLogin
        );


    connect(
        emailEdit,
        &QLineEdit::returnPressed,
        this,
        [this]()
        {
            passwordEdit->setFocus();
        }
        );


    emailEdit->setFocus();
}


// =========================================================
// DESTRUCTOR
// =========================================================

LoginWindow::~LoginWindow()
{
}


// =========================================================
// LOGIN
// =========================================================

void LoginWindow::handleLogin()
{
    QString email =
        emailEdit->text().trimmed();

    QString password =
        passwordEdit->text();


    // =====================================================
    // VALIDATION
    // =====================================================

    if (email.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Login",
            "Please enter your email."
            );

        emailEdit->setFocus();

        return;
    }


    if (password.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Login",
            "Please enter your password."
            );

        passwordEdit->setFocus();

        return;
    }


    loginButton->setEnabled(false);
    loginButton->setText("Signing In...");

    QJsonObject body;
    body[QStringLiteral("email")] = email;
    body[QStringLiteral("password")] = password;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/auth/login"), body, false);
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        loginButton->setEnabled(true);
        loginButton->setText("Sign In");

        if (ok)
        {
            const QString token = json[QStringLiteral("accessToken")].toString();
            const qint64 expiresIn = json.value(QStringLiteral("expiresIn")).toInteger(900);
            const QJsonObject userObj = json[QStringLiteral("user")].toObject();
            const QString userId = userObj[QStringLiteral("id")].toString();
            const QString userName = userObj[QStringLiteral("username")].toString();

            SessionManager::instance().setSession(token, expiresIn, userId, userName);

            // =================================================
            // CREATE HOME WINDOW
            // =================================================

            HomeWindow *homeWindow =
                new HomeWindow(
                    userName
                    );

            homeWindow->setAttribute(
                Qt::WA_DeleteOnClose
                );


            // =================================================
            // LOGOUT
            // =================================================

            connect(
                homeWindow,
                &HomeWindow::logoutRequested,
                this,
                [this, homeWindow]()
                {
                    homeWindow->close();

                    this->show();

                    this->raise();

                    this->activateWindow();

                    emailEdit->clear();

                    passwordEdit->clear();

                    emailEdit->setFocus();
                }
                );


            // =================================================
            // BROWSE BOOKS
            // =================================================

            connect(
                homeWindow,
                &HomeWindow::browseBooksRequested,
                this,
                [homeWindow]()
                {
                    BrowseWindow *browseWindow =
                        new BrowseWindow(
                            homeWindow->windowTitle()
                            );

                    browseWindow->setAttribute(
                        Qt::WA_DeleteOnClose
                        );


                    connect(
                        browseWindow,
                        &BrowseWindow::backRequested,
                        browseWindow,
                        [browseWindow]()
                        {
                            browseWindow->close();
                        }
                        );

                    connect(
                        browseWindow,
                        &BrowseWindow::bookSelected,
                        browseWindow,
                        [homeWindow](const QString &bookId)
                        {
                            BookDetailsWindow *details =
                                new BookDetailsWindow(
                                    bookId,
                                    homeWindow->windowTitle()
                                    );

                            details->setAttribute(
                                Qt::WA_DeleteOnClose
                                );

                            connect(
                                details,
                                &BookDetailsWindow::backRequested,
                                details,
                                &QWidget::close
                                );

                            details->show();
                            details->raise();
                            details->activateWindow();
                        }
                        );

                    browseWindow->show();

                    browseWindow->raise();

                    browseWindow->activateWindow();
                }
                );


            // =================================================
            // SELL BOOK
            // =================================================

            connect(
                homeWindow,
                &HomeWindow::sellBookRequested,
                this,
                [homeWindow, userName]()
                {
                    SellWindow *sellWindow =
                        new SellWindow(
                            userName,
                            homeWindow
                            );


                    sellWindow->setAttribute(
                        Qt::WA_DeleteOnClose
                        );


                    // -----------------------------------------
                    // BACK BUTTON
                    // -----------------------------------------

                    connect(
                        sellWindow,
                        &SellWindow::backRequested,
                        sellWindow,
                        [sellWindow]()
                        {
                            sellWindow->close();
                        }
                        );


                    // -----------------------------------------
                    // BOOK ADDED
                    // -----------------------------------------

                    connect(
                        sellWindow,
                        &SellWindow::bookAdded,
                        homeWindow,
                        [homeWindow]()
                        {
                            homeWindow->refreshRecentBooks();
                        }
                        );


                    // -----------------------------------------
                    // SHOW SELL WINDOW
                    // -----------------------------------------

                    sellWindow->show();

                    sellWindow->raise();

                    sellWindow->activateWindow();
                }
                );


            // =================================================
            // MY LISTINGS
            // =================================================
            connect(
                homeWindow,
                &HomeWindow::listingsRequested,
                this,
                [homeWindow, userName]()
                {
                    ListingsWindow *listings = new ListingsWindow(userName, homeWindow);
                    listings->setAttribute(Qt::WA_DeleteOnClose);
                    connect(listings, &ListingsWindow::backRequested, listings, &QWidget::close);
                    connect(listings, &ListingsWindow::addNewListingRequested, homeWindow, &HomeWindow::handleSellBook);
                    listings->show();
                    listings->raise();
                    listings->activateWindow();
                }
                );


            // =================================================
            // MY ORDERS
            // =================================================
            connect(
                homeWindow,
                &HomeWindow::ordersRequested,
                this,
                [homeWindow, userName]()
                {
                    OrdersWindow *orders = new OrdersWindow(userName, homeWindow);
                    orders->setAttribute(Qt::WA_DeleteOnClose);
                    connect(orders, &OrdersWindow::backRequested, orders, &QWidget::close);
                    connect(orders, &OrdersWindow::browseRequested, homeWindow, &HomeWindow::handleBrowseBooks);
                    orders->show();
                    orders->raise();
                    orders->activateWindow();
                }
                );


            // =================================================
            // SHOPPING CART
            // =================================================
            connect(
                homeWindow,
                &HomeWindow::cartRequested,
                this,
                [homeWindow]()
                {
                    CartWindow *cartWin = new CartWindow(homeWindow);
                    cartWin->setAttribute(Qt::WA_DeleteOnClose);
                    connect(cartWin, &CartWindow::backRequested, cartWin, &QWidget::close);
                    connect(cartWin, &CartWindow::browseRequested, [cartWin, homeWindow]() {
                        cartWin->close();
                        homeWindow->handleBrowseBooks();
                    });
                    connect(cartWin, &CartWindow::cartCountChanged, homeWindow, &HomeWindow::updateCartBadge);
                    connect(cartWin, &CartWindow::checkoutRequested, [cartWin, homeWindow]() {
                        CheckoutWindow *checkout = new CheckoutWindow(homeWindow);
                        checkout->setAttribute(Qt::WA_DeleteOnClose);
                        connect(checkout, &CheckoutWindow::backRequested, [checkout, cartWin]() {
                            checkout->close();
                            cartWin->show();
                            cartWin->raise();
                            cartWin->activateWindow();
                        });
                        connect(checkout, &CheckoutWindow::orderPlaced, [checkout, cartWin, homeWindow](const QString &orderId) {
                            checkout->close();
                            cartWin->close();
                            homeWindow->refreshCartCount();

                            OrderDetailWindow *detail = new OrderDetailWindow(orderId, homeWindow);
                            detail->setAttribute(Qt::WA_DeleteOnClose);
                            connect(detail, &OrderDetailWindow::backRequested, detail, &QWidget::close);
                            detail->show();
                            detail->raise();
                            detail->activateWindow();
                        });
                        cartWin->hide();
                        checkout->show();
                        checkout->raise();
                        checkout->activateWindow();
                    });

                    cartWin->show();
                    cartWin->raise();
                    cartWin->activateWindow();
                }
                );


            // =================================================
            // EXCHANGES
            // =================================================
            connect(
                homeWindow,
                &HomeWindow::exchangesRequested,
                this,
                [homeWindow]()
                {
                    ExchangeWindow *exWin = new ExchangeWindow(homeWindow);
                    exWin->setAttribute(Qt::WA_DeleteOnClose);
                    connect(exWin, &ExchangeWindow::backRequested, exWin, &QWidget::close);
                    exWin->show();
                    exWin->raise();
                    exWin->activateWindow();
                }
                );


            // =================================================
            // SHOW HOME WINDOW
            // =================================================

            homeWindow->show();

            homeWindow->raise();

            homeWindow->activateWindow();


            // =================================================
            // HIDE LOGIN WINDOW
            // =================================================

            this->hide();
        }
        else
        {
            QMessageBox::warning(
                this,
                "Login Failed",
                errorMsg.isEmpty() ? "Incorrect email or password." : errorMsg
                );

            passwordEdit->clear();
            passwordEdit->setFocus();
        }
    });
}


// =========================================================
// SIGNUP
// =========================================================

void LoginWindow::openSignup()
{
    SignupWindow *signupWindow =
        new SignupWindow();


    signupWindow->setAttribute(
        Qt::WA_DeleteOnClose
        );


    // =====================================================
    // HIDE LOGIN
    // =====================================================

    this->hide();


    // =====================================================
    // SIGNUP SUCCESSFUL
    // =====================================================

    connect(
        signupWindow,
        &SignupWindow::signupSuccessful,
        this,
        [this]()
        {
            this->show();

            this->raise();

            this->activateWindow();

            emailEdit->clear();

            passwordEdit->clear();

            emailEdit->setFocus();
        }
        );


    // =====================================================
    // SHOW SIGNUP
    // =====================================================

    signupWindow->show();

    signupWindow->raise();

    signupWindow->activateWindow();
}