#include "windows/CheckoutWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QMessageBox>
#include <QNetworkReply>

CheckoutWindow::CheckoutWindow(QWidget *parent)
    : QWidget(parent),
      fullNameEdit(nullptr),
      addressLineEdit(nullptr),
      cityEdit(nullptr),
      stateEdit(nullptr),
      postalCodeEdit(nullptr),
      phoneEdit(nullptr),
      upiRadio(nullptr),
      cardRadio(nullptr),
      codRadio(nullptr),
      itemCountLabel(nullptr),
      subtotalLabel(nullptr),
      totalLabel(nullptr),
      placeOrderButton(nullptr)
{
    setupUI();
    refreshCartSummary();
}

void CheckoutWindow::setupUI()
{
    setWindowTitle(QStringLiteral("BookBazzar - Checkout"));
    resize(980, 750);
    setMinimumSize(800, 600);
    setStyleSheet(QStringLiteral("background-color: #F8F9FD; font-family: 'Segoe UI', Arial, sans-serif;"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(createTopBar());

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto *scrollContent = new QWidget();
    scrollContent->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *contentLayout = new QHBoxLayout(scrollContent);
    contentLayout->setContentsMargins(36, 28, 36, 36);
    contentLayout->setSpacing(28);

    // Left Column: Address & Payment Method Forms
    auto *leftCol = new QWidget();
    auto *leftLayout = new QVBoxLayout(leftCol);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(22);

    leftLayout->addWidget(createAddressCard());
    leftLayout->addWidget(createPaymentCard());
    leftLayout->addStretch();

    contentLayout->addWidget(leftCol, 2);

    // Right Column: Summary & Place Order
    contentLayout->addWidget(createSummaryCard(), 1);

    scroll->setWidget(scrollContent);
    mainLayout->addWidget(scroll, 1);
}

QWidget* CheckoutWindow::createTopBar()
{
    auto *topBar = new QFrame(this);
    topBar->setFixedHeight(70);
    topBar->setStyleSheet(QStringLiteral("background-color: white; border-bottom: 1px solid #E2E8F0;"));

    auto *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(32, 12, 32, 12);
    layout->setSpacing(16);

    auto *backBtn = new QPushButton(QStringLiteral("← Back to Cart"), topBar);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #F1F5F9; color: #334155; border: 1px solid #E2E8F0; padding: 6px 14px; border-radius: 8px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #E2E8F0; color: #0F172A; }"
    ));
    connect(backBtn, &QPushButton::clicked, this, &CheckoutWindow::backRequested);
    layout->addWidget(backBtn);

    auto *title = new QLabel(QStringLiteral("Secure Checkout"), topBar);
    title->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 800; color: #0F172A;"));
    layout->addWidget(title);

    layout->addStretch();
    return topBar;
}

