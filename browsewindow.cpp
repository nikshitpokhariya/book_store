#include "browsewindow.h"
#include "database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QSizePolicy>
#include <QList>
#include <QPixmap>
#include <QFileInfo>
#include <QDir>
#include <QDebug>


// =========================================================
// CONSTRUCTOR
// =========================================================

BrowseWindow::BrowseWindow(
    const QString &userName,
    QWidget *parent
    )
    : QWidget(parent),
    userName(userName),
    searchEdit(nullptr),
    searchButton(nullptr),
    categoryCombo(nullptr),
    backButton(nullptr),
    booksGrid(nullptr),
    booksContainer(nullptr),
    resultLabel(nullptr)
{
    setWindowTitle(
        "BookBazzar - Browse Books"
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

    populateBooks();
}


// =========================================================
// DESTRUCTOR
// =========================================================

BrowseWindow::~BrowseWindow()
{
}


// =========================================================
// MAIN UI
// =========================================================

void BrowseWindow::setupUI()
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
    // CONTENT
    // =====================================================

    QWidget *content =
        new QWidget();

    content->setStyleSheet(
        "background-color: #F7F8FC;"
        );


    QVBoxLayout *contentLayout =
        new QVBoxLayout(content);

    contentLayout->setContentsMargins(
        32,
        28,
        32,
        40
        );

    contentLayout->setSpacing(0);


    // =====================================================
    // TITLE
    // =====================================================

    QHBoxLayout *titleLayout =
        new QHBoxLayout();


    titleLayout->addWidget(
        createSectionTitle(
            "Browse Books"
            )
        );


    titleLayout->addStretch();


    resultLabel =
        new QLabel(
            "Loading books..."
            );

    resultLabel->setStyleSheet(R"(

        QLabel {
            color: #6B7280;
            font-size: 12px;
            font-weight: 600;
        }

    )");


    titleLayout->addWidget(
        resultLabel
        );


    contentLayout->addLayout(
        titleLayout
        );


    contentLayout->addSpacing(
        18
        );


    // =====================================================
    // FILTER
    // =====================================================

    contentLayout->addWidget(
        createFilterBar()
        );


    contentLayout->addSpacing(
        25
        );


    // =====================================================
    // BOOKS
    // =====================================================

    contentLayout->addWidget(
        createBooksSection()
        );


    pageLayout->addWidget(
        content,
        1
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

QWidget* BrowseWindow::createTopBar()
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
            "Browse Marketplace"
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
        &BrowseWindow::handleBack
        );


    return bar;
}


// =========================================================
// FILTER BAR
// =========================================================

QWidget* BrowseWindow::createFilterBar()
{
    QFrame *filter =
        new QFrame();

    filter->setMinimumHeight(
        82
        );

    filter->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed
        );

    filter->setStyleSheet(R"(

        QFrame {
            background-color: white;
            border: 1px solid #E5E7EB;
            border-radius: 15px;
        }

    )");


    QHBoxLayout *layout =
        new QHBoxLayout(filter);

    layout->setContentsMargins(
        18,
        14,
        18,
        14
        );

    layout->setSpacing(10);


    // =====================================================
    // SEARCH
    // =====================================================

    searchEdit =
        new QLineEdit();

    searchEdit->setPlaceholderText(
        "Search by title, author or ISBN..."
        );

    searchEdit->setMinimumHeight(
        48
        );

    searchEdit->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed
        );

    searchEdit->setStyleSheet(R"(

        QLineEdit {
            background-color: #F8F9FC;
            color: #111827;
            border: 1px solid #E1E4EA;
            border-radius: 10px;
            padding-left: 16px;
            padding-right: 16px;
            font-size: 13px;
        }

        QLineEdit:focus {
            background-color: white;
            border: 2px solid #818CF8;
        }

    )");


    layout->addWidget(
        searchEdit,
        1
        );


    // =====================================================
    // SEARCH BUTTON
    // =====================================================

    searchButton =
        createPrimaryButton(
            "Search"
            );

    searchButton->setMinimumSize(
        105,
        48
        );


    layout->addWidget(
        searchButton
        );


    // =====================================================
    // CATEGORY
    // =====================================================

    categoryCombo =
        new QComboBox();

    categoryCombo->setMinimumSize(
        190,
        48
        );


    categoryCombo->addItem(
        "All Categories"
        );

    categoryCombo->addItem(
        "Academic"
        );

    categoryCombo->addItem(
        "Programming"
        );

    categoryCombo->addItem(
        "Engineering"
        );

    categoryCombo->addItem(
        "Fiction"
        );

    categoryCombo->addItem(
        "Competitive Exams"
        );

    categoryCombo->addItem(
        "School"
        );

    categoryCombo->addItem(
        "Novels"
        );


    categoryCombo->setStyleSheet(R"(

        QComboBox {
            background-color: #F8F9FC;
            color: #374151;
            border: 1px solid #E1E4EA;
            border-radius: 10px;
            padding: 0 12px;
            font-size: 13px;
            font-weight: 600;
        }

        QComboBox:focus {
            border: 2px solid #818CF8;
        }

        QComboBox::drop-down {
            border: none;
            width: 30px;
        }

        QComboBox QAbstractItemView {
            background-color: white;
            color: #374151;
            border: 1px solid #E5E7EB;
            selection-background-color: #EEF2FF;
            selection-color: #4F46E5;
        }

    )");


    layout->addWidget(
        categoryCombo
        );


    // =====================================================
    // CONNECTIONS
    // =====================================================

    connect(
        searchButton,
        &QPushButton::clicked,
        this,
        &BrowseWindow::handleSearch
        );


    connect(
        searchEdit,
        &QLineEdit::returnPressed,
        this,
        &BrowseWindow::handleSearch
        );


    connect(
        categoryCombo,
        &QComboBox::currentTextChanged,
        this,
        &BrowseWindow::handleCategoryChanged
        );


    return filter;
}


