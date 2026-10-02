#include "orderswindow.h"
#include "windows/OrderDetailWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"

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
    setWindowTitle(QStringLiteral("BookBazzar - Orders & Sales"));
    resize(1200, 800);
    setMinimumSize(950, 600);

    setupUI();
    refreshOrders();
}

void OrdersWindow::setupUI()
{
    setStyleSheet(R"(
        QWidget {
            font-family: 'Segoe UI', Arial, sans-serif;
            background-color: #F8FAFC;
        }
        QScrollArea {
            border: none;
            background-color: #F8FAFC;
        }
    )");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(createTopBar());

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *content = new QWidget();
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 24, 40, 40);
    contentLayout->setSpacing(20);

    contentLayout->addWidget(createTabBar());
    contentLayout->addWidget(createStatsBar());

    ordersLayout = new QVBoxLayout();
    ordersLayout->setContentsMargins(0, 0, 0, 0);
    ordersLayout->setSpacing(14);
    contentLayout->addLayout(ordersLayout);

    emptyStateWidget = new QWidget();
    auto *emptyL = new QVBoxLayout(emptyStateWidget);
    emptyL->setContentsMargins(20, 40, 20, 40);
    emptyL->setAlignment(Qt::AlignCenter);
    auto *emptyLbl = new QLabel(QStringLiteral("No orders found."), emptyStateWidget);
    emptyLbl->setStyleSheet(QStringLiteral("color: #64748B; font-size: 15px; font-weight: 600;"));
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
    bar->setFixedHeight(74);
    bar->setStyleSheet(QStringLiteral("QFrame { background-color: white; border-bottom: 1px solid #E2E8F0; }"));

    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(36, 12, 36, 12);
    layout->setSpacing(16);

    auto *backButton = new QPushButton(QStringLiteral("← Back"), bar);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setFixedSize(90, 40);
    backButton->setStyleSheet(R"(
        QPushButton {
            background-color: #F1F5F9;
            color: #334155;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 700;
        }
        QPushButton:hover {
            background-color: #E2E8F0;
            color: #0F172A;
        }
    )");
    connect(backButton, &QPushButton::clicked, this, &OrdersWindow::handleBack);
    layout->addWidget(backButton);

    auto *title = new QLabel(QStringLiteral("Order History & Management"), bar);
    title->setStyleSheet(QStringLiteral("QLabel { color: #0F172A; font-size: 22px; font-weight: 800; }"));
    layout->addWidget(title);

    layout->addStretch();

    auto *browseBtn = new QPushButton(QStringLiteral("Browse Books"), bar);
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setMinimumSize(140, 42);
    browseBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #4F46E5;
            color: white;
            border: none;
            border-radius: 9px;
            font-size: 13px;
            font-weight: 800;
            padding: 0 16px;
        }
        QPushButton:hover {
            background-color: #4338CA;
        }
    )");
    connect(browseBtn, &QPushButton::clicked, this, &OrdersWindow::handleBrowse);
    layout->addWidget(browseBtn);

    return bar;
}

QWidget* OrdersWindow::createTabBar()
{
    auto *container = new QFrame();
    container->setStyleSheet(QStringLiteral("background: transparent;"));

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    purchasesTabBtn = new QPushButton(QStringLiteral("My Purchases (Buyer)"), container);
    purchasesTabBtn->setCursor(Qt::PointingHandCursor);
    purchasesTabBtn->setFixedHeight(42);
    purchasesTabBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: #4F46E5; color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: 700; padding: 0 20px; }"
    ));
    connect(purchasesTabBtn, &QPushButton::clicked, this, [this]() { handleTabChange(0); });
    layout->addWidget(purchasesTabBtn);

    salesTabBtn = new QPushButton(QStringLiteral("My Sales (Seller)"), container);
    salesTabBtn->setCursor(Qt::PointingHandCursor);
    salesTabBtn->setFixedHeight(42);
    salesTabBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: white; color: #475569; border: 1px solid #CBD5E1; border-radius: 8px; font-size: 14px; font-weight: 700; padding: 0 20px; }"
        "QPushButton:hover { background: #F1F5F9; color: #0F172A; }"
    ));
    connect(salesTabBtn, &QPushButton::clicked, this, [this]() { handleTabChange(1); });
    layout->addWidget(salesTabBtn);

    layout->addStretch();
    return container;
}

