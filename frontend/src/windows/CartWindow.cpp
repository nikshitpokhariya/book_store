#include "windows/CartWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QPainter>
#include <QPainterPath>
#include <QMessageBox>
#include <QFileInfo>
#include <QNetworkReply>

CartWindow::CartWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    refreshCart();
}

void CartWindow::setupUI()
{
    setWindowTitle(QStringLiteral("BookBazzar - Shopping Cart"));
    resize(1000, 700);
    setMinimumSize(850, 550);
    setStyleSheet(QStringLiteral("background-color: #F8F9FD; font-family: 'Segoe UI', Arial, sans-serif;"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Top Bar
    mainLayout->addWidget(createTopBar());

    // Body content
    auto *bodyWidget = new QWidget(this);
    auto *bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(32, 24, 32, 32);
    bodyLayout->setSpacing(28);

    // Left Column: Items List & Empty State
    auto *leftCol = new QWidget(bodyWidget);
    auto *leftLayout = new QVBoxLayout(leftCol);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);

    scrollArea = new QScrollArea(leftCol);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    itemsContainer = new QWidget();
    itemsContainer->setStyleSheet(QStringLiteral("background: transparent;"));
    itemsLayout = new QVBoxLayout(itemsContainer);
    itemsLayout->setContentsMargins(0, 0, 8, 0);
    itemsLayout->setSpacing(14);
    itemsLayout->addStretch();

    scrollArea->setWidget(itemsContainer);
    leftLayout->addWidget(scrollArea);

    // Empty state widget
    emptyStateWidget = new QWidget(leftCol);
    auto *emptyLayout = new QVBoxLayout(emptyStateWidget);
    emptyLayout->setContentsMargins(40, 60, 40, 60);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(14);

    auto *emptyIcon = new QLabel(QStringLiteral("🛒"), emptyStateWidget);
    emptyIcon->setStyleSheet(QStringLiteral("font-size: 64px;"));
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyIcon);

    auto *emptyTitle = new QLabel(QStringLiteral("Your Cart is Empty"), emptyStateWidget);
    emptyTitle->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 800; color: #1E293B;"));
    emptyTitle->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyTitle);

    auto *emptySub = new QLabel(QStringLiteral("Explore our book exchange & marketplace to find amazing books."), emptyStateWidget);
    emptySub->setStyleSheet(QStringLiteral("font-size: 14px; color: #64748B;"));
    emptySub->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptySub);

    auto *exploreBtn = new QPushButton(QStringLiteral("Browse Books"), emptyStateWidget);
    exploreBtn->setCursor(Qt::PointingHandCursor);
    exploreBtn->setFixedSize(160, 42);
    exploreBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #4F46E5; color: white; border-radius: 8px; font-weight: 700; font-size: 14px; }"
        "QPushButton:hover { background-color: #4338CA; }"
    ));
    connect(exploreBtn, &QPushButton::clicked, this, &CartWindow::browseRequested);
    emptyLayout->addWidget(exploreBtn, 0, Qt::AlignCenter);

    emptyStateWidget->setVisible(false);
    leftLayout->addWidget(emptyStateWidget);

    bodyLayout->addWidget(leftCol, 2);

    // Right Column: Summary Card
    bodyLayout->addWidget(createSummaryCard(), 1);

    mainLayout->addWidget(bodyWidget, 1);
}

QWidget* CartWindow::createTopBar()
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
    connect(backBtn, &QPushButton::clicked, this, &CartWindow::backRequested);
    layout->addWidget(backBtn);

    auto *title = new QLabel(QStringLiteral("Shopping Cart"), topBar);
    title->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 800; color: #0F172A;"));
    layout->addWidget(title);

    layout->addStretch();

    auto *refreshBtn = new QPushButton(QStringLiteral("↻ Refresh"), topBar);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: #4F46E5; border: 1px solid #C7D2FE; padding: 6px 14px; border-radius: 8px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #EEF2FF; }"
    ));
    connect(refreshBtn, &QPushButton::clicked, this, &CartWindow::refreshCart);
    layout->addWidget(refreshBtn);

    return topBar;
}

