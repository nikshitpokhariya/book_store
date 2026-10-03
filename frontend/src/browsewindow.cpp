#include "browsewindow.h"
#include "network/ApiClient.h"
#include "AppStyle.h"
#include "ImageLoader.h"

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
#include <QPixmap>
#include <QFileInfo>
#include <QFont>
#include <QPainter>
#include <QLinearGradient>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QDoubleSpinBox>
#include <QTimer>
#include <QListWidget>

BrowseWindow::BrowseWindow(const QString &userName,
                           const QString &initialCategory,
                           const QString &initialSearch,
                           QWidget *parent)
    : QWidget(parent),
    userName(userName),
    searchEdit(nullptr),
    searchButton(nullptr),
    suggestionsList(nullptr),
    debounceTimer(nullptr),
    categoryCombo(nullptr),
    conditionCombo(nullptr),
    sortCombo(nullptr),
    minPriceSpin(nullptr),
    maxPriceSpin(nullptr),
    resetFilterButton(nullptr),
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
    resize(1380, 860);
    setMinimumSize(1020, 680);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    setupUI();

    if (!initialSearch.isEmpty() && searchEdit) {
        searchEdit->setText(initialSearch);
    }

    if (!initialCategory.isEmpty() && categoryCombo) {
        int idx = categoryCombo->findText(initialCategory, Qt::MatchFixedString);
        if (idx != -1) {
            categoryCombo->setCurrentIndex(idx);
        }
    }

    fetchBooks();
}

BrowseWindow::~BrowseWindow()
{
}

void BrowseWindow::setupUI()
{
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QString(R"(
        QScrollArea {
            background-color: %1;
            border: none;
        }
        %2
    )").arg(AppStyle::Background, AppStyle::scrollBarStyle()));

    QWidget *page = new QWidget();
    page->setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 48);
    pageLayout->setSpacing(24);

    pageLayout->addWidget(createTopBar());
    pageLayout->addWidget(createFilterBar());
    pageLayout->addWidget(createBooksSection());
    pageLayout->addWidget(createPaginationBar());

    scrollArea->setWidget(page);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(scrollArea);
}

QWidget* BrowseWindow::createTopBar()
{
    QFrame *topBar = new QFrame();
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

    QHBoxLayout *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(36, 10, 36, 10);
    layout->setSpacing(16);

    backButton = new QPushButton("← Back to Home");
    backButton->setMinimumHeight(38);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setStyleSheet(AppStyle::secondaryButtonStyle());

    QLabel *pageTitle = new QLabel("Browse Book Catalog");
    pageTitle->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: 800;").arg(AppStyle::TextPrimary));

    layout->addWidget(backButton);
    layout->addWidget(pageTitle);
    layout->addStretch();

    connect(backButton, &QPushButton::clicked, this, &BrowseWindow::handleBack);
    return topBar;
}

