#include "windows/CheckoutWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include "AppStyle.h"

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
#include "StyledMessageBox.h"
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
    setWindowTitle("BookBazzar - Checkout");
    resize(1180, 800);
    setMinimumSize(940, 620);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(createTopBar());

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QString(R"(
        QScrollArea {
            background-color: %1;
            border: none;
        }
        %2
    )").arg(AppStyle::Background, AppStyle::scrollBarStyle()));

    auto *scrollContent = new QWidget();
    scrollContent->setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));
    auto *contentLayout = new QHBoxLayout(scrollContent);
    contentLayout->setContentsMargins(36, 28, 36, 36);
    contentLayout->setSpacing(32);

    // Left Column: Address & Payment Method Forms
    auto *leftCol = new QWidget();
    auto *leftLayout = new QVBoxLayout(leftCol);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(24);

    leftLayout->addWidget(createAddressCard());
    leftLayout->addWidget(createPaymentCard());
    leftLayout->addStretch();

    contentLayout->addWidget(leftCol, 2);
    contentLayout->addWidget(createSummaryCard(), 1);

    scroll->setWidget(scrollContent);
    mainLayout->addWidget(scroll, 1);
}

QWidget* CheckoutWindow::createTopBar()
{
    auto *topBar = new QFrame(this);
    topBar->setFixedHeight(72);
    topBar->setStyleSheet(R"(
        .QFrame {
            background-color: #FFFFFF;
            border-bottom: 1px solid #E2E8F0;
        }
        QLabel {
            border: none;
            background: transparent;
        }
    )");
    AppStyle::applyElevation(topBar, 16, 2, 15);

    auto *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(36, 10, 36, 10);
    layout->setSpacing(16);

    auto *backBtn = new QPushButton("← Back to Cart", topBar);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setMinimumHeight(38);
    backBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(backBtn, &QPushButton::clicked, this, &CheckoutWindow::backRequested);
    layout->addWidget(backBtn);

    auto *title = new QLabel("Secure Order Checkout", topBar);
    title->setStyleSheet(QString("font-size: 20px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    layout->addWidget(title);

    layout->addStretch();
    return topBar;
}

QWidget* CheckoutWindow::createAddressCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 20, 6, 18);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(16);

    auto *heading = new QLabel("1. Delivery & Shipping Address", card);
    heading->setStyleSheet(QString("font-size: 17px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    layout->addWidget(heading);

    auto *grid = new QGridLayout();
    grid->setSpacing(12);

    auto addLabel = [](const QString &text, QWidget *parent) {
        auto *lbl = new QLabel(text, parent);
        lbl->setStyleSheet(QString("font-weight: 600; color: %1; font-size: 13px;").arg(AppStyle::TextSecondary));
        return lbl;
    };

    fullNameEdit = new QLineEdit(card);
    fullNameEdit->setPlaceholderText("e.g. John Doe");
    fullNameEdit->setStyleSheet(AppStyle::inputStyle());
    fullNameEdit->setMinimumHeight(42);
    grid->addWidget(addLabel("Full Recipient Name *", card), 0, 0);
    grid->addWidget(fullNameEdit, 1, 0, 1, 2);

    addressLineEdit = new QLineEdit(card);
    addressLineEdit->setPlaceholderText("Flat / House no., Building, Street name, Area");
    addressLineEdit->setStyleSheet(AppStyle::inputStyle());
    addressLineEdit->setMinimumHeight(42);
    grid->addWidget(addLabel("Street Address *", card), 2, 0);
    grid->addWidget(addressLineEdit, 3, 0, 1, 2);

    cityEdit = new QLineEdit(card);
    cityEdit->setPlaceholderText("e.g. Mumbai");
    cityEdit->setStyleSheet(AppStyle::inputStyle());
    cityEdit->setMinimumHeight(42);
    grid->addWidget(addLabel("City *", card), 4, 0);
    grid->addWidget(cityEdit, 5, 0);

    stateEdit = new QLineEdit(card);
    stateEdit->setPlaceholderText("e.g. Maharashtra");
    stateEdit->setStyleSheet(AppStyle::inputStyle());
    stateEdit->setMinimumHeight(42);
    grid->addWidget(addLabel("State *", card), 4, 1);
    grid->addWidget(stateEdit, 5, 1);

    postalCodeEdit = new QLineEdit(card);
    postalCodeEdit->setPlaceholderText("e.g. 400001");
    postalCodeEdit->setStyleSheet(AppStyle::inputStyle());
    postalCodeEdit->setMinimumHeight(42);
    grid->addWidget(addLabel("Postal Code / PIN *", card), 6, 0);
    grid->addWidget(postalCodeEdit, 7, 0);

    phoneEdit = new QLineEdit(card);
    phoneEdit->setPlaceholderText("e.g. 9876543210");
    phoneEdit->setStyleSheet(AppStyle::inputStyle());
    phoneEdit->setMinimumHeight(42);
    grid->addWidget(addLabel("Mobile Phone Number *", card), 6, 1);
    grid->addWidget(phoneEdit, 7, 1);

    layout->addLayout(grid);
    return card;
}

QWidget* CheckoutWindow::createPaymentCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 20, 6, 18);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(14);

    auto *heading = new QLabel("2. Payment Method", card);
    heading->setStyleSheet(QString("font-size: 17px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    layout->addWidget(heading);

    auto *payGroup = new QButtonGroup(this);

    auto createRadioRow = [card, payGroup](QRadioButton* &btn, const QString &title, const QString &desc, bool checked) {
        auto *frame = new QFrame(card);
        frame->setStyleSheet(QString(R"(
            .QFrame {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 10px;
            }
            QLabel {
                border: none;
                background: transparent;
            }
        )").arg(AppStyle::SurfaceSubtle, AppStyle::BorderSubtle));

        auto *row = new QHBoxLayout(frame);
        row->setContentsMargins(14, 12, 14, 12);
        row->setSpacing(14);

        btn = new QRadioButton(frame);
        btn->setChecked(checked);
        payGroup->addButton(btn);
        row->addWidget(btn);

        auto *info = new QVBoxLayout();
        info->setSpacing(2);
        auto *tLabel = new QLabel(title, frame);
        tLabel->setStyleSheet(QString("font-size: 14px; font-weight: 700; color: %1;").arg(AppStyle::TextPrimary));
        auto *dLabel = new QLabel(desc, frame);
        dLabel->setStyleSheet(QString("font-size: 12px; color: %1;").arg(AppStyle::TextSecondary));
        info->addWidget(tLabel);
        info->addWidget(dLabel);
        row->addLayout(info, 1);

        return frame;
    };

    layout->addWidget(createRadioRow(upiRadio, "UPI / QR Code (Instant)", "Pay via Google Pay, PhonePe, Paytm, or UPI ID", true));
    layout->addWidget(createRadioRow(cardRadio, "Credit / Debit Card", "Visa, MasterCard, and RuPay cards supported", false));
    layout->addWidget(createRadioRow(codRadio, "Cash on Delivery", "Pay in cash when your books are delivered at your doorstep", false));

    return card;
}

QWidget* CheckoutWindow::createSummaryCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 20, 6, 18);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto *heading = new QLabel("Order Summary", card);
    heading->setStyleSheet(QString("font-size: 18px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    layout->addWidget(heading);

    auto *subtotalRow = new QHBoxLayout();
    itemCountLabel = new QLabel("Items (0)", card);
    itemCountLabel->setStyleSheet(QString("font-size: 13px; color: %1;").arg(AppStyle::TextSecondary));
    subtotalLabel = new QLabel("₹0.00", card);
    subtotalLabel->setStyleSheet(QString("font-size: 15px; font-weight: 700; color: %1;").arg(AppStyle::TextPrimary));
    subtotalRow->addWidget(itemCountLabel);
    subtotalRow->addStretch();
    subtotalRow->addWidget(subtotalLabel);
    layout->addLayout(subtotalRow);

    auto *shipRow = new QHBoxLayout();
    auto *shipTitle = new QLabel("Standard Delivery", card);
    shipTitle->setStyleSheet(QString("font-size: 13px; color: %1;").arg(AppStyle::TextSecondary));
    auto *shipFree = new QLabel("FREE", card);
    shipFree->setStyleSheet(AppStyle::badgeStyle(AppStyle::SuccessLight, AppStyle::SuccessText, AppStyle::SuccessBorder));
    shipRow->addWidget(shipTitle);
    shipRow->addStretch();
    shipRow->addWidget(shipFree);
    layout->addLayout(shipRow);

    auto *line = new QFrame(card);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QString("color: %1;").arg(AppStyle::BorderSubtle));
    layout->addWidget(line);

    auto *totalRow = new QHBoxLayout();
    auto *totalTitle = new QLabel("Total to Pay", card);
    totalTitle->setStyleSheet(QString("font-size: 16px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    totalLabel = new QLabel("₹0.00", card);
    totalLabel->setStyleSheet(QString("font-size: 24px; font-weight: 900; color: %1;").arg(AppStyle::Primary));
    totalRow->addWidget(totalTitle);
    totalRow->addStretch();
    totalRow->addWidget(totalLabel);
    layout->addLayout(totalRow);

    layout->addSpacing(12);

    placeOrderButton = new QPushButton("Place Order", card);
    placeOrderButton->setCursor(Qt::PointingHandCursor);
    placeOrderButton->setMinimumHeight(48);
    placeOrderButton->setStyleSheet(AppStyle::primaryButtonStyle());
    connect(placeOrderButton, &QPushButton::clicked, this, &CheckoutWindow::handlePlaceOrder);
    layout->addWidget(placeOrderButton);

    auto *securityNote = new QLabel("🔒  100% Encrypted & Secure Checkout", card);
    securityNote->setAlignment(Qt::AlignCenter);
    securityNote->setStyleSheet(QString("font-size: 12px; color: %1; font-weight: 600; margin-top: 4px;").arg(AppStyle::TextMuted));
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
        StyledMessageBox::warning(this, "Validation Error", "Full Name is required and must not exceed 100 characters.");
        fullNameEdit->setFocus();
        return false;
    }

    const QString addr = addressLineEdit->text().trimmed();
    if (addr.isEmpty() || addr.length() > 200) {
        StyledMessageBox::warning(this, "Validation Error", "Street Address is required and must not exceed 200 characters.");
        addressLineEdit->setFocus();
        return false;
    }

    const QString city = cityEdit->text().trimmed();
    if (city.isEmpty() || city.length() > 50) {
        StyledMessageBox::warning(this, "Validation Error", "City is required and must not exceed 50 characters.");
        cityEdit->setFocus();
        return false;
    }

    const QString state = stateEdit->text().trimmed();
    if (state.isEmpty() || state.length() > 50) {
        StyledMessageBox::warning(this, "Validation Error", "State is required and must not exceed 50 characters.");
        stateEdit->setFocus();
        return false;
    }

    const QString pin = postalCodeEdit->text().trimmed();
    if (pin.isEmpty() || pin.length() > 20) {
        StyledMessageBox::warning(this, "Validation Error", "Postal Code / PIN is required and must not exceed 20 characters.");
        postalCodeEdit->setFocus();
        return false;
    }

    const QString phone = phoneEdit->text().trimmed();
    if (phone.isEmpty() || phone.length() > 20) {
        StyledMessageBox::warning(this, "Validation Error", "Phone number is required and must not exceed 20 characters.");
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
    placeOrderButton->setText("Processing Order...");

    QString paymentMethod = "UPI";
    if (cardRadio && cardRadio->isChecked()) {
        paymentMethod = "CARD";
    } else if (codRadio && codRadio->isChecked()) {
        paymentMethod = "COD";
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
        placeOrderButton->setText("Place Order");

        if (ok && json.contains(QStringLiteral("order"))) {
            OrderModel order = OrderModel::fromJson(json[QStringLiteral("order")].toObject());
            StyledMessageBox::success(
                this, "Order Placed Successfully",
                QString("Congratulations! Your order #%1 has been placed successfully.\nTotal Amount: ₹%2")
                    .arg(order.orderNumber)
                    .arg(order.total, 0, 'f', 2)
            );
            emit orderPlaced(order.id);
        } else {
            StyledMessageBox::critical(
                this, "Checkout Failed",
                errorMsg.isEmpty() ? "Unable to complete checkout." : errorMsg
            );
        }
    });
}