QWidget* CartWindow::createSummaryCard()
{
    auto *card = new QFrame(this);
    card->setObjectName(QStringLiteral("summaryCard"));
    card->setStyleSheet(QStringLiteral(
        "QFrame#summaryCard { background-color: white; border: 1px solid #E2E8F0; border-radius: 16px; padding: 20px; }"
    ));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(14);

    auto *heading = new QLabel(QStringLiteral("Order Summary"), card);
    heading->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 800; color: #0F172A;"));
    layout->addWidget(heading);

    // Row: Items count & subtotal
    auto *subtotalRow = new QHBoxLayout();
    itemCountLabel = new QLabel(QStringLiteral("Subtotal (0 items)"), card);
    itemCountLabel->setStyleSheet(QStringLiteral("font-size: 14px; color: #64748B;"));
    subtotalLabel = new QLabel(QStringLiteral("₹0.00"), card);
    subtotalLabel->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 700; color: #1E293B;"));
    subtotalRow->addWidget(itemCountLabel);
    subtotalRow->addStretch();
    subtotalRow->addWidget(subtotalLabel);
    layout->addLayout(subtotalRow);

    // Row: Shipping
    auto *shippingRow = new QHBoxLayout();
    auto *shippingTitle = new QLabel(QStringLiteral("Shipping"), card);
    shippingTitle->setStyleSheet(QStringLiteral("font-size: 14px; color: #64748B;"));
    shippingLabel = new QLabel(QStringLiteral("FREE"), card);
    shippingLabel->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 800; color: #059669; background: #D1FAE5; padding: 2px 8px; border-radius: 4px;"));
    shippingRow->addWidget(shippingTitle);
    shippingRow->addStretch();
    shippingRow->addWidget(shippingLabel);
    layout->addLayout(shippingRow);

    // Divider
    auto *line = new QFrame(card);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("color: #E2E8F0;"));
    layout->addWidget(line);

    // Row: Total
    auto *totalRow = new QHBoxLayout();
    auto *totalTitle = new QLabel(QStringLiteral("Total Amount"), card);
    totalTitle->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: 800; color: #0F172A;"));
    totalLabel = new QLabel(QStringLiteral("₹0.00"), card);
    totalLabel->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 900; color: #4F46E5;"));
    totalRow->addWidget(totalTitle);
    totalRow->addStretch();
    totalRow->addWidget(totalLabel);
    layout->addLayout(totalRow);

    // Warning if unavailable
    unavailableWarningLabel = new QLabel(card);
    unavailableWarningLabel->setWordWrap(true);
    unavailableWarningLabel->setText(QStringLiteral("⚠️ Some items are no longer available. Please remove them before checkout."));
    unavailableWarningLabel->setStyleSheet(QStringLiteral("color: #DC2626; font-size: 12px; font-weight: 600; background: #FEE2E2; padding: 8px; border-radius: 6px;"));
    unavailableWarningLabel->setVisible(false);
    layout->addWidget(unavailableWarningLabel);

    layout->addSpacing(10);

    // Checkout Button
    checkoutButton = new QPushButton(QStringLiteral("Proceed to Checkout →"), card);
    checkoutButton->setCursor(Qt::PointingHandCursor);
    checkoutButton->setFixedHeight(48);
    checkoutButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #4F46E5; color: white; border-radius: 10px; font-size: 15px; font-weight: 800; border: none; }"
        "QPushButton:hover { background-color: #4338CA; }"
        "QPushButton:disabled { background-color: #CBD5E1; color: #94A3B8; }"
    ));
    connect(checkoutButton, &QPushButton::clicked, this, &CartWindow::handleCheckout);
    layout->addWidget(checkoutButton);

    // Clear Cart Button
    clearCartButton = new QPushButton(QStringLiteral("Clear Cart"), card);
    clearCartButton->setCursor(Qt::PointingHandCursor);
    clearCartButton->setFixedHeight(38);
    clearCartButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: #64748B; border: 1px solid #E2E8F0; border-radius: 8px; font-size: 13px; font-weight: 600; }"
        "QPushButton:hover { background: #FEE2E2; color: #DC2626; border-color: #FCA5A5; }"
    ));
    connect(clearCartButton, &QPushButton::clicked, this, &CartWindow::handleClearCart);
    layout->addWidget(clearCartButton);

    layout->addStretch();
    return card;
}

