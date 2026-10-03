#include "windows/CartWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QPainter>
#include "StyledMessageBox.h"
#include <QFileInfo>
#include <QNetworkReply>

CartWindow::CartWindow(QWidget *parent)
    : QWidget(parent),
    itemsLayout(nullptr),
    itemsContainer(nullptr),
    emptyStateWidget(nullptr),
    scrollArea(nullptr),
    itemCountLabel(nullptr),
    subtotalLabel(nullptr),
    shippingLabel(nullptr),
    totalLabel(nullptr),
    unavailableWarningLabel(nullptr),
    checkoutButton(nullptr),
    clearCartButton(nullptr)
{
    setupUI();
    refreshCart();
}

void CartWindow::setupUI()
{
    setWindowTitle("BookBazzar - Shopping Cart");
    resize(1180, 780);
    setMinimumSize(920, 600);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(createTopBar());

    auto *bodyWidget = new QWidget(this);
    auto *bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(36, 24, 36, 36);
    bodyLayout->setSpacing(32);

    // Left Column: Items List & Empty State
    auto *leftCol = new QWidget(bodyWidget);
    auto *leftLayout = new QVBoxLayout(leftCol);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);

    scrollArea = new QScrollArea(leftCol);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QString(R"(
        QScrollArea {
            background-color: %1;
            border: none;
        }
        %2
    )").arg(AppStyle::Background, AppStyle::scrollBarStyle()));

    itemsContainer = new QWidget();
    itemsContainer->setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));
    itemsLayout = new QVBoxLayout(itemsContainer);
    itemsLayout->setContentsMargins(0, 0, 8, 0);
    itemsLayout->setSpacing(14);
    itemsLayout->addStretch();

    scrollArea->setWidget(itemsContainer);
    leftLayout->addWidget(scrollArea);

    // Empty state widget
    emptyStateWidget = new QWidget(leftCol);
    auto *emptyLayout = new QVBoxLayout(emptyStateWidget);
    emptyLayout->setContentsMargins(40, 80, 40, 80);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(16);

    auto *emptyIcon = new QLabel("🛒", emptyStateWidget);
    emptyIcon->setStyleSheet("font-size: 56px;");
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyIcon);

    auto *emptyTitle = new QLabel("Your Cart is Empty", emptyStateWidget);
    emptyTitle->setStyleSheet(QString("font-size: 22px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    emptyTitle->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyTitle);

    auto *emptySub = new QLabel("Explore our textbook & novel catalog to find books from other readers.", emptyStateWidget);
    emptySub->setStyleSheet(QString("font-size: 13px; color: %1;").arg(AppStyle::TextSecondary));
    emptySub->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptySub);

    auto *exploreBtn = new QPushButton("Browse Books Catalog", emptyStateWidget);
    exploreBtn->setCursor(Qt::PointingHandCursor);
    exploreBtn->setMinimumSize(180, 42);
    exploreBtn->setStyleSheet(AppStyle::primaryButtonStyle());
    connect(exploreBtn, &QPushButton::clicked, this, &CartWindow::browseRequested);
    emptyLayout->addWidget(exploreBtn, 0, Qt::AlignCenter);

    emptyStateWidget->setVisible(false);
    leftLayout->addWidget(emptyStateWidget);

    bodyLayout->addWidget(leftCol, 2);
    bodyLayout->addWidget(createSummaryCard(), 1);

    mainLayout->addWidget(bodyWidget, 1);
}