QWidget* BrowseWindow::createFilterBar()
{
    QFrame *filterBar = new QFrame();
    filterBar->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(filterBar, 16, 4, 12);

    QVBoxLayout *containerLayout = new QVBoxLayout(filterBar);
    containerLayout->setContentsMargins(24, 20, 24, 20);
    containerLayout->setSpacing(16);

    // Search row
    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(10);

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search by book title, author, or ISBN...");
    searchEdit->setMinimumHeight(44);
    searchEdit->setStyleSheet(AppStyle::inputStyle());

    searchButton = new QPushButton("Search");
    searchButton->setMinimumHeight(44);
    searchButton->setMinimumWidth(110);
    searchButton->setCursor(Qt::PointingHandCursor);
    searchButton->setStyleSheet(AppStyle::primaryButtonStyle());

    topRow->addWidget(searchEdit, 1);
    topRow->addWidget(searchButton);
    containerLayout->addLayout(topRow);

    debounceTimer = new QTimer(this);
    debounceTimer->setSingleShot(true);
    debounceTimer->setInterval(350);
    connect(debounceTimer, &QTimer::timeout, this, &BrowseWindow::fetchSuggestions);
    connect(searchEdit, &QLineEdit::textChanged, this, &BrowseWindow::handleSearchTextChanged);

    suggestionsList = new QListWidget(filterBar);
    suggestionsList->setVisible(false);
    suggestionsList->setMaximumHeight(150);
    suggestionsList->setStyleSheet(QString(R"(
        QListWidget {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 8px;
            padding: 4px;
            font-size: 13px;
            color: %2;
        }
        QListWidget::item {
            padding: 8px 12px;
            border-radius: 6px;
        }
        QListWidget::item:hover {
            background-color: %3;
            color: %4;
        }
    )").arg(AppStyle::BorderSubtle, AppStyle::TextPrimary, AppStyle::PrimaryLight, AppStyle::Primary));

    connect(suggestionsList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (item) {
            searchEdit->setText(item->text());
            suggestionsList->clear();
            suggestionsList->setVisible(false);
            handleSearch();
        }
    });

    containerLayout->addWidget(suggestionsList);

    // Filter row 1: Category, Condition, Sort
    QHBoxLayout *filterRow1 = new QHBoxLayout();
    filterRow1->setSpacing(16);

    QLabel *catLbl = new QLabel("Category:");
    catLbl->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600;").arg(AppStyle::TextSecondary));
    categoryCombo = new QComboBox();
    categoryCombo->setStyleSheet(AppStyle::comboBoxStyle());
    categoryCombo->addItems({"All Categories", "Academic", "Programming", "Engineering", "Fiction", "Competitive Exams", "School", "Novels", "Non-Fiction", "Other"});

    QLabel *condLbl = new QLabel("Condition:");
    condLbl->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600;").arg(AppStyle::TextSecondary));
    conditionCombo = new QComboBox();
    conditionCombo->setStyleSheet(AppStyle::comboBoxStyle());
    conditionCombo->addItems({"All Conditions", "New", "Like New", "Very Good", "Good", "Acceptable"});

    QLabel *sortLbl = new QLabel("Sort:");
    sortLbl->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600;").arg(AppStyle::TextSecondary));
    sortCombo = new QComboBox();
    sortCombo->setStyleSheet(AppStyle::comboBoxStyle());
    sortCombo->addItem("Newest First", "newest");
    sortCombo->addItem("Oldest First", "oldest");
    sortCombo->addItem("Price: Low to High", "price_asc");
    sortCombo->addItem("Price: High to Low", "price_desc");

    filterRow1->addWidget(catLbl);
    filterRow1->addWidget(categoryCombo);
    filterRow1->addSpacing(8);
    filterRow1->addWidget(condLbl);
    filterRow1->addWidget(conditionCombo);
    filterRow1->addSpacing(8);
    filterRow1->addWidget(sortLbl);
    filterRow1->addWidget(sortCombo);
    filterRow1->addStretch();
    containerLayout->addLayout(filterRow1);

    // Filter row 2: Price range (minPrice & maxPrice) + Reset
    QHBoxLayout *filterRow2 = new QHBoxLayout();
    filterRow2->setSpacing(16);

    QLabel *priceRangeLbl = new QLabel("Price (₹):");
    priceRangeLbl->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600;").arg(AppStyle::TextSecondary));

    QLabel *minLbl = new QLabel("Min:");
    minLbl->setStyleSheet(QString("color: %1; font-size: 12px;").arg(AppStyle::TextMuted));
    minPriceSpin = new QDoubleSpinBox();
    minPriceSpin->setRange(0, 50000);
    minPriceSpin->setPrefix("₹");
    minPriceSpin->setDecimals(0);
    minPriceSpin->setSingleStep(50);
    minPriceSpin->setStyleSheet(AppStyle::inputStyle());
    minPriceSpin->setMinimumWidth(110);
    minPriceSpin->setMinimumHeight(36);

    QLabel *maxLbl = new QLabel("Max:");
    maxLbl->setStyleSheet(QString("color: %1; font-size: 12px;").arg(AppStyle::TextMuted));
    maxPriceSpin = new QDoubleSpinBox();
    maxPriceSpin->setRange(0, 50000);
    maxPriceSpin->setPrefix("₹");
    maxPriceSpin->setDecimals(0);
    maxPriceSpin->setSingleStep(50);
    maxPriceSpin->setStyleSheet(AppStyle::inputStyle());
    maxPriceSpin->setMinimumWidth(110);
    maxPriceSpin->setMinimumHeight(36);

    resetFilterButton = new QPushButton("Clear All Filters");
    resetFilterButton->setCursor(Qt::PointingHandCursor);
    resetFilterButton->setStyleSheet(AppStyle::ghostButtonStyle());
    resetFilterButton->setMinimumHeight(36);

    filterRow2->addWidget(priceRangeLbl);
    filterRow2->addWidget(minLbl);
    filterRow2->addWidget(minPriceSpin);
    filterRow2->addWidget(maxLbl);
    filterRow2->addWidget(maxPriceSpin);
    filterRow2->addSpacing(12);
    filterRow2->addWidget(resetFilterButton);
    filterRow2->addStretch();
    containerLayout->addLayout(filterRow2);

    connect(searchButton, &QPushButton::clicked, this, &BrowseWindow::handleSearch);
    connect(searchEdit, &QLineEdit::returnPressed, this, &BrowseWindow::handleSearch);
    connect(categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BrowseWindow::handleFilterChanged);
    connect(conditionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BrowseWindow::handleFilterChanged);
    connect(sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BrowseWindow::handleFilterChanged);
    connect(minPriceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &BrowseWindow::handleFilterChanged);
    connect(maxPriceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &BrowseWindow::handleFilterChanged);
    connect(resetFilterButton, &QPushButton::clicked, this, &BrowseWindow::handleResetFilters);

    QWidget *wrapper = new QWidget();
    QVBoxLayout *wLayout = new QVBoxLayout(wrapper);
    wLayout->setContentsMargins(36, 0, 36, 0);
    wLayout->addWidget(filterBar);
    return wrapper;
}

