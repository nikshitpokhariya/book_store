#include "windows/OrderDetailWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialog>
#include <QSpinBox>
#include <QTextEdit>
#include <QPainter>
#include <QFileInfo>
#include <QNetworkReply>

OrderDetailWindow::OrderDetailWindow(const QString &orderId, QWidget *parent)
    : QWidget(parent),
      m_orderId(orderId),
      orderNumberLabel(nullptr),
      dateLabel(nullptr),
      statusBadge(nullptr),
      trackingLabel(nullptr),
      timelineLayout(nullptr),
      itemsListLayout(nullptr),
      addressDetailsLabel(nullptr),
      paymentDetailsLabel(nullptr),
      totalAmountLabel(nullptr),
      actionsLayout(nullptr),
      actionsCard(nullptr)
{
    setupUI();
    loadOrder();
}

void OrderDetailWindow::setupUI()
{
    setWindowTitle(QStringLiteral("BookBazzar - Order Details"));
    resize(1000, 800);
    setMinimumSize(850, 600);
    setStyleSheet(QStringLiteral("background-color: #F8F9FD; font-family: 'Segoe UI', Arial, sans-serif;"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(createTopBar());

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto *content = new QWidget();
    content->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(36, 24, 36, 36);
    contentLayout->setSpacing(22);

    contentLayout->addWidget(createStatusHeader());
    contentLayout->addWidget(createTimelineCard());
    contentLayout->addWidget(createActionsCard());
    contentLayout->addWidget(createItemsCard());
    contentLayout->addWidget(createAddressAndPaymentCard());

    scroll->setWidget(content);
    mainLayout->addWidget(scroll, 1);
}

QWidget* OrderDetailWindow::createTopBar()
{
    auto *topBar = new QFrame(this);
    topBar->setFixedHeight(70);
    topBar->setStyleSheet(QStringLiteral("background-color: white; border-bottom: 1px solid #E2E8F0;"));

    auto *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(32, 12, 32, 12);
    layout->setSpacing(16);

    auto *backBtn = new QPushButton(QStringLiteral("← Back"), topBar);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #F1F5F9; color: #334155; border: 1px solid #E2E8F0; padding: 6px 14px; border-radius: 8px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #E2E8F0; color: #0F172A; }"
    ));
    connect(backBtn, &QPushButton::clicked, this, &OrderDetailWindow::backRequested);
    layout->addWidget(backBtn);

    auto *title = new QLabel(QStringLiteral("Order Details"), topBar);
    title->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 800; color: #0F172A;"));
    layout->addWidget(title);

    layout->addStretch();

    auto *refreshBtn = new QPushButton(QStringLiteral("↻ Refresh"), topBar);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: #4F46E5; border: 1px solid #C7D2FE; padding: 6px 14px; border-radius: 8px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #EEF2FF; }"
    ));
    connect(refreshBtn, &QPushButton::clicked, this, &OrderDetailWindow::loadOrder);
    layout->addWidget(refreshBtn);

    return topBar;
}

QWidget* OrderDetailWindow::createStatusHeader()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background-color: white; border: 1px solid #E2E8F0; border-radius: 14px; padding: 18px; }"
    ));

    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(20);

    auto *info = new QVBoxLayout();
    info->setSpacing(4);

    orderNumberLabel = new QLabel(QStringLiteral("Order #..."), card);
    orderNumberLabel->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 800; color: #0F172A;"));
    info->addWidget(orderNumberLabel);

    dateLabel = new QLabel(QStringLiteral("Placed on: ..."), card);
    dateLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #64748B;"));
    info->addWidget(dateLabel);

    trackingLabel = new QLabel(QStringLiteral("Tracking ID: ..."), card);
    trackingLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #475569; font-weight: 600;"));
    info->addWidget(trackingLabel);

    layout->addLayout(info, 1);

    statusBadge = new QLabel(QStringLiteral("CONFIRMED"), card);
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setFixedHeight(36);
    statusBadge->setStyleSheet(QStringLiteral(
        "QLabel { background: #DBEAFE; color: #1E40AF; font-size: 13px; font-weight: 800; border-radius: 8px; padding: 0 16px; }"
    ));
    layout->addWidget(statusBadge);

    return card;
}

