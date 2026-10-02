#include "bookdetailswindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"

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
#include <QFileInfo>
#include <QPainter>
#include <QLinearGradient>
#include <QNetworkReply>
#include <QComboBox>
#include <QTextEdit>
#include <QDialog>
#include <QUrlQuery>

// =========================================================
// CONSTRUCTOR
// =========================================================

BookDetailsWindow::BookDetailsWindow(
    const QString &bookId,
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
    sellerLabel(nullptr),
    ratingLabel(nullptr),
    descriptionLabel(nullptr),
    coverLabel(nullptr),
    reviewsLayout(nullptr),
    orderButton(nullptr),
    exchangeButton(nullptr),
    backButton(nullptr)
{
    setWindowTitle("BookBazzar - Book Details");
    resize(1400, 850);
    setMinimumSize(1000, 650);

    setupUI();
    loadBook();
    loadReviews();
}

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
    pageLayout->setSpacing(25);

    pageLayout->addWidget(createTopBar());
    pageLayout->addWidget(createDetailsSection());
    pageLayout->addWidget(createReviewsSection());

    scrollArea->setWidget(page);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(scrollArea);
}

// =========================================================
// TOP BAR
// =========================================================

QWidget* BookDetailsWindow::createTopBar()
{
    QFrame *topBar = new QFrame();
    topBar->setFixedHeight(76);
    topBar->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border-bottom: 1px solid #E6E8EF;
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(32, 10, 32, 10);
    layout->setSpacing(15);

    backButton = new QPushButton("← Back to Browse");
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
            color: #111827;
        }
    )");

    QLabel *pageTitle = new QLabel("Book Details");
    pageTitle->setStyleSheet(R"(
        QLabel {
            color: #172033;
            font-size: 20px;
            font-weight: 800;
        }
    )");

    layout->addWidget(backButton);
    layout->addWidget(pageTitle);
    layout->addStretch();

    connect(backButton, &QPushButton::clicked, this, &BookDetailsWindow::handleBack);

    return topBar;
}

// =========================================================
// DETAILS SECTION
// =========================================================

