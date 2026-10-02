#ifndef ORDERSWINDOW_H
#define ORDERSWINDOW_H

#include <QWidget>
#include <QString>
#include <QVBoxLayout>
#include <QVariantMap>

class QLabel;
class QPushButton;

class OrdersWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OrdersWindow(const QString &userName, QWidget *parent = nullptr);
    ~OrdersWindow() override;

    void refreshOrders();

signals:
    void backRequested();
    void browseRequested();

private slots:
    void handleBack();
    void handleBrowse();

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createStatsBar();
    QWidget* createOrderCard(const QVariantMap &order);

private:
    QString userName;

    QLabel *totalOrdersLabel;
    QLabel *totalSpentLabel;

    QVBoxLayout *ordersLayout;
};

#endif // ORDERSWINDOW_H
