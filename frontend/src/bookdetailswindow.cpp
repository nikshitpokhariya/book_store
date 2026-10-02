#include "bookdetailswindow.h"
#include "database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QMessageBox>
#include <QSizePolicy>
#include <QPixmap>
#include <QFont>


// =========================================================
// CONSTRUCTOR
// =========================================================

BookDetailsWindow::BookDetailsWindow(
    int bookId,
    const QString &userName,
    QWidget *parent
    )
    : QWidget(parent),
    bookId(bookId),
    userName(userName),
    titleLabel(nullptr),
    authorLabel(nullptr),
    priceLabel(nullptr),
    conditionLabel(nullptr),
    categoryLabel(nullptr),
    isbnLabel(nullptr),
    locationLabel(nullptr),
    sellerLabel(nullptr),
    descriptionLabel(nullptr),
    coverLabel(nullptr),
    orderButton(nullptr),
    backButton(nullptr)
{
    setWindowTitle(
        "BookBazzar - Book Details"
        );

    resize(
        1400,
        850
        );

    setMinimumSize(
        1000,
        650
        );

    setupUI();

    loadBook();
}


// =========================================================
// DESTRUCTOR
// =========================================================

BookDetailsWindow::~BookDetailsWindow()
{
}


// =========================================================
// SETUP UI
// =========================================================

void BookDetailsWindow::setupUI()
{
    setStyleSheet(R"(

        QWidget {
            font-family: "Segoe UI";
        }

        QScrollArea {
            border: none;
            background-color: #F7F8FC;
        }

        QScrollBar:vertical {
            width: 10px;
            background: transparent;
            margin: 4px;
        }

        QScrollBar::handle:vertical {
            background: #C7CBD6;
            border-radius: 5px;
            min-height: 45px;
        }

        QScrollBar::handle:vertical:hover {
            background: #A5A9B4;
        }

        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0px;
        }

        QScrollBar:horizontal {
            height: 0px;
        }

    )");


    // =====================================================
    // SCROLL AREA
    // =====================================================

    QScrollArea *scrollArea =
        new QScrollArea(this);

    scrollArea->setWidgetResizable(true);

    scrollArea->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
        );

    scrollArea->setVerticalScrollBarPolicy(
        Qt::ScrollBarAsNeeded
        );

    scrollArea->setFrameShape(
        QFrame::NoFrame
        );


    // =====================================================
    // PAGE
    // =====================================================

    QWidget *page =
        new QWidget();

    page->setStyleSheet(
        "background-color: #F7F8FC;"
        );


    QVBoxLayout *pageLayout =
        new QVBoxLayout(page);

    pageLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    pageLayout->setSpacing(0);


    // =====================================================
    // TOP BAR
    // =====================================================

    pageLayout->addWidget(
        createTopBar()
        );


    // =====================================================
    // DETAILS
    // =====================================================

    pageLayout->addWidget(
        createDetailsSection()
        );


    scrollArea->setWidget(
        page
        );


    // =====================================================
    // MAIN LAYOUT
    // =====================================================

    QVBoxLayout *mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    mainLayout->setSpacing(0);

    mainLayout->addWidget(
        scrollArea
        );
}


// =========================================================
// TOP BAR
// =========================================================