QWidget* OrderDetailWindow::createTimelineCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background-color: white; border: 1px solid #E2E8F0; border-radius: 14px; padding: 20px; }"
    ));

    auto *mainV = new QVBoxLayout(card);
    mainV->setContentsMargins(20, 16, 20, 16);
    mainV->setSpacing(14);

    auto *title = new QLabel(QStringLiteral("Delivery Timeline & Status"), card);
    title->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 800; color: #0F172A;"));
    mainV->addWidget(title);

    timelineLayout = new QVBoxLayout();
    timelineLayout->setSpacing(8);
    mainV->addLayout(timelineLayout);

    return card;
}

QWidget* OrderDetailWindow::createActionsCard()
{
    actionsCard = new QFrame(this);
    actionsCard->setStyleSheet(QStringLiteral(
        "QFrame { background-color: white; border: 1px solid #E2E8F0; border-radius: 14px; padding: 18px; }"
    ));

    auto *vbox = new QVBoxLayout(actionsCard);
    vbox->setContentsMargins(20, 16, 20, 16);
    vbox->setSpacing(12);

    auto *title = new QLabel(QStringLiteral("Order Actions"), actionsCard);
    title->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 800; color: #0F172A;"));
    vbox->addWidget(title);

    actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(12);
    vbox->addLayout(actionsLayout);

    return actionsCard;
}

QWidget* OrderDetailWindow::createItemsCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background-color: white; border: 1px solid #E2E8F0; border-radius: 14px; padding: 20px; }"
    ));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(14);

    auto *title = new QLabel(QStringLiteral("Items in this Order"), card);
    title->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 800; color: #0F172A;"));
    layout->addWidget(title);

    itemsListLayout = new QVBoxLayout();
    itemsListLayout->setSpacing(12);
    layout->addLayout(itemsListLayout);

    return card;
}

QWidget* OrderDetailWindow::createAddressAndPaymentCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background-color: white; border: 1px solid #E2E8F0; border-radius: 14px; padding: 20px; }"
    ));

    auto *grid = new QGridLayout(card);
    grid->setContentsMargins(20, 16, 20, 16);
    grid->setHorizontalSpacing(32);
    grid->setVerticalSpacing(10);

    auto *addrHeader = new QLabel(QStringLiteral("Delivery Address"), card);
    addrHeader->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 800; color: #0F172A;"));
    grid->addWidget(addrHeader, 0, 0);

    addressDetailsLabel = new QLabel(card);
    addressDetailsLabel->setWordWrap(true);
    addressDetailsLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #475569; line-height: 1.4;"));
    grid->addWidget(addressDetailsLabel, 1, 0);

    auto *payHeader = new QLabel(QStringLiteral("Payment Summary"), card);
    payHeader->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 800; color: #0F172A;"));
    grid->addWidget(payHeader, 0, 1);

    paymentDetailsLabel = new QLabel(card);
    paymentDetailsLabel->setWordWrap(true);
    paymentDetailsLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #475569; line-height: 1.4;"));
    grid->addWidget(paymentDetailsLabel, 1, 1);

    totalAmountLabel = new QLabel(card);
    totalAmountLabel->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 900; color: #4F46E5; margin-top: 6px;"));
    grid->addWidget(totalAmountLabel, 2, 1);

    return card;
}

