#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>
#include <QString>

class QLabel;
class QPushButton;
class SellWindow;
class BrowseWindow;

class Dashboard : public QWidget
{
    Q_OBJECT

public:

    explicit Dashboard(
        QWidget *parent = nullptr
        );

    ~Dashboard() override;

    void setUser(
        const QString &email
        );


signals:

    void logoutRequested();


private slots:

    void openSellBooks();

    void openBrowseBooks();

    void openProfile();

    void handleLogout();

    void closeSellWindow();

    void closeBrowseWindow();


private:

    QLabel *welcomeLabel;

    QLabel *emailLabel;

    QPushButton *buyButton;

    QPushButton *sellButton;

    QPushButton *profileButton;

    QPushButton *logoutButton;

    QString userEmail;

    QString userName;

    SellWindow *sellWindow;

    BrowseWindow *browseWindow;
};

#endif // DASHBOARD_H