// =========================================================
// BOOKS SECTION
// =========================================================

QWidget* BrowseWindow::createBooksSection()
{
    QWidget *section =
        new QWidget();


    QVBoxLayout *layout =
        new QVBoxLayout(section);

    layout->setContentsMargins(
        0,
        0,
        0,
        0
        );


    booksContainer =
        new QWidget();

    booksContainer->setStyleSheet(
        "background-color: transparent;"
        );


    booksGrid =
        new QGridLayout(
            booksContainer
            );

    booksGrid->setContentsMargins(
        0,
        0,
        0,
        0
        );

    booksGrid->setHorizontalSpacing(
        16
        );

    booksGrid->setVerticalSpacing(
        18
        );


    layout->addWidget(
        booksContainer
        );


    return section;
}


// =========================================================
// POPULATE BOOKS
// =========================================================

void BrowseWindow::populateBooks()
{
    clearBooks();

    books.clear();


    QList<::Book> databaseBooks =
        Database::instance().getRecentBooks(
            50
            );


    for (const ::Book &databaseBook :
         databaseBooks)
    {
        Book item;


        item.id =
            databaseBook.id;


        item.title =
            databaseBook.title;


        item.author =
            databaseBook.author;


        item.condition =
            databaseBook.condition;


        item.price =
            QString("₹%1")
                .arg(
                    databaseBook.price,
                    0,
                    'f',
                    0
                    );


        item.location =
            databaseBook.location;


        item.category =
            databaseBook.category;


        // IMPORTANT
        // Get image path from database

        item.imagePath =
            databaseBook.imagePath;


        books.append(
            item
            );
    }


    // =====================================================
    // EMPTY
    // =====================================================

    if (books.isEmpty())
    {
        resultLabel->setText(
            "No books available"
            );


        QLabel *emptyLabel =
            new QLabel(
                "No books have been listed yet.\n\n"
                "Be the first person to sell a book!"
                );


        emptyLabel->setAlignment(
            Qt::AlignCenter
            );

        emptyLabel->setMinimumHeight(
            220
            );

        emptyLabel->setStyleSheet(R"(

            QLabel {
                background-color: white;
                border: 1px solid #E5E7EB;
                border-radius: 15px;
                color: #6B7280;
                font-size: 14px;
                font-weight: 600;
                padding: 30px;
            }

        )");


        booksGrid->addWidget(
            emptyLabel,
            0,
            0,
            1,
            4
            );

        return;
    }


    resultLabel->setText(
        QString("%1 book%2 found")
            .arg(books.size())
            .arg(
                books.size() == 1
                    ? ""
                    : "s"
                )
        );


    // =====================================================
    // GRID
    // =====================================================

    int columns = 4;


    for (int i = 0;
         i < books.size();
         ++i)
    {
        const Book &item =
            books[i];


        QFrame *card =
            createBookCard(
                item.id,
                item.title,
                item.author,
                item.condition,
                item.price,
                item.location,
                item.imagePath
                );


        int row =
            i / columns;


        int column =
            i % columns;


        booksGrid->addWidget(
            card,
            row,
            column
            );
    }


    for (int i = 0;
         i < columns;
         ++i)
    {
        booksGrid->setColumnStretch(
            i,
            1
            );
    }
}