QPixmap OrderDetailWindow::loadOrGenerateCover(const QString &imagePath, const QString &title, const QString &author, int w, int h)
{
    if (!imagePath.trimmed().isEmpty()) {
        QFileInfo fi(imagePath);
        if (fi.exists() && fi.isFile()) {
            QPixmap pix(imagePath);
            if (!pix.isNull()) {
                return pix.scaled(w, h, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            }
        }
    }

    QPixmap pixmap(w, h);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient grad(0, 0, w, h);
    grad.setColorAt(0.0, QColor(0x4F, 0x46, 0xE5));
    grad.setColorAt(1.0, QColor(0x7C, 0x3A, 0xED));
    painter.fillRect(0, 0, w, h, grad);

    painter.setPen(Qt::white);
    QFont f = painter.font();
    f.setBold(true);
    f.setPointSize(10);
    painter.setFont(f);
    QRect titleRect(4, 8, w - 8, h / 2);
    painter.drawText(titleRect, Qt::AlignCenter | Qt::TextWordWrap, title.isEmpty() ? QStringLiteral("Book") : title);

    f.setBold(false);
    f.setPointSize(8);
    painter.setFont(f);
    QRect authorRect(4, h / 2 + 2, w - 8, h / 3);
    painter.drawText(authorRect, Qt::AlignCenter | Qt::TextWordWrap, author.isEmpty() ? QStringLiteral("Unknown") : author);

    painter.end();
    return pixmap;
}

QFrame* OrderDetailWindow::createOrderItemRow(const OrderItemSnapshot &item)
{
    auto *row = new QFrame();
    row->setStyleSheet(QStringLiteral("QFrame { background: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 10px; padding: 12px; }"));

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(16);

    auto *cover = new QLabel(row);
    cover->setFixedSize(50, 68);
    cover->setPixmap(loadOrGenerateCover(item.coverImage, item.title, item.author, 50, 68));
    cover->setStyleSheet(QStringLiteral("border-radius: 4px;"));
    layout->addWidget(cover);

    auto *info = new QVBoxLayout();
    info->setSpacing(3);

    auto *titleLbl = new QLabel(item.title, row);
    titleLbl->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: 700; color: #0F172A;"));
    info->addWidget(titleLbl);

    auto *authorLbl = new QLabel(QStringLiteral("by ") + item.author + QStringLiteral(" • Seller: ") + item.sellerUsername, row);
    authorLbl->setStyleSheet(QStringLiteral("font-size: 12px; color: #64748B;"));
    info->addWidget(authorLbl);

    auto *badges = new QHBoxLayout();
    badges->setSpacing(6);
    if (!item.condition.isEmpty()) {
        auto *cond = new QLabel(item.condition, row);
        cond->setStyleSheet(QStringLiteral("font-size: 10px; font-weight: 600; color: #374151; background: #E5E7EB; padding: 1px 6px; border-radius: 4px;"));
        badges->addWidget(cond);
    }
    badges->addStretch();
    info->addLayout(badges);

    layout->addLayout(info, 1);

    auto *priceCol = new QVBoxLayout();
    priceCol->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    priceCol->setSpacing(6);

    auto *priceLbl = new QLabel(QString("₹%1").arg(item.price, 0, 'f', 0), row);
    priceLbl->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 800; color: #0F172A;"));
    priceCol->addWidget(priceLbl);

    // If order delivered and current user is buyer, show Write Review button
    const QString currentUserId = SessionManager::instance().userId();
    if (m_order.status == QStringLiteral("DELIVERED") && m_order.buyerId == currentUserId) {
        auto *reviewBtn = new QPushButton(QStringLiteral("★ Write Review"), row);
        reviewBtn->setCursor(Qt::PointingHandCursor);
        reviewBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background: #EEF2FF; color: #4F46E5; border: 1px solid #C7D2FE; font-size: 11px; font-weight: 700; padding: 4px 8px; border-radius: 6px; }"
            "QPushButton:hover { background: #4F46E5; color: white; }"
        ));
        connect(reviewBtn, &QPushButton::clicked, this, [this, bId = item.bookId, bTitle = item.title]() {
            handleWriteReview(bId, bTitle);
        });
        priceCol->addWidget(reviewBtn);
    }

    layout->addLayout(priceCol);
    return row;
}

