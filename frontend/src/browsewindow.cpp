#include "browsewindow.h"
#include "network/ApiClient.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QMessageBox>
#include <QSizePolicy>
#include <QPixmap>
#include <QFileInfo>
#include <QFont>
#include <QPainter>
#include <QLinearGradient>
#include <QUrlQuery>
#include <QNetworkReply>

BrowseWindow::BrowseWindow(const QString &userName, QWidget *parent)
    : QWidget(parent),
    userName(userName),
    searchEdit(nullptr),
    searchButton(nullptr),
    categoryCombo(nullptr),
    conditionCombo(nullptr),
    sortCombo(nullptr),
    backButton(nullptr),
    prevPageButton(nullptr),
    nextPageButton(nullptr),
    pageLabel(nullptr),
    booksGrid(nullptr),
    booksContainer(nullptr),
    resultLabel(nullptr),
    currentPage(1),
    totalPages(1)
{
    setWindowTitle("BookBazzar - Browse Books");
    resize(1400, 850);
    setMinimumSize(1000, 650);

    setupUI();
    fetchBooks();
}

BrowseWindow::~BrowseWindow()
{
}

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
    )");

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *page = new QWidget();
    page->setStyleSheet("background-color: #F7F8FC;");

    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 40);
    pageLayout->setSpacing(20);

    pageLayout->addWidget(createTopBar());
    pageLayout->addWidget(createFilterBar());
    pageLayout->addWidget(createBooksSection());
    pageLayout->addWidget(createPaginationBar());

    scrollArea->setWidget(page);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(scrollArea);
}

QWidget* BrowseWindow::createTopBar()
{
    QFrame *topBar = new QFrame();
    topBar->setFixedHeight(76);
    topBar->setStyleSheet("background-color: white; border-bottom: 1px solid #E6E8EF;");

    QHBoxLayout *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(32, 10, 32, 10);
    layout->setSpacing(15);

    backButton = new QPushButton("← Back to Home");
    backButton->setMinimumHeight(40);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: 1px solid #E5E7EB;
            color: #374151;
            padding: 0 16px;
            font-size: 13px;
            font-weight: 700;
            border-radius: 8px;
        }
        QPushButton:hover {
            background-color: #F3F4F6;
        }
    )");

    QLabel *pageTitle = new QLabel("Browse Catalog");
    pageTitle->setStyleSheet("color: #172033; font-size: 20px; font-weight: 800;");

    layout->addWidget(backButton);
    layout->addWidget(pageTitle);
    layout->addStretch();

    connect(backButton, &QPushButton::clicked, this, &BrowseWindow::handleBack);

    return topBar;
}