QWidget* OrdersWindow::createStatsBar()
{
    auto *container = new QWidget();
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);

    auto createStatCard = [](const QString &initialTitle, QLabel* &titleRef, QLabel* &labelRef, const QString &color) {
        auto *card = new QFrame();
        card->setStyleSheet(QStringLiteral("QFrame { background-color: white; border: 1px solid #E2E8F0; border-radius: 12px; }"));
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 16, 20, 16);
        cardLayout->setSpacing(4);

        titleRef = new QLabel(initialTitle);
        titleRef->setStyleSheet(QStringLiteral("QLabel { color: #64748B; font-size: 12px; font-weight: 600; }"));
        cardLayout->addWidget(titleRef);

        labelRef = new QLabel(QStringLiteral("0"));
        labelRef->setStyleSheet(QString("QLabel { color: %1; font-size: 26px; font-weight: 800; }").arg(color));
        cardLayout->addWidget(labelRef);

        return card;
    };

    layout->addWidget(createStatCard(QStringLiteral("Total Purchases"), statsTitleOrders, totalOrdersLabel, QStringLiteral("#0F172A")));
    layout->addWidget(createStatCard(QStringLiteral("Total Spent"), statsTitleSpent, totalSpentLabel, QStringLiteral("#10B981")));

    return container;
}

QWidget* OrdersWindow::createPaginationBar()
{
    auto *container = new QFrame();
    container->setStyleSheet(QStringLiteral("background: transparent;"));

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 10, 0, 10);
    layout->setSpacing(12);

    prevPageBtn = new QPushButton(QStringLiteral("← Previous"), container);
    prevPageBtn->setCursor(Qt::PointingHandCursor);
    prevPageBtn->setFixedSize(100, 36);
    prevPageBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: white; border: 1px solid #CBD5E1; border-radius: 6px; font-weight: 600; color: #334155; }"
        "QPushButton:hover { background: #F1F5F9; }"
        "QPushButton:disabled { color: #94A3B8; border-color: #E2E8F0; }"
    ));
    connect(prevPageBtn, &QPushButton::clicked, this, &OrdersWindow::handlePrevPage);
    layout->addWidget(prevPageBtn);

    pageIndicatorLabel = new QLabel(QStringLiteral("Page 1 of 1"), container);
    pageIndicatorLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #475569; font-size: 13px;"));
    layout->addWidget(pageIndicatorLabel);

    nextPageBtn = new QPushButton(QStringLiteral("Next →"), container);
    nextPageBtn->setCursor(Qt::PointingHandCursor);
    nextPageBtn->setFixedSize(100, 36);
    nextPageBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: white; border: 1px solid #CBD5E1; border-radius: 6px; font-weight: 600; color: #334155; }"
        "QPushButton:hover { background: #F1F5F9; }"
        "QPushButton:disabled { color: #94A3B8; border-color: #E2E8F0; }"
    ));
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

    QLinearGradient grad(0, 0, w, h);
    grad.setColorAt(0.0, QColor(0x4F, 0x46, 0xE5));
    grad.setColorAt(1.0, QColor(0x7C, 0x3A, 0xED));
    painter.fillRect(0, 0, w, h, grad);

    painter.setPen(Qt::white);
    QFont f = painter.font();
    f.setBold(true);
    f.setPointSize(9);
    painter.setFont(f);
    QRect titleRect(3, 6, w - 6, h / 2);
    painter.drawText(titleRect, Qt::AlignCenter | Qt::TextWordWrap, title.isEmpty() ? QStringLiteral("Book") : title);
    painter.end();
    return pixmap;
}