void OrderDetailWindow::loadOrder()
{
    auto *reply = ApiClient::instance().get(QStringLiteral("/api/orders/") + m_orderId);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!ok || !json.contains(QStringLiteral("order"))) {
            QMessageBox::warning(this, QStringLiteral("Error"), errorMsg.isEmpty() ? QStringLiteral("Unable to load order details.") : errorMsg);
            return;
        }

        m_order = OrderModel::fromJson(json[QStringLiteral("order")].toObject());

        orderNumberLabel->setText(QString("Order #%1").arg(m_order.orderNumber));
        dateLabel->setText(QString("Placed on: %1").arg(m_order.createdAt.toString(QStringLiteral("dd MMM yyyy, hh:mm AP"))));
        trackingLabel->setText(QString("Tracking ID: %1 • Shipping: %2")
                                   .arg(m_order.trackingId.isEmpty() ? QStringLiteral("Not assigned") : m_order.trackingId)
                                   .arg(m_order.shippingStatus.isEmpty() ? QStringLiteral("Standard") : m_order.shippingStatus));

        // Status Badge styling
        statusBadge->setText(m_order.status);
        if (m_order.status == QStringLiteral("DELIVERED")) {
            statusBadge->setStyleSheet(QStringLiteral("QLabel { background: #D1FAE5; color: #065F46; font-size: 13px; font-weight: 800; border-radius: 8px; padding: 0 16px; }"));
        } else if (m_order.status == QStringLiteral("CANCELLED") || m_order.status == QStringLiteral("RETURN_REJECTED")) {
            statusBadge->setStyleSheet(QStringLiteral("QLabel { background: #FEE2E2; color: #991B1B; font-size: 13px; font-weight: 800; border-radius: 8px; padding: 0 16px; }"));
        } else if (m_order.status == QStringLiteral("RETURN_REQUESTED") || m_order.status == QStringLiteral("RETURNED")) {
            statusBadge->setStyleSheet(QStringLiteral("QLabel { background: #FEF3C7; color: #92400E; font-size: 13px; font-weight: 800; border-radius: 8px; padding: 0 16px; }"));
        } else {
            statusBadge->setStyleSheet(QStringLiteral("QLabel { background: #DBEAFE; color: #1E40AF; font-size: 13px; font-weight: 800; border-radius: 8px; padding: 0 16px; }"));
        }

        // Rebuild Delivery Timeline
        QLayoutItem *child;
        while ((child = timelineLayout->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }

        QStringList standardStages = {
            QStringLiteral("CONFIRMED"),
            QStringLiteral("PACKED"),
            QStringLiteral("SHIPPED"),
            QStringLiteral("OUT_FOR_DELIVERY"),
            QStringLiteral("DELIVERED")
        };

        int currentIndex = standardStages.indexOf(m_order.status);
        if (m_order.status == QStringLiteral("PLACED")) currentIndex = 0;

        auto *stepperRow = new QHBoxLayout();
        stepperRow->setSpacing(8);

        for (int i = 0; i < standardStages.size(); ++i) {
            auto *stepBox = new QFrame();
            stepBox->setFrameShape(QFrame::StyledPanel);

            bool isReached = (currentIndex >= i);
            bool isCurrent = (currentIndex == i);

            QString stepBg = isReached ? (isCurrent ? QStringLiteral("#4F46E5") : QStringLiteral("#10B981")) : QStringLiteral("#E2E8F0");
            QString stepColor = isReached ? QStringLiteral("white") : QStringLiteral("#64748B");

            stepBox->setStyleSheet(QString("QFrame { background-color: %1; border-radius: 8px; padding: 8px; }").arg(stepBg));
            auto *boxL = new QVBoxLayout(stepBox);
            boxL->setContentsMargins(6, 4, 6, 4);
            boxL->setAlignment(Qt::AlignCenter);

            auto *sLabel = new QLabel(standardStages[i], stepBox);
            sLabel->setAlignment(Qt::AlignCenter);
            sLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 11px; font-weight: 700; }").arg(stepColor));
            boxL->addWidget(sLabel);

            stepperRow->addWidget(stepBox);
        }
        timelineLayout->addLayout(stepperRow);

        // History logs
        if (!m_order.history.isEmpty()) {
            auto *histBox = new QVBoxLayout();
            histBox->setSpacing(4);
            for (const auto &h : m_order.history) {
                auto *hLbl = new QLabel(QString("• [%1] %2%3")
                                            .arg(h.timestamp.toString(QStringLiteral("yyyy-MM-dd hh:mm")))
                                            .arg(h.status)
                                            .arg(h.note.isEmpty() ? QString() : QStringLiteral(" - ") + h.note));
                hLbl->setStyleSheet(QStringLiteral("color: #64748B; font-size: 11px;"));
                histBox->addWidget(hLbl);
            }
            timelineLayout->addLayout(histBox);
        }

        // Rebuild Items List
        while ((child = itemsListLayout->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }
        for (const auto &item : m_order.items) {
            itemsListLayout->addWidget(createOrderItemRow(item));
        }

        // Address & Payment
        addressDetailsLabel->setText(QString("%1\n%2\n%3, %4 - %5\nPhone: %6")
                                         .arg(m_order.shippingAddress.fullName)
                                         .arg(m_order.shippingAddress.addressLine)
                                         .arg(m_order.shippingAddress.city)
                                         .arg(m_order.shippingAddress.state)
                                         .arg(m_order.shippingAddress.postalCode)
                                         .arg(m_order.shippingAddress.phone));

        paymentDetailsLabel->setText(QString("Method: %1\nPayment Status: %2\nShipping Fee: ₹%3")
                                         .arg(m_order.paymentMethod)
                                         .arg(m_order.paymentStatus)
                                         .arg(m_order.shippingCost, 0, 'f', 2));

        totalAmountLabel->setText(QString("Total: ₹%1").arg(m_order.total, 0, 'f', 2));

        // Rebuild Dynamic Actions
        while ((child = actionsLayout->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }

        const QString currentUserId = SessionManager::instance().userId();
        const bool isBuyer = (m_order.buyerId == currentUserId);

        bool isSeller = false;
        for (const auto &it : m_order.items) {
            if (it.sellerId == currentUserId) {
                isSeller = true;
                break;
            }
        }

        auto createActionButton = [this](const QString &text, const QString &bg, const QString &hover) {
            auto *btn = new QPushButton(text);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setMinimumHeight(40);
            btn->setStyleSheet(QString(
                "QPushButton { background-color: %1; color: white; border-radius: 8px; font-weight: 700; font-size: 13px; padding: 0 16px; border: none; }"
                "QPushButton:hover { background-color: %2; }"
            ).arg(bg, hover));
            return btn;
        };

        bool hasAction = false;

        // Buyer actions
        if (isBuyer) {
            if (m_order.status == QStringLiteral("PLACED") ||
                m_order.status == QStringLiteral("CONFIRMED") ||
                m_order.status == QStringLiteral("PACKED")) {
                auto *cancelBtn = createActionButton(QStringLiteral("Cancel Order"), QStringLiteral("#EF4444"), QStringLiteral("#DC2626"));
                connect(cancelBtn, &QPushButton::clicked, this, &OrderDetailWindow::handleCancelOrder);
                actionsLayout->addWidget(cancelBtn);
                hasAction = true;
            } else if (m_order.status == QStringLiteral("DELIVERED")) {
                auto *returnBtn = createActionButton(QStringLiteral("Request Return"), QStringLiteral("#F59E0B"), QStringLiteral("#D97706"));
                connect(returnBtn, &QPushButton::clicked, this, &OrderDetailWindow::handleRequestReturn);
                actionsLayout->addWidget(returnBtn);
                hasAction = true;
            }
        }

        // Seller actions
        if (isSeller) {
            if (m_order.status == QStringLiteral("CONFIRMED")) {
                auto *packBtn = createActionButton(QStringLiteral("Mark Packed"), QStringLiteral("#4F46E5"), QStringLiteral("#4338CA"));
                connect(packBtn, &QPushButton::clicked, this, [this]() { handleUpdateStatus(QStringLiteral("PACKED")); });
                actionsLayout->addWidget(packBtn);
                hasAction = true;
            }
            if (m_order.status == QStringLiteral("CONFIRMED") || m_order.status == QStringLiteral("PACKED")) {
                auto *shipBtn = createActionButton(QStringLiteral("Mark Shipped"), QStringLiteral("#2563EB"), QStringLiteral("#1D4ED8"));
                connect(shipBtn, &QPushButton::clicked, this, [this]() { handleUpdateStatus(QStringLiteral("SHIPPED")); });
                actionsLayout->addWidget(shipBtn);
                hasAction = true;
            }
            if (m_order.status == QStringLiteral("SHIPPED")) {
                auto *outBtn = createActionButton(QStringLiteral("Mark Out for Delivery"), QStringLiteral("#0D9488"), QStringLiteral("#0F766E"));
                connect(outBtn, &QPushButton::clicked, this, [this]() { handleUpdateStatus(QStringLiteral("OUT_FOR_DELIVERY")); });
                actionsLayout->addWidget(outBtn);
                hasAction = true;
            }
            if (m_order.status == QStringLiteral("OUT_FOR_DELIVERY")) {
                auto *delBtn = createActionButton(QStringLiteral("Mark Delivered"), QStringLiteral("#10B981"), QStringLiteral("#059669"));
                connect(delBtn, &QPushButton::clicked, this, [this]() { handleUpdateStatus(QStringLiteral("DELIVERED")); });
                actionsLayout->addWidget(delBtn);
                hasAction = true;
            }
            if (m_order.status == QStringLiteral("RETURN_REQUESTED")) {
                auto *appBtn = createActionButton(QStringLiteral("Approve Return"), QStringLiteral("#10B981"), QStringLiteral("#059669"));
                connect(appBtn, &QPushButton::clicked, this, [this]() { handleProcessReturn(true); });
                actionsLayout->addWidget(appBtn);

                auto *rejBtn = createActionButton(QStringLiteral("Reject Return"), QStringLiteral("#EF4444"), QStringLiteral("#DC2626"));
                connect(rejBtn, &QPushButton::clicked, this, [this]() { handleProcessReturn(false); });
                actionsLayout->addWidget(rejBtn);
                hasAction = true;
            }
        }

        actionsLayout->addStretch();
        actionsCard->setVisible(hasAction);
    });
}