QWidget* BookDetailsWindow::createTopBar()
{
    QFrame *bar =
        new QFrame();

    bar->setFixedHeight(
        76
        );

    bar->setStyleSheet(R"(

        QFrame {
            background-color: white;
            border-bottom: 1px solid #E5E7EB;
        }

    )");


    QHBoxLayout *layout =
        new QHBoxLayout(bar);

    layout->setContentsMargins(
        32,
        10,
        32,
        10
        );

    layout->setSpacing(8);


    // =====================================================
    // LOGO
    // =====================================================

    QLabel *logo =
        new QLabel(
            "BookBazzar"
            );

    logo->setStyleSheet(R"(

        QLabel {
            color: #172033;
            font-size: 25px;
            font-weight: 800;
        }

    )");

    layout->addWidget(
        logo
        );


    QLabel *dot =
        new QLabel(
            "."
            );

    dot->setStyleSheet(R"(

        QLabel {
            color: #5B5FEF;
            font-size: 30px;
            font-weight: 800;
        }

    )");

    layout->addWidget(
        dot
        );


    layout->addSpacing(
        30
        );


    // =====================================================
    // PAGE TITLE
    // =====================================================

    QLabel *pageTitle =
        new QLabel(
            "Book Details"
            );

    pageTitle->setStyleSheet(R"(

        QLabel {
            color: #6B7280;
            font-size: 13px;
            font-weight: 600;
        }

    )");

    layout->addWidget(
        pageTitle
        );


    layout->addStretch();


    // =====================================================
    // USER
    // =====================================================

    QLabel *userLabel =
        new QLabel(
            "Hello, " + userName
            );

    userLabel->setStyleSheet(R"(

        QLabel {
            color: #374151;
            font-size: 14px;
            font-weight: 600;
            padding-right: 15px;
        }

    )");

    layout->addWidget(
        userLabel
        );


    // =====================================================
    // BACK BUTTON
    // =====================================================

    backButton =
        new QPushButton(
            "← Back"
            );

    backButton->setMinimumSize(
        90,
        40
        );

    backButton->setStyleSheet(R"(

        QPushButton {
            background-color: #F8F9FC;
            color: #374151;
            border: 1px solid #E1E4EA;
            border-radius: 9px;
            padding: 0 15px;
            font-size: 13px;
            font-weight: 700;
        }

        QPushButton:hover {
            background-color: #F3F4FF;
            color: #4F46E5;
            border-color: #C7D2FE;
        }

        QPushButton:pressed {
            background-color: #EDE9FE;
        }

    )");

    layout->addWidget(
        backButton
        );


    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &BookDetailsWindow::handleBack
        );


    return bar;
}


// =========================================================
// DETAILS SECTION
// =========================================================