// =========================================================
// CLEAR BOOKS
// =========================================================

void BrowseWindow::clearBooks()
{
    if (!booksGrid)
    {
        return;
    }


    while (QLayoutItem *item =
           booksGrid->takeAt(0))
    {
        if (item->widget())
        {
            item->widget()->deleteLater();
        }

        delete item;
    }
}


// =========================================================
// FILTER BOOKS
// =========================================================

void BrowseWindow::filterBooks(
    const QString &searchText,
    const QString &category
    )
{
    clearBooks();

    books.clear();


    QList<::Book> databaseBooks;


    // =====================================================
    // SEARCH + CATEGORY
    // =====================================================

    if (!searchText.trimmed().isEmpty())
    {
        databaseBooks =
            Database::instance().searchBooks(
                searchText.trimmed()
                );


        if (category != "All Categories")
        {
            QList<::Book> filtered;


            for (const ::Book &databaseBook :
                 databaseBooks)
            {
                if (databaseBook.category.compare(
                        category,
                        Qt::CaseInsensitive
                        ) == 0)
                {
                    filtered.append(
                        databaseBook
                        );
                }
            }


            databaseBooks =
                filtered;
        }
    }
    else if (category != "All Categories")
    {
        databaseBooks =
            Database::instance().getBooksByCategory(
                category
                );
    }
    else
    {
        databaseBooks =
            Database::instance().getRecentBooks(
                50
                );
    }


    // =====================================================
    // CONVERT DATABASE BOOKS
    // =====================================================

    for (const ::Book &databaseBook :
         databaseBooks)
    {
        Book item;


        item.id =
            databaseBook.id;


        item.title =
            databaseBook.title;


        item.author =
            databaseBook.author;


        item.condition =
            databaseBook.condition;


        item.price =
            QString("₹%1")
                .arg(
                    databaseBook.price,
                    0,
                    'f',
                    0
                    );


        item.location =
            databaseBook.location;


        item.category =
            databaseBook.category;


        // IMPORTANT
        // Preserve image path while filtering

        item.imagePath =
            databaseBook.imagePath;


        books.append(
            item
            );
    }


    // =====================================================
    // NO RESULTS
    // =====================================================

    if (books.isEmpty())
    {
        resultLabel->setText(
            "No books found"
            );


        QLabel *emptyLabel =
            new QLabel(
                "No books match your search.\n\n"
                "Try another title, author, ISBN or category."
                );


        emptyLabel->setAlignment(
            Qt::AlignCenter
            );

        emptyLabel->setMinimumHeight(
            220
            );

        emptyLabel->setStyleSheet(R"(

            QLabel {
                background-color: white;
                border: 1px solid #E5E7EB;
                border-radius: 15px;
                color: #6B7280;
                font-size: 14px;
                font-weight: 600;
                padding: 30px;
            }

        )");


        booksGrid->addWidget(
            emptyLabel,
            0,
            0,
            1,
            4
            );

        return;
    }


    // =====================================================
    // RESULT COUNT
    // =====================================================

    resultLabel->setText(
        QString("%1 book%2 found")
            .arg(books.size())
            .arg(
                books.size() == 1
                    ? ""
                    : "s"
                )
        );


    // =====================================================
    // DISPLAY
    // =====================================================

    int columns = 4;


    for (int i = 0;
         i < books.size();
         ++i)
    {
        const Book &item =
            books[i];


        QFrame *card =
            createBookCard(
                item.id,
                item.title,
                item.author,
                item.condition,
                item.price,
                item.location,
                item.imagePath
                );


        int row =
            i / columns;


        int column =
            i % columns;


        booksGrid->addWidget(
            card,
            row,
            column
            );
    }


    for (int i = 0;
         i < columns;
         ++i)
    {
        booksGrid->setColumnStretch(
            i,
            1
            );
    }
}


// =========================================================
// BOOK CARD
// =========================================================