QWidget* CheckoutWindow::createAddressCard()
{
    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("addressCard"));
    card->setStyleSheet(QStringLiteral(
        "QFrame#addressCard { background-color: white; border: 1px solid #E2E8F0; border-radius: 16px; padding: 22px; }"
        "QLineEdit { background: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 8px; padding: 10px 14px; font-size: 14px; color: #1E293B; }"
        "QLineEdit:focus { border-color: #4F46E5; background: white; }"
    ));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(14);

    auto *heading = new QLabel(QStringLiteral("1. Shipping Address"), card);
    heading->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: 800; color: #0F172A;"));
    layout->addWidget(heading);

    auto *grid = new QGridLayout();
    grid->setSpacing(12);

    // Full Name
    auto *nameLabel = new QLabel(QStringLiteral("Full Name *"), card);
    nameLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #475569; font-size: 13px;"));
    fullNameEdit = new QLineEdit(card);
    fullNameEdit->setPlaceholderText(QStringLiteral("e.g. John Doe"));
    grid->addWidget(nameLabel, 0, 0);
    grid->addWidget(fullNameEdit, 1, 0, 1, 2);

    // Address Line
    auto *addrLabel = new QLabel(QStringLiteral("Street Address *"), card);
    addrLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #475569; font-size: 13px;"));
    addressLineEdit = new QLineEdit(card);
    addressLineEdit->setPlaceholderText(QStringLiteral("House number, street name, area"));
    grid->addWidget(addrLabel, 2, 0);
    grid->addWidget(addressLineEdit, 3, 0, 1, 2);

    // City & State
    auto *cityLabel = new QLabel(QStringLiteral("City *"), card);
    cityLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #475569; font-size: 13px;"));
    cityEdit = new QLineEdit(card);
    cityEdit->setPlaceholderText(QStringLiteral("e.g. Mumbai"));
    grid->addWidget(cityLabel, 4, 0);
    grid->addWidget(cityEdit, 5, 0);

    auto *stateLabel = new QLabel(QStringLiteral("State *"), card);
    stateLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #475569; font-size: 13px;"));
    stateEdit = new QLineEdit(card);
    stateEdit->setPlaceholderText(QStringLiteral("e.g. Maharashtra"));
    grid->addWidget(stateLabel, 4, 1);
    grid->addWidget(stateEdit, 5, 1);

    // Postal Code & Phone
    auto *postalLabel = new QLabel(QStringLiteral("Postal Code / PIN *"), card);
    postalLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #475569; font-size: 13px;"));
    postalCodeEdit = new QLineEdit(card);
    postalCodeEdit->setPlaceholderText(QStringLiteral("e.g. 400001"));
    grid->addWidget(postalLabel, 6, 0);
    grid->addWidget(postalCodeEdit, 7, 0);

    auto *phoneLabel = new QLabel(QStringLiteral("Phone Number *"), card);
    phoneLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #475569; font-size: 13px;"));
    phoneEdit = new QLineEdit(card);
    phoneEdit->setPlaceholderText(QStringLiteral("e.g. 9876543210"));
    grid->addWidget(phoneLabel, 6, 1);
    grid->addWidget(phoneEdit, 7, 1);

    layout->addLayout(grid);
    return card;
}

QWidget* CheckoutWindow::createPaymentCard()
{
    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("paymentCard"));
    card->setStyleSheet(QStringLiteral(
        "QFrame#paymentCard { background-color: white; border: 1px solid #E2E8F0; border-radius: 16px; padding: 22px; }"
    ));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(14);

    auto *heading = new QLabel(QStringLiteral("2. Payment Method"), card);
    heading->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: 800; color: #0F172A;"));
    layout->addWidget(heading);

    auto *payGroup = new QButtonGroup(this);

    auto createRadioRow = [card, payGroup](QRadioButton* &btn, const QString &id, const QString &title, const QString &desc, bool checked) {
        auto *frame = new QFrame(card);
        frame->setStyleSheet(QStringLiteral("QFrame { background: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 10px; padding: 12px; }"));
        auto *row = new QHBoxLayout(frame);
        row->setContentsMargins(12, 10, 12, 10);
        row->setSpacing(12);

        btn = new QRadioButton(frame);
        btn->setChecked(checked);
        payGroup->addButton(btn);
        row->addWidget(btn);

        auto *info = new QVBoxLayout();
        info->setSpacing(2);
        auto *tLabel = new QLabel(title, frame);
        tLabel->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: 700; color: #1E293B;"));
        auto *dLabel = new QLabel(desc, frame);
        dLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #64748B;"));
        info->addWidget(tLabel);
        info->addWidget(dLabel);
        row->addLayout(info, 1);

        return frame;
    };

    layout->addWidget(createRadioRow(upiRadio, QStringLiteral("UPI"), QStringLiteral("UPI / QR Code (Instant)"), QStringLiteral("Pay via Google Pay, PhonePe, Paytm, or UPI ID"), true));
    layout->addWidget(createRadioRow(cardRadio, QStringLiteral("CARD"), QStringLiteral("Credit / Debit Card"), QStringLiteral("Visa, MasterCard, RuPay cards supported"), false));
    layout->addWidget(createRadioRow(codRadio, QStringLiteral("COD"), QStringLiteral("Cash on Delivery"), QStringLiteral("Pay with cash when your books are delivered"), false));

    return card;
}

