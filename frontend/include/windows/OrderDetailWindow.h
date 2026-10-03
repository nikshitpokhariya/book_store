#ifndef ORDERDETAILWINDOW_H
#define ORDERDETAILWINDOW_H

#include <QWidget>
#include <QString>
#include "models/DataModels.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QScrollArea;
class QFrame;

class OrderDetailWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OrderDetailWindow(const QString &orderId, QWidget *parent = nullptr);
    ~OrderDetailWindow() override = default;

    void loadOrder();

signals:
    void backRequested();
    void orderUpdated();
    void orderCancelled();
    void orderStatusUpdated();

public slots:
    void handleWriteReview(const QString &bookId, const QString &bookTitle);

private slots:
    void handleCancelOrder();
    void handleRequestReturn();
    void handleUpdateStatus(const QString &newStatus);
    void handleProcessReturn(bool approve);

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createStatusHeader();
    QWidget* createTimelineCard();
    QWidget* createItemsCard();
    QWidget* createAddressAndPaymentCard();
    QWidget* createActionsCard();
    QFrame* createOrderItemRow(const OrderItemSnapshot &item);
    QPixmap loadOrGenerateCover(const QString &imagePath, const QString &title, const QString &author, int w, int h);

private:
    QString m_orderId;
    OrderModel m_order;

    QLabel *orderNumberLabel;
    QLabel *dateLabel;
    QLabel *statusBadge;
    QLabel *trackingLabel;

    QVBoxLayout *timelineLayout;
    QVBoxLayout *itemsListLayout;
    QLabel *addressDetailsLabel;
    QLabel *paymentDetailsLabel;
    QLabel *totalAmountLabel;

    QHBoxLayout *actionsLayout;
    QWidget *actionsCard;
};

#endif // ORDERDETAILWINDOW_H