QFrame* BrowseWindow::createBookCard(
    int bookId,
    const QString &title,
    const QString &author,
    const QString &condition,
    const QString &price,
    const QString &location,
    const QString &imagePath
    )
{
    QFrame *card =
        new QFrame();


    card->setMinimumHeight(
        355
        );


    card->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed
        );


    card->setStyleSheet(R"(

        QFrame {
            background-color: white;
            border: 1px solid #E5E7EB;
            border-radius: 16px;
        }

        QFrame:hover {
            border-color: #C7D2FE;
            background-color: #FEFEFF;
        }

    )");


    QVBoxLayout *layout =
        new QVBoxLayout(card);


    layout->setContentsMargins(
        12,
        12,
        12,
        12
        );

    layout->setSpacing(
        7
        );


    // =====================================================
    // COVER
    // =====================================================

    QLabel *cover =
        new QLabel();


    cover->setFixedHeight(
        155
        );


    cover->setAlignment(
        Qt::AlignCenter
        );


    // =====================================================
    // IMAGE PATH
    // =====================================================

    QString actualImagePath =
        imagePath.trimmed();


    bool imageLoaded = false;


    if (!actualImagePath.isEmpty())
    {
        QFileInfo fileInfo(
            actualImagePath
            );


        // -------------------------------------------------
        // Direct path
        // -------------------------------------------------

        if (fileInfo.exists() &&
            fileInfo.isFile())
        {
            QPixmap pixmap(
                actualImagePath
                );


            if (!pixmap.isNull())
            {
                cover->setPixmap(
                    pixmap.scaled(
                        150,
                        145,
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation
                        )
                    );


                imageLoaded = true;
            }
        }


        // -------------------------------------------------
        // Try absolute path
        // -------------------------------------------------

        if (!imageLoaded)
        {
            QString absolutePath =
                QDir::fromNativeSeparators(
                    actualImagePath
                    );


            QFileInfo absoluteInfo(
                absolutePath
                );


            if (absoluteInfo.exists() &&
                absoluteInfo.isFile())
            {
                QPixmap pixmap(
                    absolutePath
                    );


                if (!pixmap.isNull())
                {
                    cover->setPixmap(
                        pixmap.scaled(
                            150,
                            145,
                            Qt::KeepAspectRatio,
                            Qt::SmoothTransformation
                            )
                        );


                    imageLoaded = true;
                }
            }
        }


        if (!imageLoaded)
        {
            qDebug()
            << "Image not found:"
            << actualImagePath;
        }
    }


    // =====================================================
    // FALLBACK
    // =====================================================

    if (!imageLoaded)
    {
        QString initials =
            title.left(2).toUpper();


        if (initials.isEmpty())
        {
            initials = "BK";
        }


        cover->setText(
            initials
            );


        cover->setStyleSheet(R"(

            QLabel {
                background-color: #EEF2FF;
                color: #4F46E5;
                border-radius: 11px;
                font-size: 34px;
                font-weight: 900;
            }

        )");
    }
    else
    {
        cover->setStyleSheet(R"(

            QLabel {
                background-color: #F8F9FC;
                border-radius: 11px;
            }

        )");
    }


    layout->addWidget(
        cover
        );


    // =====================================================
    // TITLE
    // =====================================================

    QLabel *titleLabel =
        new QLabel(
            title
            );


    titleLabel->setWordWrap(
        true
        );


    titleLabel->setMaximumHeight(
        42
        );


    titleLabel->setStyleSheet(R"(

        QLabel {
            color: #172033;
            font-size: 14px;
            font-weight: 800;
        }

    )");


    layout->addWidget(
        titleLabel
        );


    // =====================================================
    // AUTHOR
    // =====================================================

    QLabel *authorLabel =
        new QLabel(
            author.isEmpty()
                ? "by Unknown Author"
                : "by " + author
            );


    authorLabel->setStyleSheet(R"(

        QLabel {
            color: #6B7280;
            font-size: 11px;
        }

    )");


    layout->addWidget(
        authorLabel
        );


    // =====================================================
    // CONDITION
    // =====================================================

    QLabel *conditionLabel =
        new QLabel(
            condition.isEmpty()
                ? "Not specified"
                : condition
            );


    conditionLabel->setAlignment(
        Qt::AlignCenter
        );


    conditionLabel->setMaximumWidth(
        110
        );


    conditionLabel->setStyleSheet(R"(

        QLabel {
            background-color: #ECFDF5;
            color: #047857;
            border-radius: 6px;
            padding: 4px 8px;
            font-size: 10px;
            font-weight: 800;
        }

    )");


    layout->addWidget(
        conditionLabel,
        0,
        Qt::AlignLeft
        );


    // =====================================================
    // LOCATION
    // =====================================================

    QLabel *locationLabel =
        new QLabel(
            "Location  •  " +
            (
                location.isEmpty()
                    ? "Not specified"
                    : location
                )
            );


    locationLabel->setStyleSheet(R"(

        QLabel {
            color: #9CA3AF;
            font-size: 10px;
        }

    )");


    layout->addWidget(
        locationLabel
        );


    layout->addStretch();


    // =====================================================
    // BOTTOM
    // =====================================================

    QHBoxLayout *bottom =
        new QHBoxLayout();


    QLabel *priceLabel =
        new QLabel(
            price
            );


    priceLabel->setStyleSheet(R"(

        QLabel {
            color: #111827;
            font-size: 18px;
            font-weight: 900;
        }

    )");


    bottom->addWidget(
        priceLabel
        );


    bottom->addStretch();


    // =====================================================
    // VIEW BUTTON
    // =====================================================

    QPushButton *viewButton =
        new QPushButton(
            "View Book"
            );


    viewButton->setMinimumHeight(
        34
        );


    viewButton->setStyleSheet(R"(

        QPushButton {
            background-color: #4F46E5;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 0 13px;
            font-size: 11px;
            font-weight: 800;
        }

        QPushButton:hover {
            background-color: #4338CA;
        }

        QPushButton:pressed {
            background-color: #3730A3;
        }

    )");


    bottom->addWidget(
        viewButton
        );


    layout->addLayout(
        bottom
        );


    // =====================================================
    // VIEW BOOK
    // =====================================================

    connect(
        viewButton,
        &QPushButton::clicked,
        this,
        [this, bookId]()
        {
            emit bookSelected(
                bookId
                );
        }
        );


    return card;
}


