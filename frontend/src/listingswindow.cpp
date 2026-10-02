#include "listingswindow.h"
#include "database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QMessageBox>
#include <QFileInfo>
#include <QPixmap>

ListingsWindow::ListingsWindow(const QString &userName, QWidget *parent)
    : QWidget(parent),
      userName(userName),
      totalCountLabel(nullptr),
      availableCountLabel(nullptr),
      soldCountLabel(nullptr),
      cardsLayout(nullptr)
{
    setWindowTitle("BookBazzar - My Book Listings");
    resize(1200, 800);
    setMinimumSize(950, 600);

    setupUI();
    refreshListings();
}

ListingsWindow::~ListingsWindow()
{
}

void ListingsWindow::setupUI()
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
            margin: 0;
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

    // 1. Top Navigation Bar
    mainLayout->addWidget(createTopBar());

    // 2. Scrollable Body
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *content = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 30, 40, 40);
    contentLayout->setSpacing(24);

    // Stats bar
    contentLayout->addWidget(createStatsBar());

    // Cards container
    cardsLayout = new QVBoxLayout();
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(14);
    contentLayout->addLayout(cardsLayout);

    contentLayout->addStretch();
    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

QWidget* ListingsWindow::createTopBar()
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
    connect(backButton, &QPushButton::clicked, this, &ListingsWindow::handleBack);
    layout->addWidget(backButton);

    QLabel *title = new QLabel("My Book Listings");
    title->setStyleSheet("QLabel { color: #0F172A; font-size: 22px; font-weight: 800; }");
    layout->addWidget(title);

    layout->addStretch();

    QPushButton *addBtn = new QPushButton("+ Sell Another Book");
    addBtn->setMinimumSize(170, 42);
    addBtn->setStyleSheet(R"(
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
    connect(addBtn, &QPushButton::clicked, this, &ListingsWindow::handleAddNew);
    layout->addWidget(addBtn);

    return bar;
}

QWidget* ListingsWindow::createStatsBar()
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

    layout->addWidget(createStatCard("Total Listings", totalCountLabel, "#0F172A"));
    layout->addWidget(createStatCard("Active / Available", availableCountLabel, "#10B981"));
    layout->addWidget(createStatCard("Books Sold", soldCountLabel, "#6366F1"));

    return container;
}

void ListingsWindow::refreshListings()
{
    if (!cardsLayout) return;

    // Clear previous items
    QLayoutItem *item;
    while ((item = cardsLayout->takeAt(0)) != nullptr)
    {
        if (item->widget()) delete item->widget();
        delete item;
    }

    QList<Book> books = Database::instance().getUserListings(userName);

    int total = books.size();
    int available = 0;
    int sold = 0;

    for (const Book &b : books)
    {
        if (b.status.compare("Sold", Qt::CaseInsensitive) == 0) sold++;
        else available++;
    }

    if (totalCountLabel) totalCountLabel->setText(QString::number(total));
    if (availableCountLabel) availableCountLabel->setText(QString::number(available));
    if (soldCountLabel) soldCountLabel->setText(QString::number(sold));

    if (books.isEmpty())
    {
        QLabel *empty = new QLabel("You haven't listed any books for sale yet.\nClick '+ Sell Another Book' above to create your first listing!");
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
        cardsLayout->addWidget(empty);
    }
    else
    {
        for (const Book &b : books)
        {
            cardsLayout->addWidget(createListingCard(
                b.id, b.title, b.author, b.category, b.condition, b.price, b.status, b.imagePath
            ));
        }
    }
}

QWidget* ListingsWindow::createListingCard(
    int bookId, const QString &title, const QString &author,
    const QString &category, const QString &condition,
    double price, const QString &status, const QString &imagePath)
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
    layout->setContentsMargins(18, 16, 20, 16);
    layout->setSpacing(20);

    // Book Cover / Monogram
    QLabel *cover = new QLabel();
    cover->setFixedSize(65, 80);
    cover->setAlignment(Qt::AlignCenter);

    bool loaded = false;
    if (!imagePath.isEmpty() && QFileInfo::exists(imagePath))
    {
        QPixmap pix(imagePath);
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

    QLabel *titleLbl = new QLabel(title);
    titleLbl->setStyleSheet("QLabel { color: #0F172A; font-size: 16px; font-weight: 800; }");
    info->addWidget(titleLbl);

    QLabel *authorLbl = new QLabel("by " + author + "  •  " + category + "  •  " + condition);
    authorLbl->setStyleSheet("QLabel { color: #64748B; font-size: 12px; font-weight: 500; }");
    info->addWidget(authorLbl);

    layout->addLayout(info, 1);

    // Status Pill
    bool isSold = (status.compare("Sold", Qt::CaseInsensitive) == 0);
    QLabel *statusBadge = new QLabel(isSold ? "Sold" : "Available");
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setFixedSize(85, 30);
    if (isSold)
    {
        statusBadge->setStyleSheet("QLabel { background-color: #F1F5F9; color: #475569; border-radius: 6px; font-size: 11px; font-weight: 800; }");
    }
    else
    {
        statusBadge->setStyleSheet("QLabel { background-color: #ECFDF5; color: #059669; border-radius: 6px; font-size: 11px; font-weight: 800; }");
    }
    layout->addWidget(statusBadge);

    // Price
    QLabel *priceLbl = new QLabel(QString("₹%1").arg(QString::number(price, 'f', 0)));
    priceLbl->setStyleSheet("QLabel { color: #0F172A; font-size: 18px; font-weight: 900; }");
    layout->addWidget(priceLbl);

    layout->addSpacing(10);

    // Delete Button
    QPushButton *deleteBtn = new QPushButton("Delete");
    deleteBtn->setFixedSize(85, 36);
    deleteBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #FFF1F2;
            color: #E11D48;
            border: 1px solid #FFE4E6;
            border-radius: 8px;
            font-size: 12px;
            font-weight: 700;
        }
        QPushButton:hover {
            background-color: #FFE4E6;
            color: #BE123C;
        }
    )");
    connect(deleteBtn, &QPushButton::clicked, this, [this, bookId, title]() {
        handleDeleteListing(bookId, title);
    });
    layout->addWidget(deleteBtn);

    return card;
}

void ListingsWindow::handleDeleteListing(int bookId, const QString &bookTitle)
{
    QMessageBox::StandardButton res = QMessageBox::question(
        this, "Delete Listing",
        QString("Are you sure you want to remove \"%1\" from your listings?").arg(bookTitle),
        QMessageBox::Yes | QMessageBox::No
    );

    if (res == QMessageBox::Yes)
    {
        bool ok = Database::instance().deleteBook(bookId, userName);
        if (ok)
        {
            QMessageBox::information(this, "Listing Removed", "Your book listing has been removed successfully.");
            refreshListings();
        }
        else
        {
            QMessageBox::warning(this, "Error", "Could not delete this book listing.");
        }
    }
}

void ListingsWindow::handleAddNew()
{
    emit addNewListingRequested();
}

void ListingsWindow::handleBack()
{
    emit backRequested();
}
