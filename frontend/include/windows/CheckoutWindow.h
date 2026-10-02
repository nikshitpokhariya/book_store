#ifndef CHECKOUTWINDOW_H
#define CHECKOUTWINDOW_H

#include <QWidget>
#include <QString>
#include "models/DataModels.h"

class QLineEdit;
class QRadioButton;
class QPushButton;
class QLabel;

class CheckoutWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CheckoutWindow(QWidget *parent = nullptr);
    ~CheckoutWindow() override = default;

    void refreshCartSummary();

signals:
    void orderPlaced(const QString &orderId);
    void backRequested();

private slots:
    void handlePlaceOrder();

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createAddressCard();
    QWidget* createPaymentCard();
    QWidget* createSummaryCard();

    bool validateForm();

private:
    CartModel cart;

    // Address fields
    QLineEdit *fullNameEdit;
    QLineEdit *addressLineEdit;
    QLineEdit *cityEdit;
    QLineEdit *stateEdit;
    QLineEdit *postalCodeEdit;
    QLineEdit *phoneEdit;

    // Payment radios
    QRadioButton *upiRadio;
    QRadioButton *cardRadio;
    QRadioButton *codRadio;

    // Summary labels & button
    QLabel *itemCountLabel;
    QLabel *subtotalLabel;
    QLabel *totalLabel;
    QPushButton *placeOrderButton;
};

#endif // CHECKOUTWINDOW_H