void OrderDetailWindow::handleCancelOrder()
{
    bool ok = false;
    QString reason = QInputDialog::getText(this, QStringLiteral("Cancel Order"),
                                          QStringLiteral("Please provide a reason for cancellation:"),
                                          QLineEdit::Normal, QString(), &ok);
    if (!ok || reason.trimmed().isEmpty()) return;

    QJsonObject payload;
    payload[QStringLiteral("reason")] = reason.trimmed();

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/orders/") + m_orderId + QStringLiteral("/cancel"), payload, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            QMessageBox::information(this, QStringLiteral("Order Cancelled"), QStringLiteral("Your order has been cancelled successfully."));
            loadOrder();
            emit orderUpdated();
        } else {
            QMessageBox::warning(this, QStringLiteral("Error"), errorMsg.isEmpty() ? QStringLiteral("Unable to cancel order.") : errorMsg);
        }
    });
}

void OrderDetailWindow::handleRequestReturn()
{
    bool ok = false;
    QString reason = QInputDialog::getText(this, QStringLiteral("Request Return"),
                                          QStringLiteral("Please state why you want to return this order:"),
                                          QLineEdit::Normal, QString(), &ok);
    if (!ok || reason.trimmed().isEmpty()) return;

    QJsonObject payload;
    payload[QStringLiteral("reason")] = reason.trimmed();

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/orders/") + m_orderId + QStringLiteral("/return"), payload, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            QMessageBox::information(this, QStringLiteral("Return Requested"), QStringLiteral("Your return request has been submitted to the seller."));
            loadOrder();
            emit orderUpdated();
        } else {
            QMessageBox::warning(this, QStringLiteral("Error"), errorMsg.isEmpty() ? QStringLiteral("Unable to request return.") : errorMsg);
        }
    });
}