QWidget* CartWindow::createTopBar()
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
    backBtn->setMinimumHeight(38);
    backBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(backBtn, &QPushButton::clicked, this, &CartWindow::backRequested);
    layout->addWidget(backBtn);

    auto *title = new QLabel("My Shopping Cart", topBar);
    title->setStyleSheet(QString("font-size: 20px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    layout->addWidget(title);

    layout->addStretch();

    auto *refreshBtn = new QPushButton("↻ Refresh Cart", topBar);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setMinimumHeight(38);
    refreshBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(refreshBtn, &QPushButton::clicked, this, &CartWindow::refreshCart);
    layout->addWidget(refreshBtn);

    return topBar;
}

QWidget* CartWindow::createSummaryCard()
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
    itemCountLabel = new QLabel("Subtotal (0 items)", card);
    itemCountLabel->setStyleSheet(QString("font-size: 13px; color: %1;").arg(AppStyle::TextSecondary));
    subtotalLabel = new QLabel("₹0.00", card);
    subtotalLabel->setStyleSheet(QString("font-size: 15px; font-weight: 700; color: %1;").arg(AppStyle::TextPrimary));
    subtotalRow->addWidget(itemCountLabel);
    subtotalRow->addStretch();
    subtotalRow->addWidget(subtotalLabel);
    layout->addLayout(subtotalRow);

    auto *shippingRow = new QHBoxLayout();
    auto *shippingTitle = new QLabel("Estimated Delivery", card);
    shippingTitle->setStyleSheet(QString("font-size: 13px; color: %1;").arg(AppStyle::TextSecondary));
    shippingLabel = new QLabel("FREE", card);
    shippingLabel->setStyleSheet(AppStyle::badgeStyle(AppStyle::SuccessLight, AppStyle::SuccessText, AppStyle::SuccessBorder));
    shippingRow->addWidget(shippingTitle);
    shippingRow->addStretch();
    shippingRow->addWidget(shippingLabel);
    layout->addLayout(shippingRow);

    auto *line = new QFrame(card);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QString("color: %1;").arg(AppStyle::BorderSubtle));
    layout->addWidget(line);

    auto *totalRow = new QHBoxLayout();
    auto *totalTitle = new QLabel("Total Amount", card);
    totalTitle->setStyleSheet(QString("font-size: 16px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    totalLabel = new QLabel("₹0.00", card);
    totalLabel->setStyleSheet(QString("font-size: 24px; font-weight: 900; color: %1;").arg(AppStyle::Primary));
    totalRow->addWidget(totalTitle);
    totalRow->addStretch();
    totalRow->addWidget(totalLabel);
    layout->addLayout(totalRow);

    unavailableWarningLabel = new QLabel(card);
    unavailableWarningLabel->setWordWrap(true);
    unavailableWarningLabel->setText("⚠️ Some items in your cart are no longer available. Please remove them before checkout.");
    unavailableWarningLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600; background-color: %2; border: 1px solid %3; padding: 10px; border-radius: 8px;").arg(AppStyle::DangerText, AppStyle::DangerLight, AppStyle::DangerBorder));
    unavailableWarningLabel->setVisible(false);
    layout->addWidget(unavailableWarningLabel);

    layout->addSpacing(8);

    checkoutButton = new QPushButton("Proceed to Checkout →", card);
    checkoutButton->setCursor(Qt::PointingHandCursor);
    checkoutButton->setMinimumHeight(46);
    checkoutButton->setStyleSheet(AppStyle::primaryButtonStyle());
    connect(checkoutButton, &QPushButton::clicked, this, &CartWindow::handleCheckout);
    layout->addWidget(checkoutButton);

    clearCartButton = new QPushButton("Clear Cart", card);
    clearCartButton->setCursor(Qt::PointingHandCursor);
    clearCartButton->setMinimumHeight(38);
    clearCartButton->setStyleSheet(AppStyle::secondaryButtonStyle());
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

    QPixmap pixmap(w, h);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient grad(0, 0, w, h);
    grad.setColorAt(0.0, QColor("#4F46E5"));
    grad.setColorAt(1.0, QColor("#312E81"));
    painter.fillRect(0, 0, w, h, grad);

    painter.setPen(Qt::white);
    QFont f(AppStyle::appFont(), 10, QFont::Bold);
    painter.setFont(f);

    QRect titleRect(4, 8, w - 8, h / 2);
    painter.drawText(titleRect, Qt::AlignCenter | Qt::TextWordWrap, title.isEmpty() ? "Book" : title);

    f.setBold(false);
    f.setPointSize(8);
    painter.setFont(f);
    QRect authorRect(4, h / 2 + 4, w - 8, h / 3);
    painter.drawText(authorRect, Qt::AlignCenter | Qt::TextWordWrap, author.isEmpty() ? "Unknown" : author);

    painter.end();
    return pixmap;
}

QFrame* CartWindow::createItemCard(const CartItemModel &item)
{
    auto *card = new QFrame();
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 14, 3, 10);

    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(16);

    auto *coverLabel = new QLabel(card);
    coverLabel->setFixedSize(65, 88);
    coverLabel->setPixmap(loadOrGenerateCover(item.coverImage, item.title, item.author, 65, 88));
    coverLabel->setStyleSheet("border-radius: 6px;");
    layout->addWidget(coverLabel);

    auto *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4);

    auto *titleLabel = new QLabel(item.title.isEmpty() ? "Untitled Book" : item.title, card);
    titleLabel->setStyleSheet(QString("font-size: 15px; font-weight: 700; color: %1;").arg(AppStyle::TextPrimary));
    infoLayout->addWidget(titleLabel);

    auto *authorLabel = new QLabel("by " + (item.author.isEmpty() ? "Unknown Author" : item.author), card);
    authorLabel->setStyleSheet(QString("font-size: 12px; color: %1;").arg(AppStyle::TextSecondary));
    infoLayout->addWidget(authorLabel);

    auto *tagsLayout = new QHBoxLayout();
    tagsLayout->setSpacing(8);

    if (!item.condition.isEmpty()) {
        auto *condLabel = new QLabel(item.condition, card);
        condLabel->setStyleSheet(AppStyle::statusBadgeStyle(item.condition));
        tagsLayout->addWidget(condLabel);
    }
    if (!item.category.isEmpty()) {
        auto *catLabel = new QLabel(item.category, card);
        catLabel->setStyleSheet(AppStyle::badgeStyle(AppStyle::PrimaryLight, AppStyle::Primary, AppStyle::PrimaryBorder));
        tagsLayout->addWidget(catLabel);
    }

    if (!item.isAvailable) {
        auto *staleLabel = new QLabel("⚠️ No longer available", card);
        staleLabel->setStyleSheet(AppStyle::badgeStyle(AppStyle::DangerLight, AppStyle::DangerText, AppStyle::DangerBorder));
        tagsLayout->addWidget(staleLabel);
    }

    tagsLayout->addStretch();
    infoLayout->addLayout(tagsLayout);

    layout->addLayout(infoLayout, 1);

    auto *actionLayout = new QVBoxLayout();
    actionLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    actionLayout->setSpacing(8);

    auto *priceLabel = new QLabel(QString("₹%1").arg(item.price, 0, 'f', 0), card);
    priceLabel->setStyleSheet(QString("font-size: 18px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    priceLabel->setAlignment(Qt::AlignRight);
    actionLayout->addWidget(priceLabel);

    auto *removeBtn = new QPushButton("Remove ✕", card);
    removeBtn->setCursor(Qt::PointingHandCursor);
    removeBtn->setStyleSheet(QString("QPushButton { background: transparent; color: %1; border: none; font-size: 12px; font-weight: 600; padding: 4px 6px; } QPushButton:hover { color: %2; }").arg(AppStyle::Danger, AppStyle::DangerHover));
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
            StyledMessageBox::warning(this, "Error", errorMsg.isEmpty() ? "Failed to remove item." : errorMsg);
        }
    });
}

void CartWindow::handleClearCart()
{
    bool confirmed = StyledMessageBox::question(
        this, "Clear Cart",
        "Are you sure you want to remove all items from your cart?",
        "Yes, Clear", "Keep Items"
    );
    if (!confirmed) {
        return;
    }

    auto *reply = ApiClient::instance().del(QStringLiteral("/api/cart"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            refreshCart();
        } else {
            StyledMessageBox::warning(this, "Error", errorMsg.isEmpty() ? "Failed to clear cart." : errorMsg);
        }
    });
}

void CartWindow::handleCheckout()
{
    if (cart.items.isEmpty()) {
        StyledMessageBox::information(this, "Empty Cart", "Your cart is empty.");
        return;
    }

    if (cart.hasUnavailableItems) {
        StyledMessageBox::warning(this, "Unavailable Items", "Please remove unavailable items before proceeding to checkout.");
        return;
    }

    emit checkoutRequested();
}