QWidget* BrowseWindow::createFilterBar()
{
    QFrame *filterBar = new QFrame();
    filterBar->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border: 1px solid #E5E7EB;
            border-radius: 14px;
        }
    )");

    QVBoxLayout *containerLayout = new QVBoxLayout(filterBar);
    containerLayout->setContentsMargins(24, 16, 24, 16);
    containerLayout->setSpacing(12);

    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(12);

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search by title, author, or ISBN...");
    searchEdit->setMinimumHeight(42);
    searchEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #F9FAFB;
            border: 1px solid #D1D5DB;
            border-radius: 8px;
            padding: 0 14px;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: #4F46E5;
            background-color: white;
        }
    )");

    searchButton = new QPushButton("Search");
    searchButton->setMinimumHeight(42);
    searchButton->setMinimumWidth(100);
    searchButton->setCursor(Qt::PointingHandCursor);
    searchButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4F46E5;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: 700;
        }
        QPushButton:hover {
            background-color: #4338CA;
        }
    )");

    topRow->addWidget(searchEdit, 1);
    topRow->addWidget(searchButton);
    containerLayout->addLayout(topRow);

    QHBoxLayout *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(12);

    categoryCombo = new QComboBox();
    categoryCombo->setMinimumHeight(38);
    categoryCombo->addItems({"All Categories", "Fiction", "Non-Fiction", "Academic", "Programming", "Engineering", "Competitive Exams", "School", "Novels", "Other"});

    conditionCombo = new QComboBox();
    conditionCombo->setMinimumHeight(38);
    conditionCombo->addItems({"All Conditions", "New", "Like New", "Very Good", "Good", "Acceptable"});

    sortCombo = new QComboBox();
    sortCombo->setMinimumHeight(38);
    sortCombo->addItem("Newest First", "newest");
    sortCombo->addItem("Oldest First", "oldest");
    sortCombo->addItem("Price: Low to High", "price_asc");
    sortCombo->addItem("Price: High to Low", "price_desc");

    QString comboStyle = R"(
        QComboBox {
            background-color: #F9FAFB;
            border: 1px solid #D1D5DB;
            border-radius: 8px;
            padding: 0 12px;
            font-size: 13px;
            color: #374151;
            font-weight: 600;
            min-width: 140px;
        }
        QComboBox::drop-down {
            border: none;
        }
    )";
    categoryCombo->setStyleSheet(comboStyle);
    conditionCombo->setStyleSheet(comboStyle);
    sortCombo->setStyleSheet(comboStyle);

    bottomRow->addWidget(new QLabel("Category:"));
    bottomRow->addWidget(categoryCombo);
    bottomRow->addSpacing(10);
    bottomRow->addWidget(new QLabel("Condition:"));
    bottomRow->addWidget(conditionCombo);
    bottomRow->addSpacing(10);
    bottomRow->addWidget(new QLabel("Sort:"));
    bottomRow->addWidget(sortCombo);
    bottomRow->addStretch();

    containerLayout->addLayout(bottomRow);

    connect(searchButton, &QPushButton::clicked, this, &BrowseWindow::handleSearch);
    connect(searchEdit, &QLineEdit::returnPressed, this, &BrowseWindow::handleSearch);
    connect(categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BrowseWindow::handleFilterChanged);
    connect(conditionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BrowseWindow::handleFilterChanged);
    connect(sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BrowseWindow::handleFilterChanged);

    QWidget *wrapper = new QWidget();
    QVBoxLayout *wLayout = new QVBoxLayout(wrapper);
    wLayout->setContentsMargins(32, 0, 32, 0);
    wLayout->addWidget(filterBar);

    return wrapper;
}

QWidget* BrowseWindow::createBooksSection()
{
    QWidget *section = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(32, 0, 32, 0);
    layout->setSpacing(14);

    resultLabel = new QLabel("Loading books...");
    resultLabel->setStyleSheet("color: #4B5563; font-size: 14px; font-weight: 700;");
    layout->addWidget(resultLabel);

    booksContainer = new QWidget();
    booksGrid = new QGridLayout(booksContainer);
    booksGrid->setContentsMargins(0, 0, 0, 0);
    booksGrid->setSpacing(16);

    layout->addWidget(booksContainer);
    return section;
}