QWidget* BookDetailsWindow::createDetailsSection()
{
    QWidget *container = new QWidget();
    QHBoxLayout *mainLayout = new QHBoxLayout(container);
    mainLayout->setContentsMargins(32, 0, 32, 0);
    mainLayout->setSpacing(35);

    // Left Column: Cover Image & Actions
    QFrame *leftCard = new QFrame();
    leftCard->setFixedWidth(340);
    leftCard->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border: 1px solid #E5E7EB;
            border-radius: 16px;
        }
    )");

    QVBoxLayout *leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(20, 20, 20, 20);
    leftLayout->setSpacing(18);

    coverLabel = new QLabel();
    coverLabel->setFixedHeight(360);
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setStyleSheet(R"(
        QLabel {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #4F46E5, stop:1 #7C3AED);
            border-radius: 12px;
            color: white;
            font-size: 28px;
            font-weight: 900;
        }
    )");
    leftLayout->addWidget(coverLabel);

    orderButton = new QPushButton("Add to Cart");
    orderButton->setMinimumHeight(48);
    orderButton->setCursor(Qt::PointingHandCursor);
    orderButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4F46E5;
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 15px;
            font-weight: 800;
        }
        QPushButton:hover {
            background-color: #4338CA;
        }
        QPushButton:disabled {
            background-color: #9CA3AF;
            color: #E5E7EB;
        }
    )");
    leftLayout->addWidget(orderButton);

    exchangeButton = new QPushButton(QStringLiteral("⇄ Propose Exchange"));
    exchangeButton->setMinimumHeight(44);
    exchangeButton->setCursor(Qt::PointingHandCursor);
    exchangeButton->setStyleSheet(R"(
        QPushButton {
            background-color: white;
            color: #4F46E5;
            border: 2px solid #4F46E5;
            border-radius: 10px;
            font-size: 14px;
            font-weight: 800;
        }
        QPushButton:hover {
            background-color: #EEF2FF;
        }
        QPushButton:disabled {
            border-color: #D1D5DB;
            color: #9CA3AF;
        }
    )");
    leftLayout->addWidget(exchangeButton);
    leftLayout->addStretch();

    connect(orderButton, &QPushButton::clicked, this, &BookDetailsWindow::handleOrder);
    connect(exchangeButton, &QPushButton::clicked, this, &BookDetailsWindow::handleProposeExchange);

    // Right Column: Metadata & Description
    QFrame *rightCard = new QFrame();
    rightCard->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border: 1px solid #E5E7EB;
            border-radius: 16px;
        }
    )");

    QVBoxLayout *rightLayout = new QVBoxLayout(rightCard);
    rightLayout->setContentsMargins(28, 25, 28, 25);
    rightLayout->setSpacing(16);

    titleLabel = new QLabel("Loading book...");
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet("color: #111827; font-size: 24px; font-weight: 800;");
    rightLayout->addWidget(titleLabel);

    authorLabel = new QLabel("by ...");
    authorLabel->setStyleSheet("color: #6B7280; font-size: 14px; font-weight: 600;");
    rightLayout->addWidget(authorLabel);

    QHBoxLayout *priceRatingRow = new QHBoxLayout();
    priceLabel = new QLabel("₹0");
    priceLabel->setStyleSheet("color: #4F46E5; font-size: 26px; font-weight: 900;");
    priceRatingRow->addWidget(priceLabel);
    priceRatingRow->addSpacing(20);

    ratingLabel = new QLabel("★ 0.0 (0 reviews)");
    ratingLabel->setStyleSheet("color: #D97706; font-size: 15px; font-weight: 700;");
    priceRatingRow->addWidget(ratingLabel);
    priceRatingRow->addStretch();
    rightLayout->addLayout(priceRatingRow);

    QFrame *divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color: #E5E7EB;");
    rightLayout->addWidget(divider);

    // Grid of metadata cards
    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(12);

    conditionLabel = new QLabel("-");
    categoryLabel = new QLabel("-");
    isbnLabel = new QLabel("-");
    sellerLabel = new QLabel("-");

    grid->addWidget(createInfoCard("Condition", conditionLabel->text()), 0, 0);
    grid->addWidget(createInfoCard("Category", categoryLabel->text()), 0, 1);
    grid->addWidget(createInfoCard("ISBN", isbnLabel->text()), 1, 0);
    grid->addWidget(createInfoCard("Listed By", sellerLabel->text()), 1, 1);

    rightLayout->addLayout(grid);

    QLabel *descHeader = new QLabel("About this book");
    descHeader->setStyleSheet("color: #172033; font-size: 16px; font-weight: 800; margin-top: 10px;");
    rightLayout->addWidget(descHeader);

    descriptionLabel = new QLabel("No description provided.");
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet("color: #4B5563; font-size: 13px; line-height: 1.5;");
    rightLayout->addWidget(descriptionLabel);
    rightLayout->addStretch();

    mainLayout->addWidget(leftCard);
    mainLayout->addWidget(rightCard, 1);

    return container;
}

