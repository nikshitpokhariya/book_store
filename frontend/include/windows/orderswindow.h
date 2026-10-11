#ifndef ORDERSWINDOW_H
#define ORDERSWINDOW_H

#include <QWidget>
#include <QString>
#include <QVector>
#include "models/DataModels.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QScrollArea;
class QFrame;

class OrdersWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OrdersWindow(const QString &userName, QWidget *parent = nullptr);
    ~OrdersWindow() override = default;

    void refreshOrders();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void backRequested();
    void browseRequested();
    void orderSelected(const QString &orderId);

public slots:
    void handleTabChange(int tabIndex);

private slots:
    void handleBack();
    void handleBrowse();
    void handleNextPage();
    void handlePrevPage();

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createTabBar();
    QWidget* createStatsBar();
    QWidget* createPaginationBar();
    QFrame* createOrderCard(const OrderModel &order);
    void showOrderDetail(const QString &orderId);
    QPixmap loadOrGenerateCover(const QString &imagePath, const QString &title, const QString &author, int w, int h);

private:
    QString userName;
    int currentTab{0}; // 0 = Purchases (Buyer), 1 = Sales (Seller)
    int currentPage{1};
    PaginationMeta pagination;

    QLabel *totalOrdersLabel;
    QLabel *totalSpentLabel;
    QLabel *statsTitleOrders;
    QLabel *statsTitleSpent;

    QPushButton *purchasesTabBtn;
    QPushButton *salesTabBtn;

    QVBoxLayout *ordersLayout;
    QWidget *emptyStateWidget;

    QPushButton *prevPageBtn;
    QPushButton *nextPageBtn;
    QLabel *pageIndicatorLabel;
};

#endif // ORDERSWINDOW_H
