#include "orderswindow.h"
#include "database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QFileInfo>
#include <QPixmap>

OrdersWindow::OrdersWindow(const QString &userName, QWidget *parent)
    : QWidget(parent),
      userName(userName),
      totalOrdersLabel(nullptr),
      totalSpentLabel(nullptr),
      ordersLayout(nullptr)
{
    setWindowTitle("BookBazzar - My Orders");
    resize(1200, 800);
    setMinimumSize(950, 600);

    setupUI();
    refreshOrders();
}

OrdersWindow::~OrdersWindow()
{
}

void OrdersWindow::setupUI()
{
    setStyleSheet(R"(
        QWidget {
            font-family: "Segoe UI";
            background-color: #F8FAFC;
        }
        QScrollArea {
            border: none;
            background-color: #F8FAFC;
        }
        QScrollBar:vertical {
            width: 8px;
            background: transparent;
        }
        QScrollBar::handle:vertical {
            background: #CBD5E1;
            border-radius: 4px;
            min-height: 40px;
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(createTopBar());

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *content = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 30, 40, 40);
    contentLayout->setSpacing(24);

    contentLayout->addWidget(createStatsBar());

    ordersLayout = new QVBoxLayout();
    ordersLayout->setContentsMargins(0, 0, 0, 0);
    ordersLayout->setSpacing(14);
    contentLayout->addLayout(ordersLayout);

    contentLayout->addStretch();
    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

QWidget* OrdersWindow::createTopBar()
{
    QFrame *bar = new QFrame();
    bar->setFixedHeight(74);
    bar->setStyleSheet("QFrame { background-color: white; border-bottom: 1px solid #E2E8F0; }");

    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(36, 12, 36, 12);
    layout->setSpacing(16);

    QPushButton *backButton = new QPushButton("← Back");
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

    QLabel *title = new QLabel("My Purchase Orders");
    title->setStyleSheet("QLabel { color: #0F172A; font-size: 22px; font-weight: 800; }");
    layout->addWidget(title);

    layout->addStretch();

    QPushButton *browseBtn = new QPushButton("Browse Books");
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

QWidget* OrdersWindow::createStatsBar()
{
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);

    auto createStatCard = [](const QString &title, QLabel* &labelRef, const QString &color) {
        QFrame *card = new QFrame();
        card->setStyleSheet("QFrame { background-color: white; border: 1px solid #E2E8F0; border-radius: 12px; }");
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 16, 20, 16);
        cardLayout->setSpacing(4);

        QLabel *titleLabel = new QLabel(title);
        titleLabel->setStyleSheet("QLabel { color: #64748B; font-size: 12px; font-weight: 600; }");
        cardLayout->addWidget(titleLabel);

        labelRef = new QLabel("0");
        labelRef->setStyleSheet(QString("QLabel { color: %1; font-size: 26px; font-weight: 800; }").arg(color));
        cardLayout->addWidget(labelRef);

        return card;
    };

    layout->addWidget(createStatCard("Total Orders", totalOrdersLabel, "#0F172A"));
    layout->addWidget(createStatCard("Total Amount Spent", totalSpentLabel, "#10B981"));

    return container;
}

void OrdersWindow::refreshOrders()
{
    if (!ordersLayout) return;

    QLayoutItem *item;
    while ((item = ordersLayout->takeAt(0)) != nullptr)
    {
        if (item->widget()) delete item->widget();
        delete item;
    }

    QList<QVariantMap> orders = Database::instance().getUserOrders(userName);

    int total = orders.size();
    double totalSpent = 0.0;

    for (const QVariantMap &o : orders)
    {
        totalSpent += o["price"].toDouble();
    }

    if (totalOrdersLabel) totalOrdersLabel->setText(QString::number(total));
    if (totalSpentLabel) totalSpentLabel->setText(QString("₹%1").arg(QString::number(totalSpent, 'f', 0)));

    if (orders.isEmpty())
    {
        QLabel *empty = new QLabel("You haven't placed any book orders yet.\nBrowse our marketplace to find books you love!");
        empty->setAlignment(Qt::AlignCenter);
        empty->setMinimumHeight(220);
        empty->setStyleSheet(R"(
            QLabel {
                background-color: white;
                border: 1px dashed #CBD5E1;
                border-radius: 16px;
                color: #64748B;
                font-size: 15px;
                font-weight: 600;
            }
        )");
        ordersLayout->addWidget(empty);
    }
    else
    {
        for (const QVariantMap &o : orders)
        {
            ordersLayout->addWidget(createOrderCard(o));
        }
    }
}

QWidget* OrdersWindow::createOrderCard(const QVariantMap &order)
{
    QFrame *card = new QFrame();
    card->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border: 1px solid #E2E8F0;
            border-radius: 14px;
        }
        QFrame:hover {
            border-color: #CBD5E1;
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(card);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(20);

    // Cover
    QLabel *cover = new QLabel();
    cover->setFixedSize(65, 80);
    cover->setAlignment(Qt::AlignCenter);

    QString imgPath = order["image_path"].toString();
    QString title = order["title"].toString();
    bool loaded = false;

    if (!imgPath.isEmpty() && QFileInfo::exists(imgPath))
    {
        QPixmap pix(imgPath);
        if (!pix.isNull())
        {
            cover->setPixmap(pix.scaled(65, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            loaded = true;
        }
    }
    if (!loaded)
    {
        cover->setText(title.left(2).toUpper());
        cover->setStyleSheet("QLabel { background-color: #EEF2FF; color: #4F46E5; border-radius: 8px; font-weight: 900; font-size: 16px; }");
    }
    layout->addWidget(cover);

    // Info
    QVBoxLayout *info = new QVBoxLayout();
    info->setSpacing(4);

    QLabel *orderNum = new QLabel(QString("Order #ORD-%1  •  %2")
        .arg(order["order_id"].toString())
        .arg(order["order_date"].toString()));
    orderNum->setStyleSheet("QLabel { color: #64748B; font-size: 11px; font-weight: 700; }");
    info->addWidget(orderNum);

    QLabel *titleLbl = new QLabel(title);
    titleLbl->setStyleSheet("QLabel { color: #0F172A; font-size: 16px; font-weight: 800; }");
    info->addWidget(titleLbl);

    QLabel *authorLbl = new QLabel("by " + order["author"].toString());
    authorLbl->setStyleSheet("QLabel { color: #64748B; font-size: 12px; }");
    info->addWidget(authorLbl);

    layout->addLayout(info, 1);

    // Status Pill
    QString status = order["status"].toString();
    QLabel *statusBadge = new QLabel(status);
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setFixedSize(95, 30);
    if (status.compare("Completed", Qt::CaseInsensitive) == 0 || status.compare("Delivered", Qt::CaseInsensitive) == 0)
    {
        statusBadge->setStyleSheet("QLabel { background-color: #ECFDF5; color: #059669; border-radius: 6px; font-size: 11px; font-weight: 800; }");
    }
    else
    {
        statusBadge->setStyleSheet("QLabel { background-color: #FEF3C7; color: #D97706; border-radius: 6px; font-size: 11px; font-weight: 800; }");
    }
    layout->addWidget(statusBadge);

    // Price
    double priceVal = order["price"].toDouble();
    QLabel *priceLbl = new QLabel(QString("₹%1").arg(QString::number(priceVal, 'f', 0)));
    priceLbl->setStyleSheet("QLabel { color: #0F172A; font-size: 18px; font-weight: 900; }");
    layout->addWidget(priceLbl);

    return card;
}

void OrdersWindow::handleBack()
{
    emit backRequested();
}

void OrdersWindow::handleBrowse()
{
    emit browseRequested();
}