QWidget* CheckoutWindow::createSummaryCard()
{
    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("orderSummaryCard"));
    card->setStyleSheet(QStringLiteral(
        "QFrame#orderSummaryCard { background-color: white; border: 1px solid #E2E8F0; border-radius: 16px; padding: 22px; }"
    ));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(16);

    auto *heading = new QLabel(QStringLiteral("Order Summary"), card);
    heading->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 800; color: #0F172A;"));
    layout->addWidget(heading);

    auto *subtotalRow = new QHBoxLayout();
    itemCountLabel = new QLabel(QStringLiteral("Items (0)"), card);
    itemCountLabel->setStyleSheet(QStringLiteral("font-size: 14px; color: #64748B;"));
    subtotalLabel = new QLabel(QStringLiteral("₹0.00"), card);
    subtotalLabel->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 700; color: #1E293B;"));
    subtotalRow->addWidget(itemCountLabel);
    subtotalRow->addStretch();
    subtotalRow->addWidget(subtotalLabel);
    layout->addLayout(subtotalRow);

    auto *shipRow = new QHBoxLayout();
    auto *shipTitle = new QLabel(QStringLiteral("Delivery Shipping"), card);
    shipTitle->setStyleSheet(QStringLiteral("font-size: 14px; color: #64748B;"));
    auto *shipFree = new QLabel(QStringLiteral("FREE"), card);
    shipFree->setStyleSheet(QStringLiteral("font-size: 12px; font-weight: 800; color: #059669; background: #D1FAE5; padding: 2px 8px; border-radius: 4px;"));
    shipRow->addWidget(shipTitle);
    shipRow->addStretch();
    shipRow->addWidget(shipFree);
    layout->addLayout(shipRow);

    auto *line = new QFrame(card);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("color: #E2E8F0;"));
    layout->addWidget(line);

    auto *totalRow = new QHBoxLayout();
    auto *totalTitle = new QLabel(QStringLiteral("Amount to Pay"), card);
    totalTitle->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 800; color: #0F172A;"));
    totalLabel = new QLabel(QStringLiteral("₹0.00"), card);
    totalLabel->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 900; color: #4F46E5;"));
    totalRow->addWidget(totalTitle);
    totalRow->addStretch();
    totalRow->addWidget(totalLabel);
    layout->addLayout(totalRow);

    layout->addSpacing(14);

    placeOrderButton = new QPushButton(QStringLiteral("Place Order"), card);
    placeOrderButton->setCursor(Qt::PointingHandCursor);
    placeOrderButton->setFixedHeight(50);
    placeOrderButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #4F46E5; color: white; border-radius: 10px; font-size: 16px; font-weight: 800; border: none; }"
        "QPushButton:hover { background-color: #4338CA; }"
        "QPushButton:disabled { background-color: #CBD5E1; color: #94A3B8; }"
    ));
    connect(placeOrderButton, &QPushButton::clicked, this, &CheckoutWindow::handlePlaceOrder);
    layout->addWidget(placeOrderButton);

    auto *securityNote = new QLabel(QStringLiteral("🔒 100% Safe & Secure Checkout"), card);
    securityNote->setAlignment(Qt::AlignCenter);
    securityNote->setStyleSheet(QStringLiteral("font-size: 12px; color: #64748B; font-weight: 600; margin-top: 6px;"));
    layout->addWidget(securityNote);

    layout->addStretch();
    return card;
}

void CheckoutWindow::refreshCartSummary()
{
    auto *reply = ApiClient::instance().get(QStringLiteral("/api/cart"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok && json.contains(QStringLiteral("cart"))) {
            cart = CartModel::fromJson(json[QStringLiteral("cart")].toObject());
            itemCountLabel->setText(QString("Items (%1)").arg(cart.items.size()));
            subtotalLabel->setText(QString("₹%1").arg(cart.subtotal, 0, 'f', 2));
            totalLabel->setText(QString("₹%1").arg(cart.total, 0, 'f', 2));
            placeOrderButton->setText(QString("Place Order (₹%1)").arg(cart.total, 0, 'f', 0));
            placeOrderButton->setEnabled(!cart.items.isEmpty() && !cart.hasUnavailableItems);
        }
    });
}

