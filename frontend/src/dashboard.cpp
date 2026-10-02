#include "dashboard.h"

#include "database.h"
#include "sellwindow.h"
#include "browsewindow.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QMessageBox>


// =========================================================
// CONSTRUCTOR
// =========================================================

Dashboard::Dashboard(
    QWidget *parent
    )
    : QWidget(parent),
    welcomeLabel(nullptr),
    emailLabel(nullptr),
    buyButton(nullptr),
    sellButton(nullptr),
    profileButton(nullptr),
    logoutButton(nullptr),
    sellWindow(nullptr),
    browseWindow(nullptr)
{
    setWindowTitle(
        "BookBazzar - Dashboard"
        );

    resize(
        600,
        500
        );


    // =====================================================
    // TITLE
    // =====================================================

    QLabel *title =
        new QLabel(
            "BOOKBAZZAR"
            );


    QFont titleFont;

    titleFont.setPointSize(
        28
        );

    titleFont.setBold(
        true
        );

    title->setFont(
        titleFont
        );

    title->setAlignment(
        Qt::AlignCenter
        );


    // =====================================================
    // WELCOME
    // =====================================================

    welcomeLabel =
        new QLabel(
            "Welcome!"
            );


    QFont welcomeFont;

    welcomeFont.setPointSize(
        20
        );

    welcomeFont.setBold(
        true
        );

    welcomeLabel->setFont(
        welcomeFont
        );

    welcomeLabel->setAlignment(
        Qt::AlignCenter
        );


    // =====================================================
    // EMAIL
    // =====================================================

    emailLabel =
        new QLabel();


    emailLabel->setAlignment(
        Qt::AlignCenter
        );


    // =====================================================
    // BUTTONS
    // =====================================================

    buyButton =
        new QPushButton(
            "Browse Books"
            );


    sellButton =
        new QPushButton(
            "Sell Books"
            );


    profileButton =
        new QPushButton(
            "My Profile"
            );


    logoutButton =
        new QPushButton(
            "Logout"
            );


    buyButton->setMinimumHeight(
        50
        );

    sellButton->setMinimumHeight(
        50
        );

    profileButton->setMinimumHeight(
        50
        );

    logoutButton->setMinimumHeight(
        50
        );


    // =====================================================
    // STYLING
    // =====================================================

    setStyleSheet(R"(

        QWidget {
            font-family: "Segoe UI";
            background-color: #F7F8FC;
        }

        QLabel {
            color: #172033;
        }

        QPushButton {
            background-color: #5B5FEF;
            color: white;
            border: none;
            border-radius: 9px;
            font-size: 14px;
            font-weight: 700;
        }

        QPushButton:hover {
            background-color: #4F46E5;
        }

        QPushButton:pressed {
            background-color: #4338CA;
        }

    )");


    logoutButton->setStyleSheet(R"(

        QPushButton {
            background-color: white;
            color: #DC2626;
            border: 1px solid #FCA5A5;
        }

        QPushButton:hover {
            background-color: #FEF2F2;
        }

    )");


    // =====================================================
    // LAYOUT
    // =====================================================

    QVBoxLayout *layout =
        new QVBoxLayout(this);


    layout->setSpacing(
        15
        );


    layout->setContentsMargins(
        70,
        40,
        70,
        40
        );


    layout->addWidget(
        title
        );


    layout->addSpacing(
        15
        );


    layout->addWidget(
        welcomeLabel
        );


    layout->addWidget(
        emailLabel
        );


    layout->addSpacing(
        20
        );


    layout->addWidget(
        buyButton
        );


    layout->addWidget(
        sellButton
        );


    layout->addWidget(
        profileButton
        );


    layout->addSpacing(
        15
        );


    layout->addWidget(
        logoutButton
        );


    // =====================================================
    // CONNECTIONS
    // =====================================================

    connect(
        buyButton,
        &QPushButton::clicked,
        this,
        &Dashboard::openBrowseBooks
        );


    connect(
        sellButton,
        &QPushButton::clicked,
        this,
        &Dashboard::openSellBooks
        );


    connect(
        profileButton,
        &QPushButton::clicked,
        this,
        &Dashboard::openProfile
        );


    connect(
        logoutButton,
        &QPushButton::clicked,
        this,
        &Dashboard::handleLogout
        );
}


// =========================================================
// DESTRUCTOR
// =========================================================

Dashboard::~Dashboard()
{
    if (sellWindow)
    {
        delete sellWindow;
        sellWindow = nullptr;
    }


    if (browseWindow)
    {
        delete browseWindow;
        browseWindow = nullptr;
    }
}


// =========================================================
// SET USER
// =========================================================

void Dashboard::setUser(
    const QString &email
    )
{
    userEmail = email;


    userName =
        Database::instance().getUserName(
            email
            );


    if (userName.isEmpty())
    {
        userName = "User";
    }


    welcomeLabel->setText(
        "Welcome, " +
        userName +
        "!"
        );


    emailLabel->setText(
        email
        );
}


// =========================================================
// OPEN SELL BOOKS
// =========================================================

void Dashboard::openSellBooks()
{
    if (userName.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Error",
            "User information is not available."
            );

        return;
    }


    if (!sellWindow)
    {
        sellWindow =
            new SellWindow(
                userName
                );


        connect(
            sellWindow,
            &SellWindow::backRequested,
            this,
            &Dashboard::closeSellWindow
            );


        connect(
            sellWindow,
            &SellWindow::bookAdded,
            this,
            []()
            {
                // Book has been successfully added.
            }
            );
    }


    hide();


    sellWindow->show();

    sellWindow->raise();

    sellWindow->activateWindow();
}


// =========================================================
// CLOSE SELL WINDOW
// =========================================================

void Dashboard::closeSellWindow()
{
    if (sellWindow)
    {
        sellWindow->hide();
    }


    show();

    raise();

    activateWindow();
}


// =========================================================
// OPEN BROWSE BOOKS
// =========================================================

void Dashboard::openBrowseBooks()
{
    if (userName.isEmpty())
    {
        userName = "User";
    }


    if (!browseWindow)
    {
        browseWindow =
            new BrowseWindow(
                userName
                );


        connect(
            browseWindow,
            &BrowseWindow::backRequested,
            this,
            &Dashboard::closeBrowseWindow
            );
    }


    hide();


    browseWindow->show();

    browseWindow->raise();

    browseWindow->activateWindow();
}


// =========================================================
// CLOSE BROWSE WINDOW
// =========================================================

void Dashboard::closeBrowseWindow()
{
    if (browseWindow)
    {
        browseWindow->hide();
    }


    show();

    raise();

    activateWindow();
}


// =========================================================
// PROFILE
// =========================================================

void Dashboard::openProfile()
{
    QMessageBox::information(
        this,
        "My Profile",
        "Profile module will be added here."
        );
}


// =========================================================
// LOGOUT
// =========================================================

void Dashboard::handleLogout()
{
    if (sellWindow)
    {
        sellWindow->hide();
    }


    if (browseWindow)
    {
        browseWindow->hide();
    }


    hide();


    emit logoutRequested();
}