void OrderDetailWindow::handleUpdateStatus(const QString &newStatus)
{
    QJsonObject payload;
    payload[QStringLiteral("status")] = newStatus;
    payload[QStringLiteral("note")] = QString("Status updated to %1").arg(newStatus);

    auto *reply = ApiClient::instance().patch(QStringLiteral("/api/orders/") + m_orderId + QStringLiteral("/status"), payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply, newStatus]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            QMessageBox::information(this, QStringLiteral("Status Updated"), QString("Order updated to %1 successfully.").arg(newStatus));
            loadOrder();
            emit orderUpdated();
        } else {
            QMessageBox::warning(this, QStringLiteral("Error"), errorMsg.isEmpty() ? QStringLiteral("Failed to update order status.") : errorMsg);
        }
    });
}

void OrderDetailWindow::handleProcessReturn(bool approve)
{
    bool ok = false;
    QString note = QInputDialog::getText(this, approve ? QStringLiteral("Approve Return") : QStringLiteral("Reject Return"),
                                        QStringLiteral("Add a note for the buyer:"),
                                        QLineEdit::Normal, QString(), &ok);
    if (!ok) return;

    QJsonObject payload;
    payload[QStringLiteral("approve")] = approve;
    payload[QStringLiteral("note")] = note.trimmed();

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/orders/") + m_orderId + QStringLiteral("/return/process"), payload, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply, approve]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            QMessageBox::information(this, QStringLiteral("Return Processed"),
                                     approve ? QStringLiteral("Return has been approved.") : QStringLiteral("Return has been rejected."));
            loadOrder();
            emit orderUpdated();
        } else {
            QMessageBox::warning(this, QStringLiteral("Error"), errorMsg.isEmpty() ? QStringLiteral("Failed to process return.") : errorMsg);
        }
    });
}