bool CheckoutWindow::validateForm()
{
    const QString name = fullNameEdit->text().trimmed();
    if (name.isEmpty() || name.length() > 100) {
        QMessageBox::warning(this, QStringLiteral("Validation Error"), QStringLiteral("Full Name is required and must not exceed 100 characters."));
        fullNameEdit->setFocus();
        return false;
    }

    const QString addr = addressLineEdit->text().trimmed();
    if (addr.isEmpty() || addr.length() > 200) {
        QMessageBox::warning(this, QStringLiteral("Validation Error"), QStringLiteral("Street Address is required and must not exceed 200 characters."));
        addressLineEdit->setFocus();
        return false;
    }

    const QString city = cityEdit->text().trimmed();
    if (city.isEmpty() || city.length() > 50) {
        QMessageBox::warning(this, QStringLiteral("Validation Error"), QStringLiteral("City is required and must not exceed 50 characters."));
        cityEdit->setFocus();
        return false;
    }

    const QString state = stateEdit->text().trimmed();
    if (state.isEmpty() || state.length() > 50) {
        QMessageBox::warning(this, QStringLiteral("Validation Error"), QStringLiteral("State is required and must not exceed 50 characters."));
        stateEdit->setFocus();
        return false;
    }

    const QString pin = postalCodeEdit->text().trimmed();
    if (pin.isEmpty() || pin.length() > 20) {
        QMessageBox::warning(this, QStringLiteral("Validation Error"), QStringLiteral("Postal Code / PIN is required and must not exceed 20 characters."));
        postalCodeEdit->setFocus();
        return false;
    }

    const QString phone = phoneEdit->text().trimmed();
    if (phone.isEmpty() || phone.length() > 20) {
        QMessageBox::warning(this, QStringLiteral("Validation Error"), QStringLiteral("Phone number is required and must not exceed 20 characters."));
        phoneEdit->setFocus();
        return false;
    }

    return true;
}

void CheckoutWindow::handlePlaceOrder()
{
    if (!validateForm()) {
        return;
    }

    placeOrderButton->setEnabled(false);
    placeOrderButton->setText(QStringLiteral("Processing Order..."));

    QString paymentMethod = QStringLiteral("UPI");
    if (cardRadio && cardRadio->isChecked()) {
        paymentMethod = QStringLiteral("CARD");
    } else if (codRadio && codRadio->isChecked()) {
        paymentMethod = QStringLiteral("COD");
    }

    ShippingAddressModel addr;
    addr.fullName = fullNameEdit->text().trimmed();
    addr.addressLine = addressLineEdit->text().trimmed();
    addr.city = cityEdit->text().trimmed();
    addr.state = stateEdit->text().trimmed();
    addr.postalCode = postalCodeEdit->text().trimmed();
    addr.phone = phoneEdit->text().trimmed();

    QJsonObject payload;
    payload[QStringLiteral("paymentMethod")] = paymentMethod;
    payload[QStringLiteral("shippingAddress")] = addr.toJson();

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/checkout"), payload, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        placeOrderButton->setEnabled(true);
        placeOrderButton->setText(QStringLiteral("Place Order"));

        if (ok && json.contains(QStringLiteral("order"))) {
            OrderModel order = OrderModel::fromJson(json[QStringLiteral("order")].toObject());
            QMessageBox::information(this, QStringLiteral("Order Placed!"),
                                     QString("Congratulations! Your order #%1 has been placed successfully.\nTotal Amount: ₹%2")
                                     .arg(order.orderNumber)
                                     .arg(order.total, 0, 'f', 2));
            emit orderPlaced(order.id);
        } else {
            QMessageBox::critical(this, QStringLiteral("Checkout Failed"),
                                  errorMsg.isEmpty() ? QStringLiteral("Unable to complete checkout.") : errorMsg);
        }
    });
}