QWidget* BookDetailsWindow::createDetailsSection()
{
    QWidget *section =
        new QWidget();

    section->setStyleSheet(
        "background-color: #F7F8FC;"
        );


    QVBoxLayout *mainLayout =
        new QVBoxLayout(section);

    mainLayout->setContentsMargins(
        45,
        35,
        45,
        45
        );

    mainLayout->setSpacing(25);


    // =====================================================
    // MAIN CARD
    // =====================================================

    QFrame *mainCard =
        new QFrame();

    mainCard->setStyleSheet(R"(

        QFrame {
            background-color: white;
            border: 1px solid #E5E7EB;
            border-radius: 18px;
        }

    )");


    QHBoxLayout *cardLayout =
        new QHBoxLayout(mainCard);

    cardLayout->setContentsMargins(
        25,
        25,
        25,
        25
        );

    cardLayout->setSpacing(30);


    // =====================================================
    // COVER
    // =====================================================

    coverLabel =
        new QLabel();

    coverLabel->setFixedSize(
        300,
        390
        );

    coverLabel->setAlignment(
        Qt::AlignCenter
        );

    coverLabel->setStyleSheet(R"(

        QLabel {
            background-color: #EEF2FF;
            color: #4F46E5;
            border-radius: 14px;
            font-size: 60px;
            font-weight: 900;
        }

    )");

    cardLayout->addWidget(
        coverLabel
        );


    // =====================================================
    // RIGHT DETAILS
    // =====================================================

    QWidget *details =
        new QWidget();


    QVBoxLayout *detailsLayout =
        new QVBoxLayout(details);

    detailsLayout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    detailsLayout->setSpacing(12);


    // =====================================================
    // TITLE
    // =====================================================

    titleLabel =
        new QLabel(
            "Book Title"
            );

    titleLabel->setWordWrap(
        true
        );

    titleLabel->setStyleSheet(R"(

        QLabel {
            color: #172033;
            font-size: 30px;
            font-weight: 900;
        }

    )");

    detailsLayout->addWidget(
        titleLabel
        );


    // =====================================================
    // AUTHOR
    // =====================================================

    authorLabel =
        new QLabel(
            "by Author"
            );

    authorLabel->setStyleSheet(R"(

        QLabel {
            color: #6B7280;
            font-size: 15px;
            font-weight: 600;
        }

    )");

    detailsLayout->addWidget(
        authorLabel
        );


    detailsLayout->addSpacing(
        8
        );


    // =====================================================
    // PRICE
    // =====================================================

    priceLabel =
        new QLabel(
            "₹0"
            );

    priceLabel->setStyleSheet(R"(

        QLabel {
            color: #111827;
            font-size: 30px;
            font-weight: 900;
        }

    )");

    detailsLayout->addWidget(
        priceLabel
        );


    // =====================================================
    // CONDITION
    // =====================================================

    conditionLabel =
        new QLabel(
            "Condition"
            );

    conditionLabel->setMaximumWidth(
        160
        );

    conditionLabel->setAlignment(
        Qt::AlignCenter
        );

    conditionLabel->setStyleSheet(R"(

        QLabel {
            background-color: #ECFDF5;
            color: #047857;
            border-radius: 7px;
            padding: 7px 12px;
            font-size: 11px;
            font-weight: 800;
        }

    )");

    detailsLayout->addWidget(
        conditionLabel,
        0,
        Qt::AlignLeft
        );


    // =====================================================
    // INFORMATION GRID
    // =====================================================

    QGridLayout *infoGrid =
        new QGridLayout();

    infoGrid->setHorizontalSpacing(
        25
        );

    infoGrid->setVerticalSpacing(
        10
        );


    categoryLabel =
        new QLabel();

    isbnLabel =
        new QLabel();

    locationLabel =
        new QLabel();

    sellerLabel =
        new QLabel();


    categoryLabel->setStyleSheet(
        "color:#374151;font-size:12px;"
        );

    isbnLabel->setStyleSheet(
        "color:#374151;font-size:12px;"
        );

    locationLabel->setStyleSheet(
        "color:#374151;font-size:12px;"
        );

    sellerLabel->setStyleSheet(
        "color:#374151;font-size:12px;"
        );


    infoGrid->addWidget(
        new QLabel("<b>Category</b>"),
        0,
        0
        );

    infoGrid->addWidget(
        categoryLabel,
        0,
        1
        );


    infoGrid->addWidget(
        new QLabel("<b>ISBN</b>"),
        1,
        0
        );

    infoGrid->addWidget(
        isbnLabel,
        1,
        1
        );


    infoGrid->addWidget(
        new QLabel("<b>Location</b>"),
        2,
        0
        );

    infoGrid->addWidget(
        locationLabel,
        2,
        1
        );


    infoGrid->addWidget(
        new QLabel("<b>Seller</b>"),
        3,
        0
        );

    infoGrid->addWidget(
        sellerLabel,
        3,
        1
        );


    detailsLayout->addLayout(
        infoGrid
        );


    detailsLayout->addSpacing(
        10
        );


    // =====================================================
    // DESCRIPTION
    // =====================================================

    QLabel *descriptionHeading =
        new QLabel(
            "Description"
            );

    descriptionHeading->setStyleSheet(R"(

        QLabel {
            color: #172033;
            font-size: 15px;
            font-weight: 800;
        }

    )");

    detailsLayout->addWidget(
        descriptionHeading
        );


    descriptionLabel =
        new QLabel(
            "No description available."
            );

    descriptionLabel->setWordWrap(
        true
        );

    descriptionLabel->setMinimumHeight(
        60
        );

    descriptionLabel->setStyleSheet(R"(

        QLabel {
            color: #6B7280;
            font-size: 12px;
        }

    )");

    detailsLayout->addWidget(
        descriptionLabel
        );


    detailsLayout->addStretch();


    // =====================================================
    // ORDER BUTTON
    // =====================================================

    orderButton =
        new QPushButton(
            "Buy / Place Order"
            );

    orderButton->setMinimumHeight(
        48
        );

    orderButton->setMaximumWidth(
        250
        );

    orderButton->setStyleSheet(R"(

        QPushButton {
            background-color: #5B5FEF;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 0 25px;
            font-size: 13px;
            font-weight: 800;
        }

        QPushButton:hover {
            background-color: #4F46E5;
        }

        QPushButton:pressed {
            background-color: #4338CA;
        }

        QPushButton:disabled {
            background-color: #D1D5DB;
            color: #6B7280;
        }

    )");


    detailsLayout->addWidget(
        orderButton,
        0,
        Qt::AlignLeft
        );


    connect(
        orderButton,
        &QPushButton::clicked,
        this,
        &BookDetailsWindow::handleOrder
        );


    cardLayout->addWidget(
        details,
        1
        );


    mainLayout->addWidget(
        mainCard
        );


    return section;
}


