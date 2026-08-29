#include "dashboard.h"
#include "database.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFont>
#include <QMessageBox>

Dashboard::Dashboard(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("BookMarket - Dashboard");

    resize(600, 500);

    QLabel *title =
        new QLabel("BOOKMARKET");

    QFont titleFont;
    titleFont.setPointSize(28);
    titleFont.setBold(true);

    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);

    welcomeLabel =
        new QLabel("Welcome!");

    QFont welcomeFont;
    welcomeFont.setPointSize(20);
    welcomeFont.setBold(true);

    welcomeLabel->setFont(welcomeFont);
    welcomeLabel->setAlignment(Qt::AlignCenter);

    emailLabel =
        new QLabel();

    emailLabel->setAlignment(Qt::AlignCenter);

    buyButton =
        new QPushButton("Buy Books");

    sellButton =
        new QPushButton("Sell Books");

    profileButton =
        new QPushButton("My Profile");

    logoutButton =
        new QPushButton("Logout");

    buyButton->setMinimumHeight(50);
    sellButton->setMinimumHeight(50);
    profileButton->setMinimumHeight(50);
    logoutButton->setMinimumHeight(50);

    QVBoxLayout *layout =
        new QVBoxLayout(this);

    layout->setSpacing(15);

    layout->setContentsMargins(
        70, 40, 70, 40
        );

    layout->addWidget(title);

    layout->addSpacing(15);

    layout->addWidget(welcomeLabel);
    layout->addWidget(emailLabel);

    layout->addSpacing(20);

    layout->addWidget(buyButton);
    layout->addWidget(sellButton);
    layout->addWidget(profileButton);

    layout->addSpacing(15);

    layout->addWidget(logoutButton);

    connect(
        buyButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            QMessageBox::information(
                this,
                "Buy Books",
                "Buy Books module will be added here."
                );
        }
        );

    connect(
        sellButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            QMessageBox::information(
                this,
                "Sell Books",
                "Sell Books module will be added here."
                );
        }
        );

    connect(
        profileButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            QMessageBox::information(
                this,
                "My Profile",
                "Profile module will be added here."
                );
        }
        );

    connect(
        logoutButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            emit logoutRequested();
            hide();
        }
        );
}

void Dashboard::setUser(const QString& email)
{
    userEmail = email;

    QString name =
        Database::instance().getUserName(email);

    if (name.isEmpty())
    {
        name = "User";
    }

    welcomeLabel->setText(
        "Welcome, " + name + "!"
        );

    emailLabel->setText(
        email
        );
}