QFrame* BookDetailsWindow::createInfoCard(const QString &label, const QString &value)
{
    QFrame *card = new QFrame();
    card->setStyleSheet(R"(
        QFrame {
            background-color: #F9FAFB;
            border: 1px solid #E5E7EB;
            border-radius: 10px;
            padding: 8px 12px;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(3);

    QLabel *lbl = new QLabel(label.toUpper());
    lbl->setStyleSheet("color: #9CA3AF; font-size: 10px; font-weight: 800;");
    layout->addWidget(lbl);

    QLabel *val = new QLabel(value);
    val->setObjectName("infoValue");
    val->setStyleSheet("color: #1F2937; font-size: 13px; font-weight: 700;");
    layout->addWidget(val);

    return card;
}

// =========================================================
// REVIEWS SECTION
// =========================================================

QWidget* BookDetailsWindow::createReviewsSection()
{
    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(32, 10, 32, 0);
    layout->setSpacing(15);

    QLabel *header = new QLabel("Reader Reviews");
    header->setStyleSheet("color: #111827; font-size: 20px; font-weight: 800;");
    layout->addWidget(header);

    QFrame *card = new QFrame();
    card->setStyleSheet("background-color: white; border: 1px solid #E5E7EB; border-radius: 16px;");
    reviewsLayout = new QVBoxLayout(card);
    reviewsLayout->setContentsMargins(20, 20, 20, 20);
    reviewsLayout->setSpacing(12);

    QLabel *empty = new QLabel("Loading reviews...");
    empty->setStyleSheet("color: #9CA3AF; font-size: 13px;");
    reviewsLayout->addWidget(empty);

    layout->addWidget(card);
    return container;
}

// =========================================================
// LOAD BOOK
// =========================================================

void BookDetailsWindow::loadBook()
{
    auto *reply = ApiClient::instance().getPublic(QStringLiteral("/api/books/") + bookId);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!ok || !json.contains(QStringLiteral("book"))) {
            QMessageBox::critical(this, "Book Not Found", errorMsg.isEmpty() ? "The selected book could not be found." : errorMsg);
            if (orderButton) orderButton->setEnabled(false);
            return;
        }

        book = BookModel::fromJson(json[QStringLiteral("book")].toObject());

        titleLabel->setText(book.title.isEmpty() ? "Untitled Book" : book.title);
        authorLabel->setText(book.author.isEmpty() ? "by Unknown Author" : "by " + book.author);
        priceLabel->setText(QString("₹%1").arg(book.price, 0, 'f', 0));
        conditionLabel->setText(book.condition.isEmpty() ? "Not specified" : book.condition);
        categoryLabel->setText(book.category.isEmpty() ? "Not specified" : book.category);
        isbnLabel->setText(book.isbn.isEmpty() ? "Not specified" : book.isbn);
        sellerLabel->setText(book.owner.username.isEmpty() ? "Unknown" : book.owner.username);
        descriptionLabel->setText(book.description.isEmpty() ? "No description available." : book.description);

        QString stars;
        for (int i = 0; i < 5; ++i) {
            stars += (i < static_cast<int>(book.averageRating + 0.5)) ? "★" : "☆";
        }
        ratingLabel->setText(QString("%1 %2 (%3 review%4)")
                                 .arg(stars)
                                 .arg(book.averageRating, 0, 'f', 1)
                                 .arg(book.reviewCount)
                                 .arg(book.reviewCount == 1 ? "" : "s"));

        // Update info cards
        auto cards = findChildren<QFrame*>();
        for (auto *c : cards) {
            auto labels = c->findChildren<QLabel*>();
            if (labels.size() >= 2) {
                QString title = labels[0]->text();
                if (title == "CONDITION") labels[1]->setText(book.condition);
                else if (title == "CATEGORY") labels[1]->setText(book.category);
                else if (title == "ISBN") labels[1]->setText(book.isbn.isEmpty() ? "N/A" : book.isbn);
                else if (title == "LISTED BY") labels[1]->setText(book.owner.username);
            }
        }

        // Check if user owns this book
        const QString currentUserId = SessionManager::instance().userId();
        if (!currentUserId.isEmpty() && book.owner.id == currentUserId) {
            orderButton->setText("Your Listing");
            orderButton->setEnabled(false);
            if (exchangeButton) {
                exchangeButton->setEnabled(false);
                exchangeButton->setVisible(false);
            }
        } else {
            orderButton->setText("Add to Cart");
            orderButton->setEnabled(true);
            if (exchangeButton) {
                exchangeButton->setEnabled(true);
                exchangeButton->setVisible(true);
            }
        }

        // Render Cover Image
        if (!book.coverImage.trimmed().isEmpty()) {
            QFileInfo fi(book.coverImage);
            if (fi.exists() && fi.isFile()) {
                QPixmap p(book.coverImage);
                if (!p.isNull()) {
                    coverLabel->setPixmap(p.scaled(coverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    return;
                }
            }
        }

        // Fallback: procedural initial/title cover
        QString initials = book.title.left(2).toUpper();
        if (initials.isEmpty()) initials = "BB";
        coverLabel->setText(initials);
    });
}

// =========================================================
// LOAD REVIEWS
// =========================================================

void BookDetailsWindow::loadReviews()
{
    auto *reply = ApiClient::instance().getPublic(QStringLiteral("/api/books/") + bookId + QStringLiteral("/reviews"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!reviewsLayout) return;

        // Clear existing reviews layout items
        QLayoutItem *child;
        while ((child = reviewsLayout->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }

        if (!ok || !json.contains(QStringLiteral("reviews"))) {
            QLabel *lbl = new QLabel("Unable to load reviews.");
            lbl->setStyleSheet("color: #9CA3AF; font-size: 13px;");
            reviewsLayout->addWidget(lbl);
            return;
        }

        const auto arr = json[QStringLiteral("reviews")].toArray();
        if (arr.isEmpty()) {
            QLabel *lbl = new QLabel("No reviews yet. Be the first to review this book!");
            lbl->setStyleSheet("color: #6B7280; font-size: 13px; font-style: italic;");
            reviewsLayout->addWidget(lbl);
            return;
        }

        for (const auto &v : arr) {
            const auto rev = ReviewModel::fromJson(v.toObject());
            QFrame *item = new QFrame();
            item->setStyleSheet("background-color: #F9FAFB; border: 1px solid #E5E7EB; border-radius: 10px; padding: 10px;");
            QVBoxLayout *iLayout = new QVBoxLayout(item);
            iLayout->setSpacing(4);

            QHBoxLayout *top = new QHBoxLayout();
            QString stars;
            for (int s = 0; s < 5; ++s) {
                stars += (s < rev.rating) ? "★" : "☆";
            }
            QLabel *starLabel = new QLabel(stars);
            starLabel->setStyleSheet("color: #D97706; font-size: 14px; font-weight: 800;");

            QLabel *userLabel = new QLabel("by " + rev.reviewerUsername);
            userLabel->setStyleSheet("color: #374151; font-size: 12px; font-weight: 700;");

            QLabel *dateLabel = new QLabel(rev.createdAt.toString("MMM d, yyyy"));
            dateLabel->setStyleSheet("color: #9CA3AF; font-size: 11px;");

            top->addWidget(starLabel);
            top->addWidget(userLabel);
            top->addStretch();
            top->addWidget(dateLabel);
            iLayout->addLayout(top);

            QLabel *comment = new QLabel(rev.comment);
            comment->setWordWrap(true);
            comment->setStyleSheet("color: #4B5563; font-size: 13px;");
            iLayout->addWidget(comment);

            reviewsLayout->addWidget(item);
        }
    });
}

// =========================================================
// ORDER / ADD TO CART
// =========================================================

void BookDetailsWindow::handleOrder()
{
    if (!SessionManager::instance().isLoggedIn()) {
        QMessageBox::warning(this, "Login Required", "Please log in before adding items to your cart.");
        return;
    }

    if (book.owner.id == SessionManager::instance().userId()) {
        QMessageBox::warning(this, "Cannot Buy", "You cannot buy your own book.");
        return;
    }

    orderButton->setEnabled(false);
    orderButton->setText("Adding to Cart...");

    QJsonObject body;
    body[QStringLiteral("bookId")] = book.id;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/cart/items"), body, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            orderButton->setText("In Cart");
            QMessageBox::information(this, "Cart", "Book has been added to your shopping cart!");
            emit addToCartRequested(book.id);
        } else {
            orderButton->setEnabled(true);
            orderButton->setText("Add to Cart");
            QMessageBox::warning(this, "Cart Error", errorMsg.isEmpty() ? "Unable to add book to cart." : errorMsg);
        }
    });
}