// =========================================================
// PRIMARY BUTTON
// =========================================================

QPushButton*
BrowseWindow::createPrimaryButton(
    const QString &text
    )
{
    QPushButton *button =
        new QPushButton(
            text
            );


    button->setMinimumHeight(
        42
        );


    button->setStyleSheet(R"(

        QPushButton {
            background-color: #5B5FEF;
            color: white;
            border: none;
            border-radius: 9px;
            padding: 0 20px;
            font-size: 13px;
            font-weight: 800;
        }

        QPushButton:hover {
            background-color: #4F46E5;
        }

        QPushButton:pressed {
            background-color: #4338CA;
        }

    )");


    return button;
}


// =========================================================
// SECONDARY BUTTON
// =========================================================

QPushButton*
BrowseWindow::createSecondaryButton(
    const QString &text
    )
{
    QPushButton *button =
        new QPushButton(
            text
            );


    button->setMinimumHeight(
        42
        );


    button->setStyleSheet(R"(

        QPushButton {
            background-color: white;
            color: #374151;
            border: 1px solid #E1E4EA;
            border-radius: 9px;
            padding: 0 20px;
            font-size: 13px;
            font-weight: 700;
        }

        QPushButton:hover {
            background-color: #F3F4FF;
            color: #4F46E5;
            border-color: #C7D2FE;
        }

    )");


    return button;
}


// =========================================================
// SECTION TITLE
// =========================================================

QLabel*
BrowseWindow::createSectionTitle(
    const QString &text
    )
{
    QLabel *label =
        new QLabel(
            text
            );


    label->setStyleSheet(R"(

        QLabel {
            color: #172033;
            font-size: 24px;
            font-weight: 800;
        }

    )");


    return label;
}


// =========================================================
// SEARCH
// =========================================================

void BrowseWindow::handleSearch()
{
    QString searchText =
        searchEdit->text().trimmed();


    QString category =
        categoryCombo->currentText();


    filterBooks(
        searchText,
        category
        );
}


// =========================================================
// CATEGORY
// =========================================================

void BrowseWindow::handleCategoryChanged(
    const QString &category
    )
{
    QString searchText =
        searchEdit->text().trimmed();


    filterBooks(
        searchText,
        category
        );
}


// =========================================================
// BACK
// =========================================================

void BrowseWindow::handleBack()
{
    emit backRequested();
}