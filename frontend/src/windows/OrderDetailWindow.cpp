#include "windows/OrderDetailWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include "StyledMessageBox.h"
#include <QInputDialog>
#include <QDialog>
#include <QSpinBox>
#include <QTextEdit>
#include <QPainter>
#include <QFileInfo>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

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
    setWindowTitle("BookBazzar - Order Details");
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

    auto *content = new QWidget();
    content->setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 28, 40, 48);
    contentLayout->setSpacing(24);

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

    auto *backBtn = new QPushButton("← Back", topBar);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setFixedSize(88, 38);
    backBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(backBtn, &QPushButton::clicked, this, &OrderDetailWindow::backRequested);
    layout->addWidget(backBtn);

    auto *title = new QLabel("Order Overview & Tracking", topBar);
    title->setStyleSheet(QString("font-size: 20px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    layout->addWidget(title);

    layout->addStretch();

    auto *refreshBtn = new QPushButton("↻ Refresh", topBar);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setFixedHeight(38);
    refreshBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(refreshBtn, &QPushButton::clicked, this, &OrderDetailWindow::loadOrder);
    layout->addWidget(refreshBtn);

    return topBar;
}

QWidget* OrderDetailWindow::createStatusHeader()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 16, 4, 12);

    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(20);

    auto *info = new QVBoxLayout();
    info->setSpacing(4);

    orderNumberLabel = new QLabel("Order #...", card);
    orderNumberLabel->setStyleSheet(QString("font-size: 19px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    info->addWidget(orderNumberLabel);

    dateLabel = new QLabel("Placed on: ...", card);
    dateLabel->setStyleSheet(QString("font-size: 13px; color: %1;").arg(AppStyle::TextMuted));
    info->addWidget(dateLabel);

    trackingLabel = new QLabel("Tracking ID: ...", card);
    trackingLabel->setStyleSheet(QString("font-size: 12px; color: %1; font-weight: 600;").arg(AppStyle::TextSecondary));
    info->addWidget(trackingLabel);

    layout->addLayout(info, 1);

    statusBadge = new QLabel("CONFIRMED", card);
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setFixedHeight(36);
    statusBadge->setStyleSheet(AppStyle::statusBadgeStyle("CONFIRMED"));
    layout->addWidget(statusBadge);

    return card;
}

QWidget* OrderDetailWindow::createTimelineCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 16, 4, 12);

    auto *mainV = new QVBoxLayout(card);
    mainV->setContentsMargins(24, 20, 24, 20);
    mainV->setSpacing(16);

    auto *title = new QLabel("Delivery Timeline & Status", card);
    title->setStyleSheet(QString("font-size: 16px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    mainV->addWidget(title);

    timelineLayout = new QVBoxLayout();
    timelineLayout->setSpacing(8);
    mainV->addLayout(timelineLayout);

    return card;
}

QWidget* OrderDetailWindow::createActionsCard()
{
    actionsCard = new QFrame(this);
    actionsCard->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(actionsCard, 16, 4, 12);

    auto *vbox = new QVBoxLayout(actionsCard);
    vbox->setContentsMargins(24, 20, 24, 20);
    vbox->setSpacing(14);

    auto *title = new QLabel("Manage Order & Actions", actionsCard);
    title->setStyleSheet(QString("font-size: 16px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    vbox->addWidget(title);

    actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(12);
    vbox->addLayout(actionsLayout);

    return actionsCard;
}

QWidget* OrderDetailWindow::createItemsCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 16, 4, 12);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto *title = new QLabel("Items in this Order", card);
    title->setStyleSheet(QString("font-size: 16px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    layout->addWidget(title);

    itemsListLayout = new QVBoxLayout();
    itemsListLayout->setSpacing(12);
    layout->addLayout(itemsListLayout);

    return card;
}

QWidget* OrderDetailWindow::createAddressAndPaymentCard()
{
    auto *card = new QFrame(this);
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 16, 4, 12);

    auto *grid = new QGridLayout(card);
    grid->setContentsMargins(24, 20, 24, 20);
    grid->setHorizontalSpacing(36);
    grid->setVerticalSpacing(10);

    auto *addrHeader = new QLabel("Delivery Address", card);
    addrHeader->setStyleSheet(QString("font-size: 15px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    grid->addWidget(addrHeader, 0, 0);

    addressDetailsLabel = new QLabel(card);
    addressDetailsLabel->setWordWrap(true);
    addressDetailsLabel->setStyleSheet(QString("font-size: 13px; color: %1; line-height: 1.4;").arg(AppStyle::TextSecondary));
    grid->addWidget(addressDetailsLabel, 1, 0);

    auto *payHeader = new QLabel("Payment Summary", card);
    payHeader->setStyleSheet(QString("font-size: 15px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    grid->addWidget(payHeader, 0, 1);

    paymentDetailsLabel = new QLabel(card);
    paymentDetailsLabel->setWordWrap(true);
    paymentDetailsLabel->setStyleSheet(QString("font-size: 13px; color: %1; line-height: 1.4;").arg(AppStyle::TextSecondary));
    grid->addWidget(paymentDetailsLabel, 1, 1);

    totalAmountLabel = new QLabel(card);
    totalAmountLabel->setStyleSheet(QString("font-size: 20px; font-weight: 900; color: %1; margin-top: 6px;").arg(AppStyle::Primary));
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

    painter.fillRect(0, 0, w, h, QColor("#0F172A"));
    painter.setPen(Qt::white);
    QFont f(AppStyle::appFont(), 8, QFont::Bold);
    painter.setFont(f);
    QRect titleRect(4, 6, w - 8, h / 2);
    painter.drawText(titleRect, Qt::AlignCenter | Qt::TextWordWrap, title.isEmpty() ? "Book" : title);
    painter.end();
    return pixmap;
}

QFrame* OrderDetailWindow::createOrderItemRow(const OrderItemSnapshot &item)
{
    auto *row = new QFrame();
    row->setStyleSheet(QString(R"(
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

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(16);

    auto *cover = new QLabel(row);
    cover->setFixedSize(50, 68);
    cover->setPixmap(loadOrGenerateCover(item.coverImage, item.title, item.author, 50, 68));
    cover->setStyleSheet("border-radius: 4px;");
    layout->addWidget(cover);

    auto *info = new QVBoxLayout();
    info->setSpacing(3);

    auto *titleLbl = new QLabel(item.title, row);
    titleLbl->setStyleSheet(QString("font-size: 14px; font-weight: 700; color: %1;").arg(AppStyle::TextPrimary));
    info->addWidget(titleLbl);

    auto *authorLbl = new QLabel("by " + item.author + " • Seller: " + item.sellerUsername, row);
    authorLbl->setStyleSheet(QString("font-size: 12px; color: %1;").arg(AppStyle::TextSecondary));
    info->addWidget(authorLbl);

    auto *badges = new QHBoxLayout();
    badges->setSpacing(6);
    if (!item.condition.isEmpty()) {
        auto *cond = new QLabel(item.condition, row);
        cond->setStyleSheet(AppStyle::statusBadgeStyle(item.condition));
        badges->addWidget(cond);
    }
    badges->addStretch();
    info->addLayout(badges);

    layout->addLayout(info, 1);

    auto *priceCol = new QVBoxLayout();
    priceCol->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    priceCol->setSpacing(6);

    auto *priceLbl = new QLabel(QString("₹%1").arg(item.price, 0, 'f', 0), row);
    priceLbl->setStyleSheet(QString("font-size: 16px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    priceCol->addWidget(priceLbl);

    // If order delivered and current user is buyer, show Write Review button
    const QString currentUserId = SessionManager::instance().userId();
    if (m_order.status == "DELIVERED" && m_order.buyerId == currentUserId) {
        auto *reviewBtn = new QPushButton("★ Write Review", row);
        reviewBtn->setCursor(Qt::PointingHandCursor);
        reviewBtn->setStyleSheet(AppStyle::ghostButtonStyle());
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
            StyledMessageBox::warning(this, "Error", errorMsg.isEmpty() ? "Unable to load order details." : errorMsg);
            return;
        }

        m_order = OrderModel::fromJson(json[QStringLiteral("order")].toObject());

        orderNumberLabel->setText(QString("Order #%1").arg(m_order.orderNumber));
        dateLabel->setText(QString("Placed on: %1").arg(m_order.createdAt.toString("dd MMM yyyy, hh:mm AP")));
        trackingLabel->setText(QString("Tracking ID: %1").arg(m_order.trackingId.isEmpty() ? "Assigned upon dispatch" : m_order.trackingId));

        statusBadge->setText(m_order.status);
        statusBadge->setStyleSheet(AppStyle::statusBadgeStyle(m_order.status));

        // Rebuild Timeline
        QLayoutItem *child;
        while ((child = timelineLayout->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }

        const QStringList standardStages = {
            "CONFIRMED", "PACKED", "SHIPPED", "OUT_FOR_DELIVERY", "DELIVERED"
        };

        int currentIndex = standardStages.indexOf(m_order.status);
        if (m_order.status == "PLACED") currentIndex = 0;

        auto *stepperRow = new QHBoxLayout();
        stepperRow->setSpacing(8);

        for (int i = 0; i < standardStages.size(); ++i) {
            auto *stepBox = new QFrame();
            bool isReached = (currentIndex >= i);
            bool isCurrent = (currentIndex == i);

            QString stepBg = isReached ? (isCurrent ? AppStyle::Primary : AppStyle::Success) : AppStyle::BorderSubtle;
            QString stepColor = isReached ? "#FFFFFF" : AppStyle::TextMuted;

            stepBox->setStyleSheet(QString("background-color: %1; border-radius: 8px;").arg(stepBg));
            auto *boxL = new QVBoxLayout(stepBox);
            boxL->setContentsMargins(10, 6, 10, 6);
            boxL->setAlignment(Qt::AlignCenter);

            auto *sLabel = new QLabel(standardStages[i], stepBox);
            sLabel->setAlignment(Qt::AlignCenter);
            sLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 700;").arg(stepColor));
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
                                            .arg(h.timestamp.toString("yyyy-MM-dd hh:mm"))
                                            .arg(h.status)
                                            .arg(h.note.isEmpty() ? QString() : " - " + h.note));
                hLbl->setStyleSheet(QString("color: %1; font-size: 11px;").arg(AppStyle::TextMuted));
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

        // Rebuild Actions
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

        bool hasAction = false;

        // Buyer actions
        if (isBuyer) {
            if (m_order.status == "PLACED" || m_order.status == "CONFIRMED" || m_order.status == "PACKED") {
                auto *cancelBtn = new QPushButton("Cancel Order", actionsCard);
                cancelBtn->setCursor(Qt::PointingHandCursor);
                cancelBtn->setStyleSheet(AppStyle::dangerButtonStyle());
                connect(cancelBtn, &QPushButton::clicked, this, &OrderDetailWindow::handleCancelOrder);
                actionsLayout->addWidget(cancelBtn);
                hasAction = true;
            } else if (m_order.status == "DELIVERED") {
                auto *returnBtn = new QPushButton("Request Return", actionsCard);
                returnBtn->setCursor(Qt::PointingHandCursor);
                returnBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
                connect(returnBtn, &QPushButton::clicked, this, &OrderDetailWindow::handleRequestReturn);
                actionsLayout->addWidget(returnBtn);
                hasAction = true;
            }
        }

        // Seller actions
        if (isSeller) {
            if (m_order.status == "CONFIRMED") {
                auto *packBtn = new QPushButton("Mark Packed", actionsCard);
                packBtn->setCursor(Qt::PointingHandCursor);
                packBtn->setStyleSheet(AppStyle::primaryButtonStyle());
                connect(packBtn, &QPushButton::clicked, this, [this]() { handleUpdateStatus("PACKED"); });
                actionsLayout->addWidget(packBtn);
                hasAction = true;
            }
            if (m_order.status == "CONFIRMED" || m_order.status == "PACKED") {
                auto *shipBtn = new QPushButton("Mark Shipped", actionsCard);
                shipBtn->setCursor(Qt::PointingHandCursor);
                shipBtn->setStyleSheet(AppStyle::primaryButtonStyle());
                connect(shipBtn, &QPushButton::clicked, this, [this]() { handleUpdateStatus("SHIPPED"); });
                actionsLayout->addWidget(shipBtn);
                hasAction = true;
            }
            if (m_order.status == "SHIPPED") {
                auto *outBtn = new QPushButton("Mark Out for Delivery", actionsCard);
                outBtn->setCursor(Qt::PointingHandCursor);
                outBtn->setStyleSheet(AppStyle::primaryButtonStyle());
                connect(outBtn, &QPushButton::clicked, this, [this]() { handleUpdateStatus("OUT_FOR_DELIVERY"); });
                actionsLayout->addWidget(outBtn);
                hasAction = true;
            }
            if (m_order.status == "OUT_FOR_DELIVERY") {
                auto *delBtn = new QPushButton("Mark Delivered", actionsCard);
                delBtn->setCursor(Qt::PointingHandCursor);
                delBtn->setStyleSheet(AppStyle::primaryButtonStyle());
                connect(delBtn, &QPushButton::clicked, this, [this]() { handleUpdateStatus("DELIVERED"); });
                actionsLayout->addWidget(delBtn);
                hasAction = true;
            }
            if (m_order.status == "RETURN_REQUESTED") {
                auto *appBtn = new QPushButton("Approve Return", actionsCard);
                appBtn->setCursor(Qt::PointingHandCursor);
                appBtn->setStyleSheet(AppStyle::primaryButtonStyle());
                connect(appBtn, &QPushButton::clicked, this, [this]() { handleProcessReturn(true); });
                actionsLayout->addWidget(appBtn);

                auto *rejBtn = new QPushButton("Reject Return", actionsCard);
                rejBtn->setCursor(Qt::PointingHandCursor);
                rejBtn->setStyleSheet(AppStyle::dangerButtonStyle());
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
    QString reason = QInputDialog::getText(
        this, "Cancel Order",
        "Please provide a reason for cancellation:",
        QLineEdit::Normal, QString(), &ok
    );
    if (!ok || reason.trimmed().isEmpty()) return;

    QJsonObject payload;
    payload[QStringLiteral("reason")] = reason.trimmed();

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/orders/") + m_orderId + QStringLiteral("/cancel"), payload, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            StyledMessageBox::success(this, "Order Cancelled", "Your order has been cancelled successfully.");
            loadOrder();
            emit orderUpdated();
            emit orderCancelled();
        } else {
            StyledMessageBox::warning(this, "Error", errorMsg.isEmpty() ? "Unable to cancel order." : errorMsg);
        }
    });
}

void OrderDetailWindow::handleRequestReturn()
{
    bool ok = false;
    QString reason = QInputDialog::getText(
        this, "Request Return",
        "Please state why you want to return this order:",
        QLineEdit::Normal, QString(), &ok
    );
    if (!ok || reason.trimmed().isEmpty()) return;

    QJsonObject payload;
    payload[QStringLiteral("reason")] = reason.trimmed();

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/orders/") + m_orderId + QStringLiteral("/return"), payload, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            StyledMessageBox::information(this, "Return Requested", "Your return request has been submitted to the seller.");
            loadOrder();
            emit orderUpdated();
            emit orderStatusUpdated();
        } else {
            StyledMessageBox::warning(this, "Error", errorMsg.isEmpty() ? "Unable to request return." : errorMsg);
        }
    });
}

void OrderDetailWindow::handleUpdateStatus(const QString &newStatus)
{
    QJsonObject payload;
    payload[QStringLiteral("status")] = newStatus;

    auto *reply = ApiClient::instance().patch(QStringLiteral("/api/orders/") + m_orderId + QStringLiteral("/status"), payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            loadOrder();
            emit orderUpdated();
            emit orderStatusUpdated();
        } else {
            StyledMessageBox::warning(this, "Status Update Failed", errorMsg.isEmpty() ? "Unable to update order status." : errorMsg);
        }
    });
}

void OrderDetailWindow::handleProcessReturn(bool approve)
{
    QJsonObject payload;
    payload[QStringLiteral("action")] = approve ? QStringLiteral("approve") : QStringLiteral("reject");

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/orders/") + m_orderId + QStringLiteral("/return/process"), payload, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            loadOrder();
            emit orderUpdated();
            emit orderStatusUpdated();
        } else {
            StyledMessageBox::warning(this, "Process Return Failed", errorMsg.isEmpty() ? "Unable to process return." : errorMsg);
        }
    });
}

static QString copyConditionPhotoToUploads(const QString &sourcePath)
{
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists()) {
        return QString();
    }

    const QString extension = sourceInfo.suffix().toLower();
    const QString fileName = sourceInfo.completeBaseName() + "_" +
                             QString::number(QDateTime::currentMSecsSinceEpoch()) +
                             "." + extension;

    const QStringList candidates = {
        QStringLiteral("d:/1Hello-World/1pbl-oops/backend/public/uploads"),
        QDir::currentPath() + QStringLiteral("/../backend/public/uploads"),
        QDir::currentPath() + QStringLiteral("/../../backend/public/uploads"),
        QDir::currentPath() + QStringLiteral("/backend/public/uploads")
    };

    QString targetDir;
    for (const auto &c : candidates) {
        QDir d(c);
        if (d.exists()) {
            targetDir = d.canonicalPath();
            break;
        }
    }

    if (!targetDir.isEmpty()) {
        const QString destFile = targetDir + "/" + fileName;
        if (QFile::copy(sourcePath, destFile)) {
            return QStringLiteral("http://localhost:8080/uploads/") + fileName;
        }
    }

    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/uploads";
    QDir(appDataPath).mkpath(".");
    const QString fallbackFile = appDataPath + "/" + fileName;
    if (QFile::copy(sourcePath, fallbackFile)) {
        return fallbackFile;
    }
    return QString();
}

void OrderDetailWindow::handleWriteReview(const QString &bookId, const QString &bookTitle)
{
    QString sellerName = "Seller";
    for (const auto &it : m_order.items) {
        if (it.bookId == bookId && !it.sellerUsername.isEmpty()) {
            sellerName = it.sellerUsername;
            break;
        }
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Rate & Review Seller");
    dialog.resize(520, 560);
    dialog.setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(14);

    QFrame *card = new QFrame(&dialog);
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 16, 4, 12);

    auto *cLayout = new QVBoxLayout(card);
    cLayout->setContentsMargins(22, 20, 22, 20);
    cLayout->setSpacing(12);

    auto *tLabel = new QLabel(QString("Rate Seller: %1").arg(sellerName), card);
    tLabel->setStyleSheet(QString("font-size: 17px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    cLayout->addWidget(tLabel);

    auto *subLabel = new QLabel(QString("Book Received: %1").arg(bookTitle), card);
    subLabel->setStyleSheet(QString("font-size: 12px; color: %1;").arg(AppStyle::TextSecondary));
    cLayout->addWidget(subLabel);

    auto *descLabel = new QLabel("Share your experience with the seller's packaging, delivery speed, and the accuracy of the book condition.", card);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(QString("font-size: 12px; color: %1;").arg(AppStyle::TextMuted));
    cLayout->addWidget(descLabel);

    auto *rRow = new QHBoxLayout();
    auto *rLabel = new QLabel("Seller Rating:", card);
    rLabel->setStyleSheet(QString("font-weight: 700; color: %1; font-size: 13px;").arg(AppStyle::TextSecondary));
    auto *rSpin = new QSpinBox(card);
    rSpin->setRange(1, 5);
    rSpin->setValue(5);
    rSpin->setPrefix("★ ");
    rSpin->setSuffix(" Stars");
    rSpin->setStyleSheet(AppStyle::inputStyle());
    rRow->addWidget(rLabel);
    rRow->addWidget(rSpin);
    rRow->addStretch();
    cLayout->addLayout(rRow);

    auto *cHead = new QLabel("Feedback on Seller & Received Condition:", card);
    cHead->setStyleSheet(QString("font-weight: 700; color: %1; font-size: 13px;").arg(AppStyle::TextSecondary));
    cLayout->addWidget(cHead);

    auto *commentEdit = new QTextEdit(card);
    commentEdit->setPlaceholderText("Describe the condition of the book you received, packaging quality, and seller communication...");
    commentEdit->setStyleSheet(AppStyle::inputStyle());
    commentEdit->setMaximumHeight(90);
    cLayout->addWidget(commentEdit);

    // Photos of received condition
    auto *photoHead = new QLabel("Condition Photos Received (Optional):", card);
    photoHead->setStyleSheet(QString("font-weight: 700; color: %1; font-size: 13px;").arg(AppStyle::TextSecondary));
    cLayout->addWidget(photoHead);

    auto *photosRow = new QHBoxLayout();
    photosRow->setSpacing(8);
    auto *photosWidget = new QWidget(card);
    photosWidget->setLayout(photosRow);

    auto *addPhotoBtn = new QPushButton("📷 Add Photos", card);
    addPhotoBtn->setCursor(Qt::PointingHandCursor);
    addPhotoBtn->setStyleSheet(AppStyle::secondaryButtonStyle());

    auto *photoContainer = new QHBoxLayout();
    photoContainer->addWidget(addPhotoBtn);
    photoContainer->addWidget(photosWidget, 1);
    cLayout->addLayout(photoContainer);

    QStringList attachedPhotoPaths;

    auto updatePhotosUI = [&attachedPhotoPaths, photosRow, photosWidget]() {
        QLayoutItem *child;
        while ((child = photosRow->takeAt(0)) != nullptr) {
            if (child->widget()) delete child->widget();
            delete child;
        }
        for (const auto &p : attachedPhotoPaths) {
            QLabel *thumb = new QLabel(photosWidget);
            thumb->setFixedSize(54, 54);
            thumb->setStyleSheet("border: 1px solid #CBD5E1; border-radius: 6px; background-color: #F1F5F9;");
            QPixmap pix(p);
            if (!pix.isNull()) {
                thumb->setPixmap(pix.scaled(52, 52, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            }
            photosRow->addWidget(thumb);
        }
        photosRow->addStretch();
    };

    connect(addPhotoBtn, &QPushButton::clicked, [&dialog, &attachedPhotoPaths, updatePhotosUI]() {
        QStringList files = QFileDialog::getOpenFileNames(
            &dialog,
            "Select Photos of Received Book",
            QString(),
            "Images (*.png *.jpg *.jpeg *.webp)"
        );
        for (const auto &f : files) {
            if (attachedPhotoPaths.size() < 4 && !attachedPhotoPaths.contains(f)) {
                attachedPhotoPaths.append(f);
            }
        }
        updatePhotosUI();
    });

    layout->addWidget(card);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *cancelBtn = new QPushButton("Cancel", &dialog);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    btnRow->addWidget(cancelBtn);

    auto *subBtn = new QPushButton("Submit Seller Review", &dialog);
    subBtn->setCursor(Qt::PointingHandCursor);
    subBtn->setStyleSheet(AppStyle::primaryButtonStyle());
    btnRow->addWidget(subBtn);
    layout->addLayout(btnRow);

    connect(subBtn, &QPushButton::clicked, [&dialog, bookId, rSpin, commentEdit, &attachedPhotoPaths, subBtn, this]() {
        QString comment = commentEdit->toPlainText().trimmed();
        if (comment.isEmpty()) {
            StyledMessageBox::warning(&dialog, "Missing Review", "Please write a brief comment describing your experience with the seller and the received condition.");
            return;
        }

        subBtn->setEnabled(false);
        subBtn->setText("Submitting...");

        QJsonArray imagesArray;
        for (const auto &p : attachedPhotoPaths) {
            QString uploaded = copyConditionPhotoToUploads(p);
            if (!uploaded.isEmpty()) {
                imagesArray.append(uploaded);
            }
        }

        QJsonObject body;
        body[QStringLiteral("orderId")] = m_orderId;
        body[QStringLiteral("bookId")] = bookId;
        body[QStringLiteral("rating")] = rSpin->value();
        body[QStringLiteral("comment")] = comment;
        if (!imagesArray.isEmpty()) {
            body[QStringLiteral("images")] = imagesArray;
        }

        auto *reply = ApiClient::instance().post(QStringLiteral("/api/reviews"), body, true);
        connect(reply, &QNetworkReply::finished, [reply, &dialog, subBtn, this]() {
            auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
            reply->deleteLater();

            subBtn->setEnabled(true);
            subBtn->setText("Submit Seller Review");

            if (ok) {
                StyledMessageBox::success(this, "Review Submitted", "Thank you! Your seller rating and condition photos have been published.");
                dialog.accept();
                loadOrder();
            } else {
                StyledMessageBox::warning(&dialog, "Submission Failed", errorMsg.isEmpty() ? "Unable to submit review." : errorMsg);
            }
        });
    });

    dialog.exec();
}
