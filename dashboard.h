#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QWidget>

class QLabel;
class QPushButton;

class Dashboard : public QWidget
{
    Q_OBJECT

public:
    explicit Dashboard(QWidget *parent = nullptr);

    void setUser(const QString& email);

signals:
    void logoutRequested();

private:
    QLabel *welcomeLabel;
    QLabel *emailLabel;

    QPushButton *buyButton;
    QPushButton *sellButton;
    QPushButton *profileButton;
    QPushButton *logoutButton;

    QString userEmail;
};

#endif