// =========================================================
// INFO CARD
// =========================================================

QFrame* BookDetailsWindow::createInfoCard(
    const QString &label,
    const QString &value
    )
{
    QFrame *card =
        new QFrame();

    card->setStyleSheet(R"(

        QFrame {
            background-color: #F8F9FC;
            border: 1px solid #E5E7EB;
            border-radius: 10px;
        }

    )");


    QVBoxLayout *layout =
        new QVBoxLayout(card);

    layout->setContentsMargins(
        12,
        10,
        12,
        10
        );


    QLabel *labelWidget =
        new QLabel(label);

    labelWidget->setStyleSheet(
        "color:#6B7280;font-size:10px;font-weight:700;"
        );


    QLabel *valueWidget =
        new QLabel(value);

    valueWidget->setWordWrap(
        true
        );

    valueWidget->setStyleSheet(
        "color:#172033;font-size:12px;font-weight:700;"
        );


    layout->addWidget(
        labelWidget
        );

    layout->addWidget(
        valueWidget
        );


    return card;
}


// =========================================================
// LOAD BOOK
// =========================================================

bool BookDetailsWindow::loadBook()
{
    ::Book databaseBook =
        Database::instance().getBookById(
            bookId
            );


    if (databaseBook.id == -1)
    {
        QMessageBox::critical(
            this,
            "Book Not Found",
            "The selected book could not be found."
            );

        if (orderButton)
        {
            orderButton->setDisabled(
                true
                );
        }

        return false;
    }


    // =====================================================
    // COPY DATABASE DATA
    // =====================================================

    book.id =
        databaseBook.id;

    book.title =
        databaseBook.title;

    book.author =
        databaseBook.author;

    book.isbn =
        databaseBook.isbn;

    book.category =
        databaseBook.category;

    book.condition =
        databaseBook.condition;

    book.price =
        databaseBook.price;

    book.description =
        databaseBook.description;

    book.location =
        databaseBook.location;

    book.imagePath =
        databaseBook.imagePath;

    book.sellerName =
        databaseBook.sellerName;

    book.createdAt =
        databaseBook.createdAt;


    // =====================================================
    // DISPLAY TITLE
    // =====================================================

    titleLabel->setText(
        book.title.isEmpty()
            ? "Untitled Book"
            : book.title
        );


    // =====================================================
    // AUTHOR
    // =====================================================

    authorLabel->setText(
        book.author.isEmpty()
            ? "by Unknown Author"
            : "by " + book.author
        );


    // =====================================================
    // PRICE
    // =====================================================

    priceLabel->setText(
        QString("₹%1")
            .arg(
                book.price,
                0,
                'f',
                0
                )
        );


    // =====================================================
    // CONDITION
    // =====================================================

    conditionLabel->setText(
        book.condition.isEmpty()
            ? "Not specified"
            : book.condition
        );


    // =====================================================
    // CATEGORY
    // =====================================================

    categoryLabel->setText(
        book.category.isEmpty()
            ? "Not specified"
            : book.category
        );


    // =====================================================
    // ISBN
    // =====================================================

    isbnLabel->setText(
        book.isbn.isEmpty()
            ? "Not specified"
            : book.isbn
        );


    // =====================================================
    // LOCATION
    // =====================================================

    locationLabel->setText(
        book.location.isEmpty()
            ? "Not specified"
            : book.location
        );


    // =====================================================
    // SELLER
    // =====================================================

    sellerLabel->setText(
        book.sellerName.isEmpty()
            ? "Unknown"
            : book.sellerName
        );


    // =====================================================
    // DESCRIPTION
    // =====================================================

    descriptionLabel->setText(
        book.description.isEmpty()
            ? "No description available."
            : book.description
        );


    // =====================================================
    // COVER IMAGE
    // =====================================================

    coverLabel->clear();

    if (!book.imagePath.trimmed().isEmpty())
    {
        QPixmap pixmap(
            book.imagePath
            );


        if (!pixmap.isNull())
        {
            coverLabel->setPixmap(
                pixmap.scaled(
                    coverLabel->size(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                    )
                );
        }
        else
        {
            coverLabel->setText(
                book.title.left(2).toUpper()
                );
        }
    }
    else
    {
        QString initials =
            book.title.left(2).toUpper();

        if (initials.isEmpty())
        {
            initials = "BK";
        }

        coverLabel->setText(
            initials
            );
    }


    // =====================================================
    // OWN BOOK CHECK
    // =====================================================

    if (!userName.trimmed().isEmpty() &&
        book.sellerName.compare(
            userName,
            Qt::CaseInsensitive
            ) == 0)
    {
        orderButton->setText(
            "Your Book"
            );

        orderButton->setDisabled(
            true
            );
    }


    return true;
}


// =========================================================
// ORDER
// =========================================================

void BookDetailsWindow::handleOrder()
{
    // =====================================================
    // INVALID BOOK
    // =====================================================

    if (book.id == -1)
    {
        QMessageBox::warning(
            this,
            "Error",
            "Invalid book."
            );

        return;
    }


    // =====================================================
    // LOGIN CHECK
    // =====================================================

    if (userName.trimmed().isEmpty())
    {
        QMessageBox::warning(
            this,
            "Login Required",
            "Please login before placing an order."
            );

        return;
    }


    // =====================================================
    // OWN BOOK CHECK
    // =====================================================

    if (book.sellerName.compare(
            userName,
            Qt::CaseInsensitive
            ) == 0)
    {
        QMessageBox::warning(
            this,
            "Cannot Buy",
            "You cannot buy your own book."
            );

        return;
    }


    // =====================================================
    // CONFIRM ORDER
    // =====================================================

    QMessageBox::StandardButton reply =
        QMessageBox::question(
            this,
            "Confirm Order",

            "Do you want to place an order for:\n\n"
                + book.title
                + "\n\nPrice: ₹"
                + QString::number(
                    book.price,
                    'f',
                    0
                    ),

            QMessageBox::Yes |
                QMessageBox::No
            );


    if (reply != QMessageBox::Yes)
    {
        return;
    }


    // =====================================================
    // CREATE ORDER
    // =====================================================

    bool success =
        Database::instance().createOrder(
            book.id,
            userName
            );


    if (!success)
    {
        QMessageBox::warning(
            this,
            "Order Failed",

            "Unable to place the order.\n\n"
            "The book may have already been sold "
            "or is no longer available."
            );

        return;
    }


    // =====================================================
    // SUCCESS
    // =====================================================

    QMessageBox::information(
        this,
        "Order Successful",
        "Your order has been placed successfully!"
        );


    orderButton->setDisabled(
        true
        );

    orderButton->setText(
        "Order Placed"
        );


    emit orderPlaced();
}


// =========================================================
// BACK
// =========================================================

void BookDetailsWindow::handleBack()
{
    emit backRequested();
}