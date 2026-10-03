#include "orderswindow.h"
#include "windows/OrderDetailWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QFileInfo>
#include <QPixmap>
#include <QPainter>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QEvent>
#include <QMouseEvent>
#include <QJsonArray>

OrdersWindow::OrdersWindow(const QString &userName, QWidget *parent)
    : QWidget(parent),
    userName(userName),
    totalOrdersLabel(nullptr),
    totalSpentLabel(nullptr),
    statsTitleOrders(nullptr),
    statsTitleSpent(nullptr),
    purchasesTabBtn(nullptr),
    salesTabBtn(nullptr),
    ordersLayout(nullptr),
    emptyStateWidget(nullptr),
    prevPageBtn(nullptr),
    nextPageBtn(nullptr),
    pageIndicatorLabel(nullptr)
{
    setWindowTitle("BookBazzar - Orders & Sales");
    resize(1240, 820);
    setMinimumSize(980, 640);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    setupUI();
    refreshOrders();
}

void OrdersWindow::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(createTopBar());

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QString(R"(
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

    contentLayout->addWidget(createTabBar());
    contentLayout->addWidget(createStatsBar());

    ordersLayout = new QVBoxLayout();
    ordersLayout->setContentsMargins(0, 0, 0, 0);
    ordersLayout->setSpacing(14);
    contentLayout->addLayout(ordersLayout);

    emptyStateWidget = new QWidget();
    auto *emptyL = new QVBoxLayout(emptyStateWidget);
    emptyL->setContentsMargins(20, 60, 20, 60);
    emptyL->setAlignment(Qt::AlignCenter);
    auto *emptyLbl = new QLabel("No orders found.", emptyStateWidget);
    emptyLbl->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 600;").arg(AppStyle::TextSecondary));
    emptyL->addWidget(emptyLbl);
    emptyStateWidget->setVisible(false);
    contentLayout->addWidget(emptyStateWidget);

    contentLayout->addWidget(createPaginationBar());
    contentLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

QWidget* OrdersWindow::createTopBar()
{
    auto *bar = new QFrame();
    bar->setFixedHeight(72);
    bar->setStyleSheet(R"(
        .QFrame {
            background-color: #FFFFFF;
            border-bottom: 1px solid #E2E8F0;
        }
        QLabel {
            border: none;
            background: transparent;
        }
    )");
    AppStyle::applyElevation(bar, 16, 2, 15);

    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(36, 10, 36, 10);
    layout->setSpacing(16);

    auto *backButton = new QPushButton("← Back", bar);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setFixedSize(88, 38);
    backButton->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(backButton, &QPushButton::clicked, this, &OrdersWindow::handleBack);
    layout->addWidget(backButton);

    auto *title = new QLabel("Orders & Transactions", bar);
    title->setStyleSheet(QString("font-size: 20px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    layout->addWidget(title);

    layout->addStretch();

    auto *browseBtn = new QPushButton("Browse Books", bar);
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setFixedHeight(38);
    browseBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(browseBtn, &QPushButton::clicked, this, &OrdersWindow::handleBrowse);
    layout->addWidget(browseBtn);

    return bar;
}

QWidget* OrdersWindow::createTabBar()
{
    auto *container = new QWidget();
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    purchasesTabBtn = new QPushButton("My Purchases (Orders Placed)", container);
    purchasesTabBtn->setCursor(Qt::PointingHandCursor);
    purchasesTabBtn->setStyleSheet(AppStyle::activeTabStyle());

    salesTabBtn = new QPushButton("Sales Orders (Received)", container);
    salesTabBtn->setCursor(Qt::PointingHandCursor);
    salesTabBtn->setStyleSheet(AppStyle::inactiveTabStyle());

    layout->addWidget(purchasesTabBtn);
    layout->addWidget(salesTabBtn);
    layout->addStretch();

    connect(purchasesTabBtn, &QPushButton::clicked, this, [this]() {
        handleTabChange(0);
    });
    connect(salesTabBtn, &QPushButton::clicked, this, [this]() {
        handleTabChange(1);
    });

    return container;
}

void OrdersWindow::handleTabChange(int tabIndex)
{
    if (currentTab == tabIndex) return;
    currentTab = tabIndex;
    currentPage = 1;

    if (currentTab == 0) {
        purchasesTabBtn->setStyleSheet(AppStyle::activeTabStyle());
        salesTabBtn->setStyleSheet(AppStyle::inactiveTabStyle());
        statsTitleOrders->setText("Total Purchases");
        statsTitleSpent->setText("Total Spent");
    } else {
        purchasesTabBtn->setStyleSheet(AppStyle::inactiveTabStyle());
        salesTabBtn->setStyleSheet(AppStyle::activeTabStyle());
        statsTitleOrders->setText("Total Sales Received");
        statsTitleSpent->setText("Total Earnings");
    }

    refreshOrders();
}

QWidget* OrdersWindow::createStatsBar()
{
    auto *container = new QWidget();
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);

    auto createStatCard = [](const QString &initialTitle, QLabel* &titleRef, QLabel* &labelRef, const QString &color) {
        auto *card = new QFrame();
        card->setStyleSheet(AppStyle::statCardStyle());
        AppStyle::applyElevation(card, 14, 3, 10);

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 16, 20, 16);
        cardLayout->setSpacing(4);

        titleRef = new QLabel(initialTitle);
        titleRef->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600;").arg(AppStyle::TextSecondary));
        cardLayout->addWidget(titleRef);

        labelRef = new QLabel("0");
        labelRef->setStyleSheet(QString("color: %1; font-size: 26px; font-weight: 800;").arg(color));
        cardLayout->addWidget(labelRef);

        return card;
    };

    layout->addWidget(createStatCard("Total Purchases", statsTitleOrders, totalOrdersLabel, AppStyle::TextPrimary));
    layout->addWidget(createStatCard("Total Spent", statsTitleSpent, totalSpentLabel, AppStyle::Success));

    return container;
}

QWidget* OrdersWindow::createPaginationBar()
{
    auto *container = new QFrame();
    container->setStyleSheet("background: transparent;");

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 10, 0, 10);
    layout->setSpacing(12);

    prevPageBtn = new QPushButton("← Previous", container);
    prevPageBtn->setCursor(Qt::PointingHandCursor);
    prevPageBtn->setFixedSize(100, 36);
    prevPageBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(prevPageBtn, &QPushButton::clicked, this, &OrdersWindow::handlePrevPage);
    layout->addWidget(prevPageBtn);

    pageIndicatorLabel = new QLabel("Page 1 of 1", container);
    pageIndicatorLabel->setStyleSheet(QString("font-weight: 600; color: %1; font-size: 13px;").arg(AppStyle::TextSecondary));
    layout->addWidget(pageIndicatorLabel);

    nextPageBtn = new QPushButton("Next →", container);
    nextPageBtn->setCursor(Qt::PointingHandCursor);
    nextPageBtn->setFixedSize(100, 36);
    nextPageBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(nextPageBtn, &QPushButton::clicked, this, &OrdersWindow::handleNextPage);
    layout->addWidget(nextPageBtn);

    layout->addStretch();
    return container;
}