QPixmap CartWindow::loadOrGenerateCover(const QString &imagePath, const QString &title, const QString &author, int w, int h)
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

    // Procedural gradient cover
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
    f.setPointSize(12);
    painter.setFont(f);

    QRect titleRect(6, 12, w - 12, h / 2);
    painter.drawText(titleRect, Qt::AlignCenter | Qt::TextWordWrap, title.isEmpty() ? QStringLiteral("Book") : title);

    f.setBold(false);
    f.setPointSize(9);
    painter.setFont(f);
    QRect authorRect(6, h / 2 + 4, w - 12, h / 3);
    painter.drawText(authorRect, Qt::AlignCenter | Qt::TextWordWrap, author.isEmpty() ? QStringLiteral("Unknown") : author);

    painter.end();
    return pixmap;
}

QFrame* CartWindow::createItemCard(const CartItemModel &item)
{
    auto *card = new QFrame();
    card->setObjectName(QStringLiteral("cartItemCard"));

    QString cardStyle = item.isAvailable
        ? QStringLiteral("QFrame#cartItemCard { background-color: white; border: 1px solid #E2E8F0; border-radius: 12px; }")
        : QStringLiteral("QFrame#cartItemCard { background-color: #FFF1F2; border: 1px solid #FECDD3; border-radius: 12px; }");
    card->setStyleSheet(cardStyle);

    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(16);

    // Book Cover thumbnail
    auto *coverLabel = new QLabel(card);
    coverLabel->setFixedSize(65, 88);
    coverLabel->setPixmap(loadOrGenerateCover(item.coverImage, item.title, item.author, 65, 88));
    coverLabel->setStyleSheet(QStringLiteral("border-radius: 6px;"));
    layout->addWidget(coverLabel);

    // Center Details
    auto *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4);

    auto *titleLabel = new QLabel(item.title.isEmpty() ? QStringLiteral("Untitled Book") : item.title, card);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 700; color: #0F172A;"));
    infoLayout->addWidget(titleLabel);

    auto *authorLabel = new QLabel(QStringLiteral("by ") + (item.author.isEmpty() ? QStringLiteral("Unknown Author") : item.author), card);
    authorLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #64748B;"));
    infoLayout->addWidget(authorLabel);

    // Tag pills (Condition & Category)
    auto *tagsLayout = new QHBoxLayout();
    tagsLayout->setSpacing(6);

    if (!item.condition.isEmpty()) {
        auto *condLabel = new QLabel(item.condition, card);
        condLabel->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 600; color: #374151; background: #F3F4F6; padding: 2px 6px; border-radius: 4px;"));
        tagsLayout->addWidget(condLabel);
    }
    if (!item.category.isEmpty()) {
        auto *catLabel = new QLabel(item.category, card);
        catLabel->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 600; color: #4338CA; background: #EEF2FF; padding: 2px 6px; border-radius: 4px;"));
        tagsLayout->addWidget(catLabel);
    }

    if (!item.isAvailable) {
        auto *staleLabel = new QLabel(QStringLiteral("⚠️ No longer available"), card);
        staleLabel->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 700; color: #B91C1C; background: #FEE2E2; padding: 2px 8px; border-radius: 4px;"));
        tagsLayout->addWidget(staleLabel);
    }

    tagsLayout->addStretch();
    infoLayout->addLayout(tagsLayout);

    layout->addLayout(infoLayout, 1);

    // Right side: Price & Remove button
    auto *actionLayout = new QVBoxLayout();
    actionLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    actionLayout->setSpacing(8);

    auto *priceLabel = new QLabel(QString("₹%1").arg(item.price, 0, 'f', 0), card);
    priceLabel->setStyleSheet(QStringLiteral("font-size: 18px; font-weight: 800; color: #0F172A;"));
    priceLabel->setAlignment(Qt::AlignRight);
    actionLayout->addWidget(priceLabel);

    auto *removeBtn = new QPushButton(QStringLiteral("Remove ✕"), card);
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: #EF4444; border: none; font-size: 12px; font-weight: 600; padding: 4px 6px; }"
        "QPushButton:hover { color: #B91C1C; text-decoration: underline; }"
    ));
    connect(removeBtn, &QPushButton::clicked, this, [this, bId = item.bookId]() {
        handleRemoveItem(bId);
    });
    actionLayout->addWidget(removeBtn);

    layout->addLayout(actionLayout);

    return card;
}