QFrame* OrdersWindow::createOrderCard(const OrderModel &order)
{
    auto *card = new QFrame();
    card->setObjectName(QStringLiteral("orderCard"));
    card->setCursor(Qt::PointingHandCursor);
    card->setStyleSheet(R"(
        QFrame#orderCard {
            background-color: white;
            border: 1px solid #E2E8F0;
            border-radius: 14px;
        }
        QFrame#orderCard:hover {
            border-color: #4F46E5;
        }
    )");

    auto *mainL = new QVBoxLayout(card);
    mainL->setContentsMargins(20, 16, 20, 16);
    mainL->setSpacing(12);

    // Top Header: Order Number, Date, Status badge
    auto *topRow = new QHBoxLayout();
    auto *numLbl = new QLabel(QString("Order #%1").arg(order.orderNumber), card);
    numLbl->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 800; color: #0F172A;"));
    topRow->addWidget(numLbl);

    auto *dateLbl = new QLabel(order.createdAt.toString(QStringLiteral("dd MMM yyyy, hh:mm AP")), card);
    dateLbl->setStyleSheet(QStringLiteral("font-size: 12px; color: #64748B; margin-left: 8px;"));
    topRow->addWidget(dateLbl);

    topRow->addStretch();

    auto *badge = new QLabel(order.status, card);
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedHeight(28);

    if (order.status == QStringLiteral("DELIVERED")) {
        badge->setStyleSheet(QStringLiteral("background: #D1FAE5; color: #065F46; font-size: 11px; font-weight: 800; border-radius: 6px; padding: 0 10px;"));
    } else if (order.status == QStringLiteral("CANCELLED") || order.status == QStringLiteral("RETURN_REJECTED")) {
        badge->setStyleSheet(QStringLiteral("background: #FEE2E2; color: #991B1B; font-size: 11px; font-weight: 800; border-radius: 6px; padding: 0 10px;"));
    } else if (order.status == QStringLiteral("RETURN_REQUESTED") || order.status == QStringLiteral("RETURNED")) {
        badge->setStyleSheet(QStringLiteral("background: #FEF3C7; color: #92400E; font-size: 11px; font-weight: 800; border-radius: 6px; padding: 0 10px;"));
    } else {
        badge->setStyleSheet(QStringLiteral("background: #DBEAFE; color: #1E40AF; font-size: 11px; font-weight: 800; border-radius: 6px; padding: 0 10px;"));
    }
    topRow->addWidget(badge);
    mainL->addLayout(topRow);

    // Items summary row
    auto *itemsRow = new QHBoxLayout();
    itemsRow->setSpacing(12);

    int maxPreviews = 3;
    for (int i = 0; i < qMin(maxPreviews, static_cast<int>(order.items.size())); ++i) {
        const auto &it = order.items[i];
        auto *thumb = new QLabel(card);
        thumb->setFixedSize(40, 54);
        thumb->setPixmap(loadOrGenerateCover(it.coverImage, it.title, it.author, 40, 54));
        thumb->setStyleSheet(QStringLiteral("border-radius: 4px;"));
        itemsRow->addWidget(thumb);
    }

    auto *infoBox = new QVBoxLayout();
    infoBox->setSpacing(2);
    QString summaryText = order.items.isEmpty() ? QStringLiteral("No items") : order.items[0].title;
    if (order.items.size() > 1) {
        summaryText += QString(" + %1 more item%2").arg(order.items.size() - 1).arg(order.items.size() > 2 ? "s" : "");
    }
    auto *itemsSummary = new QLabel(summaryText, card);
    itemsSummary->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 600; color: #1E293B;"));
    infoBox->addWidget(itemsSummary);

    QString partyInfo = (currentTab == 0)
        ? (order.items.isEmpty() ? QString() : QStringLiteral("Seller: ") + order.items[0].sellerUsername)
        : (QStringLiteral("Buyer: ") + order.buyerUsername);
    auto *partyLabel = new QLabel(partyInfo, card);
    partyLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #64748B;"));
    infoBox->addWidget(partyLabel);

    itemsRow->addLayout(infoBox, 1);

    auto *priceCol = new QVBoxLayout();
    priceCol->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto *pLbl = new QLabel(QString("₹%1").arg(order.total, 0, 'f', 0), card);
    pLbl->setStyleSheet(QStringLiteral("font-size: 17px; font-weight: 800; color: #0F172A;"));
    priceCol->addWidget(pLbl);

    auto *viewLink = new QLabel(QStringLiteral("View Details →"), card);
    viewLink->setStyleSheet(QStringLiteral("font-size: 12px; font-weight: 700; color: #4F46E5;"));
    priceCol->addWidget(viewLink);
    itemsRow->addLayout(priceCol);

    mainL->addLayout(itemsRow);

    // Click card -> open details
    card->installEventFilter(this);
    card->setProperty("orderId", order.id);

    return card;
}