void BookDetailsWindow::handleProposeExchange()
{
    if (!SessionManager::instance().isLoggedIn()) {
        QMessageBox::warning(this, "Login Required", "Please log in before proposing an exchange.");
        return;
    }

    if (book.owner.id == SessionManager::instance().userId()) {
        QMessageBox::warning(this, "Cannot Exchange", "You cannot exchange with your own book.");
        return;
    }

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("status"), QStringLiteral("available"));
    auto *reply = ApiClient::instance().get(QStringLiteral("/api/books/my"), query);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!ok) {
            QMessageBox::warning(this, "Error", errorMsg.isEmpty() ? "Unable to load your listings." : errorMsg);
            return;
        }

        auto arr = json[QStringLiteral("books")].toArray();
        if (arr.isEmpty()) {
            QMessageBox::information(this, "No Available Books",
                                     "You don't have any available books in your listings to offer for an exchange. Please list a book first!");
            return;
        }

        QDialog dialog(this);
        dialog.setWindowTitle("Propose Book Exchange");
        dialog.resize(460, 340);
        dialog.setStyleSheet("background-color: white; font-family: 'Segoe UI', Arial, sans-serif;");

        auto *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(24, 20, 24, 20);
        layout->setSpacing(14);

        auto *headLbl = new QLabel(QString("Exchange for '%1'").arg(book.title), &dialog);
        headLbl->setStyleSheet("font-size: 16px; font-weight: 800; color: #0F172A;");
        layout->addWidget(headLbl);

        auto *selectLbl = new QLabel("Select your book to offer in exchange:", &dialog);
        selectLbl->setStyleSheet("font-size: 13px; font-weight: 600; color: #475569;");
        layout->addWidget(selectLbl);

        auto *combo = new QComboBox(&dialog);
        combo->setStyleSheet("background: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 8px; padding: 8px; font-size: 13px;");
        for (const auto &val : arr) {
            auto bObj = val.toObject();
            QString bId = bObj["id"].toString();
            QString bTitle = bObj["title"].toString();
            QString bCond = bObj["condition"].toString();
            combo->addItem(QString("%1 (%2)").arg(bTitle, bCond), bId);
        }
        layout->addWidget(combo);

        auto *msgLbl = new QLabel("Optional message to book owner:", &dialog);
        msgLbl->setStyleSheet("font-size: 13px; font-weight: 600; color: #475569;");
        layout->addWidget(msgLbl);

        auto *msgEdit = new QTextEdit(&dialog);
        msgEdit->setPlaceholderText("e.g. Hello, I would love to trade my book with yours!");
        msgEdit->setStyleSheet("background: #F8FAFC; border: 1px solid #CBD5E1; border-radius: 8px; padding: 8px;");
        layout->addWidget(msgEdit);

        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();
        auto *cancelBtn = new QPushButton("Cancel", &dialog);
        cancelBtn->setStyleSheet("padding: 8px 16px; border-radius: 6px; font-weight: 600; background: #F1F5F9; color: #334155;");
        connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
        btnRow->addWidget(cancelBtn);

        auto *sendBtn = new QPushButton("Send Proposal", &dialog);
        sendBtn->setStyleSheet("padding: 8px 16px; border-radius: 6px; font-weight: 700; background: #4F46E5; color: white;");
        btnRow->addWidget(sendBtn);
        layout->addLayout(btnRow);

        connect(sendBtn, &QPushButton::clicked, [&dialog, combo, msgEdit, this]() {
            QString offeredBookId = combo->currentData().toString();
            QString message = msgEdit->toPlainText().trimmed();

            QJsonObject payload;
            payload["requestedBookId"] = book.id;
            payload["offeredBookId"] = offeredBookId;
            if (!message.isEmpty()) {
                payload["message"] = message;
            }

            auto *exReply = ApiClient::instance().post(QStringLiteral("/api/exchanges"), payload, true);
            connect(exReply, &QNetworkReply::finished, [exReply, &dialog, this]() {
                auto [exOk, exJson, exError] = ApiClient::parseReply(exReply);
                exReply->deleteLater();

                if (exOk) {
                    QMessageBox::information(this, "Proposal Sent", "Your exchange proposal has been sent to the book owner!");
                    dialog.accept();
                } else {
                    QMessageBox::warning(&dialog, "Exchange Error", exError.isEmpty() ? "Failed to send exchange proposal." : exError);
                }
            });
        });

        dialog.exec();
    });
}

void BookDetailsWindow::handleBack()
{
    emit backRequested();
}