QWidget* BrowseWindow::createPaginationBar()
{
    QWidget *bar = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(32, 10, 32, 10);
    layout->setSpacing(16);

    prevPageButton = new QPushButton("← Previous Page");
    prevPageButton->setMinimumHeight(38);
    prevPageButton->setCursor(Qt::PointingHandCursor);
    prevPageButton->setStyleSheet(R"(
        QPushButton {
            background-color: white;
            border: 1px solid #D1D5DB;
            border-radius: 8px;
            color: #374151;
            padding: 0 16px;
            font-size: 13px;
            font-weight: 700;
        }
        QPushButton:hover { background-color: #F3F4F6; }
        QPushButton:disabled { color: #9CA3AF; border-color: #E5E7EB; }
    )");

    pageLabel = new QLabel("Page 1 of 1");
    pageLabel->setStyleSheet("color: #374151; font-size: 13px; font-weight: 700;");

    nextPageButton = new QPushButton("Next Page →");
    nextPageButton->setMinimumHeight(38);
    nextPageButton->setCursor(Qt::PointingHandCursor);
    nextPageButton->setStyleSheet(R"(
        QPushButton {
            background-color: white;
            border: 1px solid #D1D5DB;
            border-radius: 8px;
            color: #374151;
            padding: 0 16px;
            font-size: 13px;
            font-weight: 700;
        }
        QPushButton:hover { background-color: #F3F4F6; }
        QPushButton:disabled { color: #9CA3AF; border-color: #E5E7EB; }
    )");

    layout->addStretch();
    layout->addWidget(prevPageButton);
    layout->addWidget(pageLabel);
    layout->addWidget(nextPageButton);
    layout->addStretch();

    connect(prevPageButton, &QPushButton::clicked, this, &BrowseWindow::handlePrevPage);
    connect(nextPageButton, &QPushButton::clicked, this, &BrowseWindow::handleNextPage);

    return bar;
}

void BrowseWindow::clearBooksGrid()
{
    if (!booksGrid) return;
    QLayoutItem *item;
    while ((item = booksGrid->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

void BrowseWindow::fetchBooks()
{
    clearBooksGrid();
    resultLabel->setText("Searching catalog...");

    QUrlQuery query;
    query.addQueryItem("page", QString::number(currentPage));
    query.addQueryItem("limit", "12");

    const QString search = searchEdit ? searchEdit->text().trimmed() : QString();
    if (!search.isEmpty()) {
        query.addQueryItem("search", search);
    }

    if (categoryCombo && categoryCombo->currentIndex() > 0) {
        query.addQueryItem("category", categoryCombo->currentText());
    }

    if (conditionCombo && conditionCombo->currentIndex() > 0) {
        query.addQueryItem("condition", conditionCombo->currentText());
    }

    if (sortCombo) {
        query.addQueryItem("sort", sortCombo->currentData().toString());
    }

    auto *reply = ApiClient::instance().getPublic(QStringLiteral("/api/books"), query);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        clearBooksGrid();
        books.clear();

        if (!ok || !json.contains(QStringLiteral("books"))) {
            resultLabel->setText("Error loading books.");
            return;
        }

        const auto arr = json[QStringLiteral("books")].toArray();
        for (const auto &v : arr) {
            books.append(BookModel::fromJson(v.toObject()));
        }

        if (json.contains(QStringLiteral("pagination"))) {
            const auto meta = PaginationMeta::fromJson(json[QStringLiteral("pagination")].toObject());
            currentPage = meta.page;
            totalPages = qMax(1, meta.totalPages);
            prevPageButton->setEnabled(meta.hasPrevPage);
            nextPageButton->setEnabled(meta.hasNextPage);
            pageLabel->setText(QString("Page %1 of %2 (%3 total books)").arg(currentPage).arg(totalPages).arg(meta.total));
        }

        if (books.isEmpty()) {
            resultLabel->setText("No books found matching your criteria.");
            QLabel *empty = new QLabel("No books found.\nTry broadening your search or filter.");
            empty->setAlignment(Qt::AlignCenter);
            empty->setStyleSheet("background-color: white; border: 1px solid #E5E7EB; border-radius: 12px; color: #6B7280; font-size: 14px; padding: 40px;");
            booksGrid->addWidget(empty, 0, 0, 1, 4);
            return;
        }

        resultLabel->setText(QString("Showing %1 books").arg(books.size()));

        const int columns = 4;
        for (int i = 0; i < books.size(); ++i) {
            QFrame *card = createBookCard(books[i]);
            booksGrid->addWidget(card, i / columns, i % columns);
        }

        for (int i = 0; i < columns; ++i) {
            booksGrid->setColumnStretch(i, 1);
        }
    });
}

QFrame* BrowseWindow::createBookCard(const BookModel &book)
{
    QFrame *card = new QFrame();
    card->setMinimumHeight(355);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
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

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(7);

    QLabel *cover = new QLabel();
    cover->setFixedHeight(155);
    cover->setAlignment(Qt::AlignCenter);

    bool loaded = false;
    if (!book.coverImage.trimmed().isEmpty()) {
        QFileInfo fi(book.coverImage);
        if (fi.exists() && fi.isFile()) {
            QPixmap p(book.coverImage);
            if (!p.isNull()) {
                cover->setPixmap(p.scaled(cover->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                loaded = true;
            }
        }
    }

    if (!loaded) {
        // Procedural gradient book cover
        QPixmap pix(130, 155);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setRenderHint(QPainter::Antialiasing);

        QLinearGradient grad(0, 0, 130, 155);
        grad.setColorAt(0, QColor(79, 70, 229));
        grad.setColorAt(1, QColor(124, 58, 237));
        painter.setBrush(grad);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(0, 0, 130, 155, 8, 8);

        painter.setPen(Qt::white);
        QFont f("Segoe UI", 9, QFont::Bold);
        painter.setFont(f);
        painter.drawText(QRect(8, 20, 114, 70), Qt::AlignCenter | Qt::TextWordWrap, book.title);

        QFont f2("Segoe UI", 8);
        painter.setFont(f2);
        painter.drawText(QRect(8, 100, 114, 30), Qt::AlignCenter | Qt::TextWordWrap, book.author);

        painter.end();
        cover->setPixmap(pix);
    }
    layout->addWidget(cover);

    QLabel *titleLabel = new QLabel(book.title);
    titleLabel->setWordWrap(true);
    titleLabel->setMaximumHeight(42);
    titleLabel->setStyleSheet("color: #172033; font-size: 14px; font-weight: 800;");
    layout->addWidget(titleLabel);

    QLabel *authorLabel = new QLabel("by " + (book.author.isEmpty() ? "Unknown" : book.author));
    authorLabel->setStyleSheet("color: #6B7280; font-size: 11px;");
    layout->addWidget(authorLabel);

    QLabel *conditionLabel = new QLabel(book.condition.isEmpty() ? "Good" : book.condition);
    conditionLabel->setAlignment(Qt::AlignCenter);
    conditionLabel->setMaximumWidth(90);
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
    layout->addWidget(conditionLabel, 0, Qt::AlignLeft);

    QLabel *catLabel = new QLabel("Category  •  " + (book.category.isEmpty() ? "General" : book.category));
    catLabel->setStyleSheet("color: #9CA3AF; font-size: 10px;");
    layout->addWidget(catLabel);

    layout->addStretch();

    QHBoxLayout *bottom = new QHBoxLayout();
    QLabel *priceLabel = new QLabel(QString("₹%1").arg(book.price, 0, 'f', 0));
    priceLabel->setStyleSheet("color: #111827; font-size: 18px; font-weight: 900;");
    bottom->addWidget(priceLabel);
    bottom->addStretch();

    QPushButton *viewButton = new QPushButton("View Book");
    viewButton->setMinimumHeight(34);
    viewButton->setCursor(Qt::PointingHandCursor);
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
        QPushButton:hover { background-color: #4338CA; }
    )");

    const QString bId = book.id;
    connect(viewButton, &QPushButton::clicked, this, [this, bId]() {
        emit bookSelected(bId);
    });

    bottom->addWidget(viewButton);
    layout->addLayout(bottom);

    return card;
}

void BrowseWindow::handleSearch()
{
    currentPage = 1;
    fetchBooks();
}

void BrowseWindow::handleFilterChanged()
{
    currentPage = 1;
    fetchBooks();
}

void BrowseWindow::handlePrevPage()
{
    if (currentPage > 1) {
        currentPage--;
        fetchBooks();
    }
}

void BrowseWindow::handleNextPage()
{
    if (currentPage < totalPages) {
        currentPage++;
        fetchBooks();
    }
}

void BrowseWindow::handleBack()
{
    emit backRequested();
}