void OrdersWindow::refreshOrders()
{
    if (!ordersLayout) return;

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("page"), QString::number(currentPage));
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("10"));

    QString endpoint = (currentTab == 0) ? QStringLiteral("/api/orders") : QStringLiteral("/api/orders/seller/history");

    auto *reply = ApiClient::instance().get(endpoint, query);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        QLayoutItem *item;
        while ((item = ordersLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        if (!ok || !json.contains(QStringLiteral("orders"))) {
            emptyStateWidget->setVisible(true);
            return;
        }

        QJsonArray arr = json[QStringLiteral("orders")].toArray();
        QVector<OrderModel> orders;
        double totalSum = 0.0;

        for (const auto &val : arr) {
            OrderModel o = OrderModel::fromJson(val.toObject());
            orders.append(o);
            totalSum += o.total;
        }

        if (json.contains(QStringLiteral("pagination"))) {
            pagination = PaginationMeta::fromJson(json[QStringLiteral("pagination")].toObject());
            pageIndicatorLabel->setText(QString("Page %1 of %2 (Total %3)")
                                            .arg(pagination.page)
                                            .arg(qMax(1, pagination.totalPages))
                                            .arg(pagination.total));
            prevPageBtn->setEnabled(pagination.hasPrevPage);
            nextPageBtn->setEnabled(pagination.hasNextPage);
        }

        if (totalOrdersLabel) {
            totalOrdersLabel->setText(QString::number(pagination.total > 0 ? pagination.total : orders.size()));
        }
        if (totalSpentLabel) {
            totalSpentLabel->setText(QString("₹%1").arg(totalSum, 0, 'f', 0));
        }

        if (orders.isEmpty()) {
            emptyStateWidget->setVisible(true);
        } else {
            emptyStateWidget->setVisible(false);
            for (const auto &o : orders) {
                auto *card = createOrderCard(o);
                ordersLayout->addWidget(card);
            }
        }
    });
}

void OrdersWindow::handleTabChange(int tabIndex)
{
    if (currentTab == tabIndex) return;
    currentTab = tabIndex;
    currentPage = 1;

    if (currentTab == 0) {
        purchasesTabBtn->setStyleSheet(QStringLiteral("QPushButton { background: #4F46E5; color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: 700; padding: 0 20px; }"));
        salesTabBtn->setStyleSheet(QStringLiteral("QPushButton { background: white; color: #475569; border: 1px solid #CBD5E1; border-radius: 8px; font-size: 14px; font-weight: 700; padding: 0 20px; } QPushButton:hover { background: #F1F5F9; color: #0F172A; }"));
        if (statsTitleOrders) statsTitleOrders->setText(QStringLiteral("Total Purchases"));
        if (statsTitleSpent) statsTitleSpent->setText(QStringLiteral("Total Spent"));
    } else {
        salesTabBtn->setStyleSheet(QStringLiteral("QPushButton { background: #4F46E5; color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: 700; padding: 0 20px; }"));
        purchasesTabBtn->setStyleSheet(QStringLiteral("QPushButton { background: white; color: #475569; border: 1px solid #CBD5E1; border-radius: 8px; font-size: 14px; font-weight: 700; padding: 0 20px; } QPushButton:hover { background: #F1F5F9; color: #0F172A; }"));
        if (statsTitleOrders) statsTitleOrders->setText(QStringLiteral("Total Sales"));
        if (statsTitleSpent) statsTitleSpent->setText(QStringLiteral("Total Revenue"));
    }

    refreshOrders();
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

void OrdersWindow::showOrderDetail(const QString &orderId)
{
    auto *detailWin = new OrderDetailWindow(orderId);
    detailWin->setAttribute(Qt::WA_DeleteOnClose);
    connect(detailWin, &OrderDetailWindow::backRequested, detailWin, &QWidget::close);
    connect(detailWin, &OrderDetailWindow::orderUpdated, this, &OrdersWindow::refreshOrders);
    detailWin->show();
    detailWin->raise();
    detailWin->activateWindow();
}

void OrdersWindow::handleBack()
{
    emit backRequested();
}

void OrdersWindow::handleBrowse()
{
    emit browseRequested();
}

bool OrdersWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *widget = qobject_cast<QWidget*>(watched);
        if (widget && widget->property("orderId").isValid()) {
            QString orderId = widget->property("orderId").toString();
            showOrderDetail(orderId);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}
