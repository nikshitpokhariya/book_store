#include "loginwindow.h"
#include "database.h"
#include "signupwindow.h"
#include "dashboard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("BookBazzar - Login");

    resize(950, 600);
    setMinimumSize(800, 500);

    // =====================================================
    // MAIN LAYOUT
    // =====================================================

    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(0);


    // =====================================================
    // LEFT PANEL
    // =====================================================

    QFrame *leftPanel = new QFrame(this);

    leftPanel->setObjectName("leftPanel");

    QVBoxLayout *leftLayout =
        new QVBoxLayout(leftPanel);

    leftLayout->setContentsMargins(
        45, 45, 45, 45
        );

    leftLayout->setSpacing(15);


    // BookBazzar logo

    logoLabel =
        new QLabel("BookBazzar");

    logoLabel->setObjectName("logoLabel");

    logoLabel->setAlignment(
        Qt::AlignCenter
        );


    // Tagline

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


    // Description

    descriptionLabel =
        new QLabel(
            "Give your books a second chapter.\n\n"
            "Find your next favorite book or sell "
            "the ones you no longer need."
            );

    descriptionLabel->setObjectName(
        "descriptionLabel"
        );

    descriptionLabel->setAlignment(
        Qt::AlignCenter
        );

    descriptionLabel->setWordWrap(true);


    leftLayout->addStretch();

    leftLayout->addWidget(logoLabel);

    leftLayout->addWidget(taglineLabel);

    leftLayout->addSpacing(25);

    leftLayout->addWidget(descriptionLabel);

    leftLayout->addStretch();


    // =====================================================
    // RIGHT PANEL
    // =====================================================

    QFrame *rightPanel = new QFrame(this);

    rightPanel->setObjectName("rightPanel");

    QVBoxLayout *rightLayout =
        new QVBoxLayout(rightPanel);

    rightLayout->setContentsMargins(
        55, 45, 55, 45
        );

    rightLayout->setSpacing(12);


    // =====================================================
    // WELCOME LABEL
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

    emailEdit->setMinimumHeight(45);


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

    passwordEdit->setMinimumHeight(45);


    // =====================================================
    // LOGIN BUTTON
    // =====================================================

    loginButton =
        new QPushButton("Login");

    loginButton->setObjectName(
        "loginButton"
        );

    loginButton->setMinimumHeight(48);


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

    signupButton->setMinimumHeight(40);


    // =====================================================
    // ADD WIDGETS TO RIGHT PANEL
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
    // ADD PANELS TO MAIN LAYOUT
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
    // UI STYLING
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
    // BUTTON CONNECTIONS
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


    // Press Enter in password field to login

    connect(
        passwordEdit,
        &QLineEdit::returnPressed,
        this,
        &LoginWindow::handleLogin
        );


    // Press Enter in email field to move to password

    connect(
        emailEdit,
        &QLineEdit::returnPressed,
        this,
        [this]()
        {
            passwordEdit->setFocus();
        }
        );


    // Start with email field selected

    emailEdit->setFocus();
}


// =========================================================
// DESTRUCTOR
// =========================================================

LoginWindow::~LoginWindow()
{
}


// =========================================================
// LOGIN FUNCTION
// =========================================================

void LoginWindow::handleLogin()
{
    QString email =
        emailEdit->text().trimmed();

    QString password =
        passwordEdit->text();


    // -----------------------------------------------------
    // CHECK EMAIL
    // -----------------------------------------------------

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


    // -----------------------------------------------------
    // CHECK PASSWORD
    // -----------------------------------------------------

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


    // -----------------------------------------------------
    // LOGIN USING DATABASE CLASS
    // -----------------------------------------------------

    Database &database =
        Database::instance();

    bool success =
        database.loginUser(
            email,
            password
            );


    // -----------------------------------------------------
    // LOGIN SUCCESSFUL
    // -----------------------------------------------------

    if (success)
    {
        QString userName =
            database.getUserName(email);


        QMessageBox::information(
            this,
            "Login Successful",
            "Welcome back, " +
                userName +
                "!"
            );


        // Create dashboard

        Dashboard *dashboard =
            new Dashboard();


        // Give dashboard the logged-in user's email

        dashboard->setUser(email);


        // When logout is requested,
        // show the login window again

        connect(
            dashboard,
            &Dashboard::logoutRequested,
            this,
            [this, dashboard]()
            {
                dashboard->close();

                this->show();

                this->raise();

                this->activateWindow();

                emailEdit->clear();

                passwordEdit->clear();

                emailEdit->setFocus();
            }
            );


        // Show dashboard

        dashboard->show();


        // Hide login window

        this->hide();

        return;
    }


    // -----------------------------------------------------
    // LOGIN FAILED
    // -----------------------------------------------------

    QMessageBox::warning(
        this,
        "Login Failed",
        "Incorrect email or password."
        );


    passwordEdit->clear();

    passwordEdit->setFocus();
}


// =========================================================
// OPEN SIGNUP WINDOW
// =========================================================

void LoginWindow::openSignup()
{
    SignupWindow *signupWindow =
        new SignupWindow();


    // Delete signup window when it is closed

    signupWindow->setAttribute(
        Qt::WA_DeleteOnClose
        );


    // Hide login

    this->hide();


    // -----------------------------------------------------
    // WHEN SIGNUP IS SUCCESSFUL
    // -----------------------------------------------------

    connect(
        signupWindow,
        &SignupWindow::signupSuccessful,
        this,
        [this]()
        {
            this->show();

            this->raise();

            this->activateWindow();

            emailEdit->setFocus();
        }
        );


    // -----------------------------------------------------
    // SHOW SIGNUP WINDOW
    // -----------------------------------------------------

    signupWindow->show();

    signupWindow->raise();

    signupWindow->activateWindow();
}