void CartWindow::refreshCart()
{
    auto *reply = ApiClient::instance().get(QStringLiteral("/api/cart"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!ok || !json.contains(QStringLiteral("cart"))) {
            return;
        }

        cart = CartModel::fromJson(json[QStringLiteral("cart")].toObject());

        // Clear existing items in itemsLayout
        QLayoutItem *child;
        while ((child = itemsLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->deleteLater();
            }
            delete child;
        }

        for (const auto &item : cart.items) {
            itemsLayout->addWidget(createItemCard(item));
        }
        itemsLayout->addStretch();

        // Update Summary
        itemCountLabel->setText(QString("Subtotal (%1 item%2)").arg(cart.items.size()).arg(cart.items.size() == 1 ? "" : "s"));
        subtotalLabel->setText(QString("₹%1").arg(cart.subtotal, 0, 'f', 2));
        totalLabel->setText(QString("₹%1").arg(cart.total, 0, 'f', 2));

        unavailableWarningLabel->setVisible(cart.hasUnavailableItems);
        checkoutButton->setEnabled(!cart.items.isEmpty() && !cart.hasUnavailableItems);
        clearCartButton->setEnabled(!cart.items.isEmpty());

        updateEmptyState();
        emit cartCountChanged(cart.items.size());
    });
}

void CartWindow::updateEmptyState()
{
    const bool isEmpty = cart.items.isEmpty();
    scrollArea->setVisible(!isEmpty);
    emptyStateWidget->setVisible(isEmpty);
}

void CartWindow::handleRemoveItem(const QString &bookId)
{
    auto *reply = ApiClient::instance().del(QStringLiteral("/api/cart/items/") + bookId);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            refreshCart();
        } else {
            QMessageBox::warning(this, QStringLiteral("Error"), errorMsg.isEmpty() ? QStringLiteral("Failed to remove item.") : errorMsg);
        }
    });
}

void CartWindow::handleClearCart()
{
    auto res = QMessageBox::question(this, QStringLiteral("Clear Cart"),
                                     QStringLiteral("Are you sure you want to remove all items from your cart?"),
                                     QMessageBox::Yes | QMessageBox::No);
    if (res != QMessageBox::Yes) {
        return;
    }

    auto *reply = ApiClient::instance().del(QStringLiteral("/api/cart"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            refreshCart();
        } else {
            QMessageBox::warning(this, QStringLiteral("Error"), errorMsg.isEmpty() ? QStringLiteral("Failed to clear cart.") : errorMsg);
        }
    });
}

void CartWindow::handleCheckout()
{
    if (cart.items.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Empty Cart"), QStringLiteral("Your cart is empty."));
        return;
    }

    if (cart.hasUnavailableItems) {
        QMessageBox::warning(this, QStringLiteral("Unavailable Items"), QStringLiteral("Please remove unavailable items before proceeding to checkout."));
        return;
    }

    emit checkoutRequested();
}