QWidget* BrowseWindow::createBooksSection()
{
    QWidget *section = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(36, 0, 36, 0);
    layout->setSpacing(16);

    resultLabel = new QLabel("Searching available listings...");
    resultLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 700;").arg(AppStyle::TextPrimary));
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
    layout->setContentsMargins(36, 12, 36, 12);
    layout->setSpacing(16);

    prevPageButton = new QPushButton("← Previous Page");
    prevPageButton->setMinimumHeight(38);
    prevPageButton->setCursor(Qt::PointingHandCursor);
    prevPageButton->setStyleSheet(AppStyle::secondaryButtonStyle());

    pageLabel = new QLabel("Page 1 of 1");
    pageLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 700;").arg(AppStyle::TextPrimary));

    nextPageButton = new QPushButton("Next Page →");
    nextPageButton->setMinimumHeight(38);
    nextPageButton->setCursor(Qt::PointingHandCursor);
    nextPageButton->setStyleSheet(AppStyle::secondaryButtonStyle());

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
        if (item->widget()) delete item->widget();
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

    if (minPriceSpin && minPriceSpin->value() > 0.0) {
        query.addQueryItem("minPrice", QString::number(minPriceSpin->value(), 'f', 0));
    }

    if (maxPriceSpin && maxPriceSpin->value() > 0.0) {
        query.addQueryItem("maxPrice", QString::number(maxPriceSpin->value(), 'f', 0));
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
            resultLabel->setText("No books match your criteria.");
            QLabel *empty = new QLabel("No books found.\nTry broadening your search or adjusting filters.");
            empty->setAlignment(Qt::AlignCenter);
            empty->setStyleSheet(QString("background-color: #FFFFFF; border: 1px solid %1; border-radius: 12px; color: %2; font-size: 14px; padding: 48px;").arg(AppStyle::BorderSubtle, AppStyle::TextSecondary));
            booksGrid->addWidget(empty, 0, 0, 1, 4);
            return;
        }

        resultLabel->setText(QString("Showing %1 book%2").arg(books.size()).arg(books.size() == 1 ? "" : "s"));

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
    card->setFixedSize(280, 410);
    card->setObjectName("bookCard");
    card->setStyleSheet(QString(R"(
        QFrame#bookCard {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 14px;
        }
        QFrame#bookCard:hover {
            border-color: %2;
        }
        QLabel {
            border: none;
            background: transparent;
        }
    )").arg(AppStyle::BorderSubtle, AppStyle::PrimaryBorder));
    AppStyle::applyElevation(card, 16, 4, 15);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(6);

    // Image Container - Large, prominent thumbnail
    QFrame *imgBox = new QFrame(card);
    imgBox->setFixedHeight(220);
    imgBox->setStyleSheet(QString(R"(
        QFrame {
            background-color: %1;
            border-radius: 10px;
        }
    )").arg(AppStyle::SurfaceSubtle));

    QVBoxLayout *imgBoxLayout = new QVBoxLayout(imgBox);
    imgBoxLayout->setContentsMargins(6, 6, 6, 6);
    imgBoxLayout->setSpacing(4);

    // Badges Row at top of image
    QHBoxLayout *badgesRow = new QHBoxLayout();
    badgesRow->setSpacing(6);

    QLabel *catBadge = new QLabel(book.category.isEmpty() ? "Book" : book.category, imgBox);
    catBadge->setStyleSheet(QString(R"(
        background-color: rgba(255, 255, 255, 0.92);
        color: %1;
        font-size: 11px;
        font-weight: 700;
        border-radius: 6px;
        padding: 3px 8px;
    )").arg(AppStyle::Primary));
    badgesRow->addWidget(catBadge);
    badgesRow->addStretch();

    if (book.images.size() > 1) {
        QLabel *mediaBadge = new QLabel(QString("📷 %1").arg(book.images.size()), imgBox);
        mediaBadge->setStyleSheet(R"(
            background-color: rgba(15, 23, 42, 0.75);
            color: #FFFFFF;
            font-size: 10px;
            font-weight: 700;
            border-radius: 6px;
            padding: 3px 6px;
        )");
        badgesRow->addWidget(mediaBadge);
    }
    if (!book.videoUrl.trimmed().isEmpty()) {
        QLabel *vidBadge = new QLabel("🎥 Video", imgBox);
        vidBadge->setStyleSheet(R"(
            background-color: rgba(220, 38, 38, 0.9);
            color: #FFFFFF;
            font-size: 10px;
            font-weight: 700;
            border-radius: 6px;
            padding: 3px 6px;
        )");
        badgesRow->addWidget(vidBadge);
    }
    imgBoxLayout->addLayout(badgesRow);

    // Cover image - Large thumbnail filling box
    QLabel *cover = new QLabel(imgBox);
    cover->setFixedHeight(175);
    cover->setAlignment(Qt::AlignCenter);
    ImageLoader::instance().load(book.coverImage, cover, QSize(220, 175), book.title, book.author);
    imgBoxLayout->addWidget(cover, 1);

    layout->addWidget(imgBox);

    // Title
    QLabel *titleLabel = new QLabel(book.title, card);
    titleLabel->setWordWrap(true);
    titleLabel->setFixedHeight(36);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 13.5px; font-weight: 700; line-height: 1.2;").arg(AppStyle::TextPrimary));
    layout->addWidget(titleLabel);

    // Author
    QLabel *authorLabel = new QLabel("by " + (book.author.isEmpty() ? "Unknown" : book.author), card);
    authorLabel->setStyleSheet(QString("color: %1; font-size: 11.5px;").arg(AppStyle::TextSecondary));
    layout->addWidget(authorLabel);

    // Ratings & Condition row
    QHBoxLayout *ratingRow = new QHBoxLayout();
    ratingRow->setSpacing(6);

    QLabel *ratingLabel = new QLabel(card);
    if (book.reviewCount > 0) {
        ratingLabel->setText(QString("★ %1 (%2)").arg(QString::number(book.averageRating, 'f', 1)).arg(book.reviewCount));
        ratingLabel->setStyleSheet("color: #D97706; font-size: 11.5px; font-weight: 700;");
    } else {
        ratingLabel->setText("★ New");
        ratingLabel->setStyleSheet(QString("color: %1; font-size: 11.5px;").arg(AppStyle::TextMuted));
    }
    ratingRow->addWidget(ratingLabel);

    QLabel *dot = new QLabel("•", card);
    dot->setStyleSheet(QString("color: %1; font-size: 11.5px;").arg(AppStyle::TextMuted));
    ratingRow->addWidget(dot);

    QLabel *condLabel = new QLabel(book.condition.isEmpty() ? "Good" : book.condition, card);
    condLabel->setStyleSheet(AppStyle::statusBadgeStyle(book.condition.isEmpty() ? "AVAILABLE" : book.condition));
    ratingRow->addWidget(condLabel);

    ratingRow->addStretch();
    layout->addLayout(ratingRow);

    layout->addSpacing(4);

    // Price & Action Button row
    QHBoxLayout *bottom = new QHBoxLayout();
    bottom->setSpacing(8);

    QLabel *priceLabel = new QLabel(QString("₹%1").arg(book.price, 0, 'f', 0), card);
    priceLabel->setStyleSheet(QString("color: %1; font-size: 19px; font-weight: 900;").arg(AppStyle::Primary));
    bottom->addWidget(priceLabel);
    bottom->addStretch();

    QPushButton *viewButton = new QPushButton("View Details", card);
    viewButton->setMinimumHeight(34);
    viewButton->setMinimumWidth(100);
    viewButton->setCursor(Qt::PointingHandCursor);
    viewButton->setStyleSheet(AppStyle::primaryButtonStyle());

    const QString bId = book.id;
    connect(viewButton, &QPushButton::clicked, this, [this, bId]() {
        emit bookSelected(bId);
    });

    bottom->addWidget(viewButton);
    layout->addLayout(bottom);

    return card;
}


void BrowseWindow::handleSearchTextChanged(const QString &text)
{
    if (text.trimmed().isEmpty()) {
        if (debounceTimer) debounceTimer->stop();
        if (suggestionsList) {
            suggestionsList->clear();
            suggestionsList->setVisible(false);
        }
        return;
    }

    if (debounceTimer) {
        debounceTimer->start();
    }
}

void BrowseWindow::fetchSuggestions()
{
    const QString text = searchEdit ? searchEdit->text().trimmed() : QString();
    if (text.isEmpty() || !suggestionsList) {
        if (suggestionsList) suggestionsList->setVisible(false);
        return;
    }

    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("limit", "5");

    auto *reply = ApiClient::instance().getPublic(QStringLiteral("/api/books/suggestions"), query);
    connect(reply, &QNetworkReply::finished, this, [this, reply, text]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!ok || !suggestionsList) return;

        // Verify query text still matches what user typed
        if (searchEdit && searchEdit->text().trimmed() != text) return;

        suggestionsList->clear();
        if (json.contains(QStringLiteral("suggestions")) && json[QStringLiteral("suggestions")].isArray()) {
            const auto arr = json[QStringLiteral("suggestions")].toArray();
            for (const auto &val : arr) {
                suggestionsList->addItem(val.toString());
            }
        }

        suggestionsList->setVisible(suggestionsList->count() > 0);
    });
}

void BrowseWindow::handleSuggestionSelected(int row)
{
    if (!suggestionsList || row < 0 || row >= suggestionsList->count()) return;
    QListWidgetItem *item = suggestionsList->item(row);
    if (item && searchEdit) {
        searchEdit->setText(item->text());
        suggestionsList->clear();
        suggestionsList->setVisible(false);
        handleSearch();
    }
}

void BrowseWindow::handleSearch()
{
    if (suggestionsList) {
        suggestionsList->clear();
        suggestionsList->setVisible(false);
    }
    currentPage = 1;
    fetchBooks();
}

void BrowseWindow::handleFilterChanged()
{
    currentPage = 1;
    fetchBooks();
}

void BrowseWindow::handleResetFilters()
{
    if (suggestionsList) {
        suggestionsList->clear();
        suggestionsList->setVisible(false);
    }
    if (searchEdit) searchEdit->clear();
    if (categoryCombo) categoryCombo->setCurrentIndex(0);
    if (conditionCombo) conditionCombo->setCurrentIndex(0);
    if (sortCombo) sortCombo->setCurrentIndex(0);
    if (minPriceSpin) minPriceSpin->setValue(0);
    if (maxPriceSpin) maxPriceSpin->setValue(0);
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