QPixmap OrdersWindow::loadOrGenerateCover(const QString &imagePath, const QString &title, const QString &author, int w, int h)
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

    painter.fillRect(0, 0, w, h, QColor("#1E293B"));
    painter.setPen(Qt::white);
    QFont f(AppStyle::appFont(), 8, QFont::Bold);
    painter.setFont(f);
    QRect titleRect(3, 6, w - 6, h / 2);
    painter.drawText(titleRect, Qt::AlignCenter | Qt::TextWordWrap, title.isEmpty() ? "Book" : title);
    painter.end();
    return pixmap;
}

QFrame* OrdersWindow::createOrderCard(const OrderModel &order)
{
    auto *card = new QFrame();
    card->setObjectName("orderCard");
    card->setCursor(Qt::PointingHandCursor);
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 14, 3, 10);

    auto *mainL = new QVBoxLayout(card);
    mainL->setContentsMargins(20, 16, 20, 16);
    mainL->setSpacing(12);

    auto *topRow = new QHBoxLayout();
    auto *numLbl = new QLabel(QString("Order #%1").arg(order.orderNumber), card);
    numLbl->setStyleSheet(QString("font-size: 15px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    topRow->addWidget(numLbl);

    auto *dateLbl = new QLabel(order.createdAt.toString("dd MMM yyyy, hh:mm AP"), card);
    dateLbl->setStyleSheet(QString("font-size: 12px; color: %1; margin-left: 8px;").arg(AppStyle::TextMuted));
    topRow->addWidget(dateLbl);

    topRow->addStretch();

    auto *badge = new QLabel(order.status, card);
    badge->setAlignment(Qt::AlignCenter);
    badge->setStyleSheet(AppStyle::statusBadgeStyle(order.status));
    topRow->addWidget(badge);
    mainL->addLayout(topRow);

    auto *itemsRow = new QHBoxLayout();
    itemsRow->setSpacing(12);

    int maxPreviews = 3;
    for (int i = 0; i < qMin(maxPreviews, static_cast<int>(order.items.size())); ++i) {
        const auto &it = order.items[i];
        auto *thumb = new QLabel(card);
        thumb->setFixedSize(40, 54);
        thumb->setPixmap(loadOrGenerateCover(it.coverImage, it.title, it.author, 40, 54));
        thumb->setStyleSheet("border-radius: 4px;");
        itemsRow->addWidget(thumb);
    }

    auto *descL = new QVBoxLayout();
    descL->setSpacing(2);
    QString summaryText = order.items.isEmpty() ? "1 item" : order.items[0].title;
    if (order.items.size() > 1) {
        summaryText += QString(" and %1 other item%2").arg(order.items.size() - 1).arg(order.items.size() - 1 == 1 ? "" : "s");
    }
    auto *sumLabel = new QLabel(summaryText, card);
    sumLabel->setStyleSheet(QString("font-size: 13px; font-weight: 700; color: %1;").arg(AppStyle::TextPrimary));
    descL->addWidget(sumLabel);

    auto *subDetail = new QLabel(QString("%1 • Payment: %2").arg(order.shippingAddress.city.isEmpty() ? "Standard Delivery" : order.shippingAddress.city).arg(order.paymentMethod), card);
    subDetail->setStyleSheet(QString("font-size: 12px; color: %1;").arg(AppStyle::TextSecondary));
    descL->addWidget(subDetail);

    itemsRow->addLayout(descL, 1);

    auto *priceCol = new QVBoxLayout();
    priceCol->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto *pLabel = new QLabel(QString("₹%1").arg(order.total, 0, 'f', 2), card);
    pLabel->setStyleSheet(QString("font-size: 18px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    pLabel->setAlignment(Qt::AlignRight);
    priceCol->addWidget(pLabel);

    auto *viewBtn = new QPushButton("View Details →", card);
    viewBtn->setCursor(Qt::PointingHandCursor);
    viewBtn->setStyleSheet(AppStyle::ghostButtonStyle());
    connect(viewBtn, &QPushButton::clicked, this, [this, oId = order.id]() {
        showOrderDetail(oId);
    });
    priceCol->addWidget(viewBtn);

    itemsRow->addLayout(priceCol);
    mainL->addLayout(itemsRow);

    card->setProperty("orderId", order.id);
    card->installEventFilter(this);

    return card;
}

bool OrdersWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            auto *card = qobject_cast<QFrame*>(watched);
            if (card && card->property("orderId").isValid()) {
                showOrderDetail(card->property("orderId").toString());
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void OrdersWindow::refreshOrders()
{
    QString endpoint = (currentTab == 0) ? QStringLiteral("/api/orders") : QStringLiteral("/api/orders/seller/history");

    QUrlQuery query;
    query.addQueryItem("page", QString::number(currentPage));
    query.addQueryItem("limit", "10");

    auto *reply = ApiClient::instance().get(endpoint, query);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        QLayoutItem *child;
        while ((child = ordersLayout->takeAt(0)) != nullptr) {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }

        if (!ok || !json.contains(QStringLiteral("orders"))) {
            emptyStateWidget->setVisible(true);
            return;
        }

        auto arr = json[QStringLiteral("orders")].toArray();
        QVector<OrderModel> orders;
        double sumAmount = 0.0;

        for (const auto &v : arr) {
            auto ord = OrderModel::fromJson(v.toObject());
            orders.append(ord);
            sumAmount += ord.total;
        }

        if (json.contains(QStringLiteral("pagination"))) {
            pagination = PaginationMeta::fromJson(json[QStringLiteral("pagination")].toObject());
            pageIndicatorLabel->setText(QString("Page %1 of %2 (%3 orders)").arg(pagination.page).arg(qMax(1, pagination.totalPages)).arg(pagination.total));
            prevPageBtn->setEnabled(pagination.hasPrevPage);
            nextPageBtn->setEnabled(pagination.hasNextPage);
            totalOrdersLabel->setText(QString::number(pagination.total));
        } else {
            totalOrdersLabel->setText(QString::number(orders.size()));
        }

        totalSpentLabel->setText(QString("₹%1").arg(sumAmount, 0, 'f', 2));

        if (orders.isEmpty()) {
            emptyStateWidget->setVisible(true);
        } else {
            emptyStateWidget->setVisible(false);
            for (const auto &ord : orders) {
                ordersLayout->addWidget(createOrderCard(ord));
            }
        }
    });
}

void OrdersWindow::showOrderDetail(const QString &orderId)
{
    if (receivers(SIGNAL(orderSelected(QString))) > 0) {
        emit orderSelected(orderId);
        return;
    }

    auto *detailWin = new OrderDetailWindow(orderId);
    detailWin->setAttribute(Qt::WA_DeleteOnClose);
    connect(detailWin, &OrderDetailWindow::backRequested, this, [this, detailWin]() {
        detailWin->close();
        this->show();
        this->raise();
        this->activateWindow();
    });
    connect(detailWin, &OrderDetailWindow::orderCancelled, this, &OrdersWindow::refreshOrders);
    connect(detailWin, &OrderDetailWindow::orderStatusUpdated, this, &OrdersWindow::refreshOrders);
    detailWin->show();
    detailWin->raise();
    detailWin->activateWindow();
}

void OrdersWindow::handleNextPage()
{
    if (pagination.hasNextPage) {
        currentPage++;
        refreshOrders();
    }
}

void OrdersWindow::handlePrevPage()
{
    if (pagination.hasPrevPage && currentPage > 1) {
        currentPage--;
        refreshOrders();
    }
}

void OrdersWindow::handleBack()
{
    emit backRequested();
}

void OrdersWindow::handleBrowse()
{
    emit browseRequested();
}