void OrderDetailWindow::handleWriteReview(const QString &bookId, const QString &bookTitle)
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Write Review"));
    dialog.resize(420, 300);
    dialog.setStyleSheet(QStringLiteral("background-color: white; font-family: 'Segoe UI', Arial, sans-serif;"));

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto *titleLbl = new QLabel(QString("Review '%1'").arg(bookTitle), &dialog);
    titleLbl->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 800; color: #0F172A;"));
    layout->addWidget(titleLbl);

    auto *rateRow = new QHBoxLayout();
    auto *rateLbl = new QLabel(QStringLiteral("Rating (1 to 5 Stars):"), &dialog);
    rateLbl->setStyleSheet(QStringLiteral("font-weight: 600; color: #475569; font-size: 13px;"));
    auto *spin = new QSpinBox(&dialog);
    spin->setRange(1, 5);
    spin->setValue(5);
    spin->setMinimumWidth(80);
    rateRow->addWidget(rateLbl);
    rateRow->addStretch();
    rateRow->addWidget(spin);
    layout->addLayout(rateRow);

    auto *commLbl = new QLabel(QStringLiteral("Your Review / Comment:"), &dialog);
    commLbl->setStyleSheet(QStringLiteral("font-weight: 600; color: #475569; font-size: 13px;"));
    layout->addWidget(commLbl);

    auto *commentEdit = new QTextEdit(&dialog);
    commentEdit->setPlaceholderText(QStringLiteral("Share what you thought about this book..."));
    commentEdit->setStyleSheet(QStringLiteral("background: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 8px; padding: 8px;"));
    layout->addWidget(commentEdit);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *cancelBtn = new QPushButton(QStringLiteral("Cancel"), &dialog);
    cancelBtn->setStyleSheet(QStringLiteral("padding: 8px 16px; border-radius: 6px; font-weight: 600; background: #F1F5F9; color: #334155;"));
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    btnRow->addWidget(cancelBtn);

    auto *submitBtn = new QPushButton(QStringLiteral("Submit Review"), &dialog);
    submitBtn->setStyleSheet(QStringLiteral("padding: 8px 16px; border-radius: 6px; font-weight: 700; background: #4F46E5; color: white;"));
    btnRow->addWidget(submitBtn);
    layout->addLayout(btnRow);

    connect(submitBtn, &QPushButton::clicked, [&dialog, bookId, spin, commentEdit, this]() {
        const QString comment = commentEdit->toPlainText().trimmed();
        if (comment.isEmpty()) {
            QMessageBox::warning(&dialog, QStringLiteral("Validation"), QStringLiteral("Please write a comment."));
            return;
        }

        QJsonObject payload;
        payload[QStringLiteral("orderId")] = m_orderId;
        payload[QStringLiteral("bookId")] = bookId;
        payload[QStringLiteral("rating")] = spin->value();
        payload[QStringLiteral("comment")] = comment;

        auto *reply = ApiClient::instance().post(QStringLiteral("/api/reviews"), payload, true);
        connect(reply, &QNetworkReply::finished, [reply, &dialog, this]() {
            auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
            reply->deleteLater();

            if (ok) {
                QMessageBox::information(this, QStringLiteral("Review Submitted"), QStringLiteral("Thank you! Your review has been submitted successfully."));
                dialog.accept();
            } else {
                QMessageBox::warning(&dialog, QStringLiteral("Review Error"), errorMsg.isEmpty() ? QStringLiteral("Failed to submit review.") : errorMsg);
            }
        });
    });

    dialog.exec();
}
