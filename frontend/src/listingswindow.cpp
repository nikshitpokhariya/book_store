#include "listingswindow.h"
#include "network/ApiClient.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QScrollArea>
#include "StyledMessageBox.h"
#include <QFileInfo>
#include <QPixmap>
#include <QNetworkReply>
#include <QStackedWidget>
#include <QSpinBox>
#include <QJsonObject>
#include <QJsonArray>

ListingsWindow::ListingsWindow(const QString &userName, QWidget *parent)
    : QWidget(parent),
    userName(userName),
    tabListingsBtn(nullptr),
    tabReviewsBtn(nullptr),
    addBtn(nullptr),
    totalCountLabel(nullptr),
    availableCountLabel(nullptr),
    soldCountLabel(nullptr),
    tabStack(nullptr),
    statsBarWidget(nullptr),
    cardsLayout(nullptr),
    reviewsLayout(nullptr),
    currentTab(0)
{
    setWindowTitle("BookBazzar - My Listings & Reviews");
    resize(1240, 820);
    setMinimumSize(980, 640);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    setupUI();
    refreshListings();
    refreshMyReviews();
}

ListingsWindow::~ListingsWindow()
{
}

void ListingsWindow::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(createTopBar());

    // Scrollable Body
    QScrollArea *scrollArea = new QScrollArea(this);
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

    QWidget *content = new QWidget();
    content->setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 28, 40, 48);
    contentLayout->setSpacing(24);

    // Tab buttons
    contentLayout->addWidget(createTabsBar());

    // Stacked widget for tabs
    tabStack = new QStackedWidget();

    // Tab 1: Listings page
    QWidget *listingsPage = new QWidget();
    QVBoxLayout *listingsLayout = new QVBoxLayout(listingsPage);
    listingsLayout->setContentsMargins(0, 0, 0, 0);
    listingsLayout->setSpacing(20);

    statsBarWidget = createStatsBar();
    listingsLayout->addWidget(statsBarWidget);

    cardsLayout = new QVBoxLayout();
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(14);
    listingsLayout->addLayout(cardsLayout);
    listingsLayout->addStretch();
    tabStack->addWidget(listingsPage);

    // Tab 2: Reviews page
    QWidget *reviewsPage = new QWidget();
    QVBoxLayout *revPageLayout = new QVBoxLayout(reviewsPage);
    revPageLayout->setContentsMargins(0, 0, 0, 0);
    revPageLayout->setSpacing(20);

    reviewsLayout = new QVBoxLayout();
    reviewsLayout->setContentsMargins(0, 0, 0, 0);
    reviewsLayout->setSpacing(14);
    revPageLayout->addLayout(reviewsLayout);
    revPageLayout->addStretch();
    tabStack->addWidget(reviewsPage);

    contentLayout->addWidget(tabStack);
    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

QWidget* ListingsWindow::createTopBar()
{
    QFrame *bar = new QFrame();
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

    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(36, 10, 36, 10);
    layout->setSpacing(16);

    QPushButton *backButton = new QPushButton("← Back");
    backButton->setFixedSize(88, 38);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(backButton, &QPushButton::clicked, this, &ListingsWindow::handleBack);
    layout->addWidget(backButton);

    QLabel *title = new QLabel("My Inventory & Reviews");
    title->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: 800;").arg(AppStyle::TextPrimary));
    layout->addWidget(title);

    layout->addStretch();

    addBtn = new QPushButton("+ Sell Another Book");
    addBtn->setFixedHeight(40);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(AppStyle::primaryButtonStyle());
    connect(addBtn, &QPushButton::clicked, this, &ListingsWindow::handleAddNew);
    layout->addWidget(addBtn);

    return bar;
}

QWidget* ListingsWindow::createTabsBar()
{
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    tabListingsBtn = new QPushButton("My Listed Books");
    tabListingsBtn->setCursor(Qt::PointingHandCursor);
    tabListingsBtn->setStyleSheet(AppStyle::activeTabStyle());

    tabReviewsBtn = new QPushButton("My Reviews & Feedback");
    tabReviewsBtn->setCursor(Qt::PointingHandCursor);
    tabReviewsBtn->setStyleSheet(AppStyle::inactiveTabStyle());

    layout->addWidget(tabListingsBtn);
    layout->addWidget(tabReviewsBtn);
    layout->addStretch();

    connect(tabListingsBtn, &QPushButton::clicked, this, [this]() {
        handleTabChange(0);
    });

    connect(tabReviewsBtn, &QPushButton::clicked, this, [this]() {
        handleTabChange(1);
    });

    return container;
}

void ListingsWindow::handleTabChange(int tabIndex)
{
    currentTab = tabIndex;
    tabStack->setCurrentIndex(tabIndex);

    if (tabIndex == 0) {
        tabListingsBtn->setStyleSheet(AppStyle::activeTabStyle());
        tabReviewsBtn->setStyleSheet(AppStyle::inactiveTabStyle());
        if (addBtn) addBtn->setVisible(true);
        refreshListings();
    } else {
        tabListingsBtn->setStyleSheet(AppStyle::inactiveTabStyle());
        tabReviewsBtn->setStyleSheet(AppStyle::activeTabStyle());
        if (addBtn) addBtn->setVisible(false);
        refreshMyReviews();
    }
}

QWidget* ListingsWindow::createStatsBar()
{
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);

    auto createStatCard = [](const QString &title, QLabel* &labelRef, const QString &color) {
        QFrame *card = new QFrame();
        card->setStyleSheet(AppStyle::statCardStyle());
        AppStyle::applyElevation(card, 14, 3, 10);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 16, 20, 16);
        cardLayout->setSpacing(4);

        QLabel *titleLabel = new QLabel(title);
        titleLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600;").arg(AppStyle::TextSecondary));
        cardLayout->addWidget(titleLabel);

        labelRef = new QLabel("0");
        labelRef->setStyleSheet(QString("color: %1; font-size: 26px; font-weight: 800;").arg(color));
        cardLayout->addWidget(labelRef);

        return card;
    };

    layout->addWidget(createStatCard("Total Listings", totalCountLabel, AppStyle::TextPrimary));
    layout->addWidget(createStatCard("Active / Available", availableCountLabel, AppStyle::Success));
    layout->addWidget(createStatCard("Books Sold", soldCountLabel, AppStyle::Primary));

    return container;
}

void ListingsWindow::refreshListings()
{
    if (!cardsLayout) return;

    auto *reply = ApiClient::instance().get(QStringLiteral("/api/books/my"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!cardsLayout) return;

        QLayoutItem *item;
        while ((item = cardsLayout->takeAt(0)) != nullptr) {
            if (item->widget()) delete item->widget();
            delete item;
        }

        if (!ok || !json.contains(QStringLiteral("books"))) {
            QLabel *err = new QLabel(errorMsg.isEmpty() ? "Error loading listings." : errorMsg);
            cardsLayout->addWidget(err);
            return;
        }

        const auto arr = json[QStringLiteral("books")].toArray();
        int total = static_cast<int>(arr.size());
        int available = 0;
        int sold = 0;

        if (json.contains(QStringLiteral("pagination"))) {
            PaginationMeta pag = PaginationMeta::fromJson(json[QStringLiteral("pagination")].toObject());
            total = pag.total;
        }

        QVector<BookModel> books;
        for (const auto &v : arr) {
            BookModel b = BookModel::fromJson(v.toObject());
            books.append(b);
            if (b.status.compare("sold", Qt::CaseInsensitive) == 0) sold++;
            else if (b.status.compare("available", Qt::CaseInsensitive) == 0) available++;
        }

        if (totalCountLabel) totalCountLabel->setText(QString::number(total));
        if (availableCountLabel) availableCountLabel->setText(QString::number(available));
        if (soldCountLabel) soldCountLabel->setText(QString::number(sold));

        if (books.isEmpty()) {
            QLabel *empty = new QLabel("You haven't listed any books for sale yet.\nClick '+ Sell Another Book' above to create your first listing!");
            empty->setAlignment(Qt::AlignCenter);
            empty->setMinimumHeight(200);
            empty->setStyleSheet(QString(R"(
                QLabel {
                    background-color: #FFFFFF;
                    border: 1px dashed %1;
                    border-radius: 14px;
                    color: %2;
                    font-size: 14px;
                    font-weight: 600;
                    padding: 32px;
                }
            )").arg(AppStyle::BorderStrong, AppStyle::TextSecondary));
            cardsLayout->addWidget(empty);
        } else {
            for (const BookModel &b : books) {
                cardsLayout->addWidget(createListingCard(b));
            }
        }
    });
}

QWidget* ListingsWindow::createListingCard(const BookModel &book)
{
    QFrame *card = new QFrame();
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 14, 3, 10);

    QHBoxLayout *layout = new QHBoxLayout(card);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(18);

    // Book Cover thumbnail
    QLabel *cover = new QLabel();
    cover->setFixedSize(60, 80);
    cover->setAlignment(Qt::AlignCenter);
    cover->setStyleSheet(R"(
        QLabel {
            background-color: #0F172A;
            border-radius: 6px;
            color: #FFFFFF;
            font-weight: 800;
            font-size: 13px;
        }
    )");

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
        cover->setText(book.title.left(2).toUpper());
    }
    layout->addWidget(cover);

    // Details Column
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4);

    QLabel *titleLbl = new QLabel(book.title);
    titleLbl->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 700;").arg(AppStyle::TextPrimary));
    infoLayout->addWidget(titleLbl);

    QLabel *authorLbl = new QLabel("by " + book.author);
    authorLbl->setStyleSheet(QString("color: %1; font-size: 12px;").arg(AppStyle::TextSecondary));
    infoLayout->addWidget(authorLbl);

    QHBoxLayout *metaLayout = new QHBoxLayout();
    metaLayout->setSpacing(8);

    QLabel *catBadge = new QLabel(book.category.isEmpty() ? "General" : book.category);
    catBadge->setStyleSheet(AppStyle::badgeStyle(AppStyle::PrimaryLight, AppStyle::Primary, AppStyle::PrimaryBorder));
    metaLayout->addWidget(catBadge);

    QLabel *condBadge = new QLabel(book.condition.isEmpty() ? "Good" : book.condition);
    condBadge->setStyleSheet(AppStyle::statusBadgeStyle(book.condition.isEmpty() ? "AVAILABLE" : book.condition));
    metaLayout->addWidget(condBadge);

    metaLayout->addStretch();
    infoLayout->addLayout(metaLayout);
    layout->addLayout(infoLayout, 1);

    // Status Badge
    QLabel *statusBadge = new QLabel(book.status.toUpper());
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setStyleSheet(AppStyle::statusBadgeStyle(book.status));
    layout->addWidget(statusBadge);

    // Price
    QLabel *priceLbl = new QLabel(QString("₹%1").arg(QString::number(book.price, 'f', 0)));
    priceLbl->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: 800;").arg(AppStyle::TextPrimary));
    layout->addWidget(priceLbl);

    layout->addSpacing(8);

    const bool isAvailable = (book.status.compare("available", Qt::CaseInsensitive) == 0);

    QPushButton *editBtn = new QPushButton("Edit");
    editBtn->setFixedSize(68, 36);
    editBtn->setEnabled(isAvailable);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(editBtn, &QPushButton::clicked, this, [this, book]() {
        handleEditListing(book);
    });
    layout->addWidget(editBtn);

    QPushButton *deleteBtn = new QPushButton("Delete");
    deleteBtn->setFixedSize(72, 36);
    deleteBtn->setEnabled(isAvailable);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setStyleSheet(AppStyle::dangerButtonStyle());
    connect(deleteBtn, &QPushButton::clicked, this, [this, book]() {
        handleDeleteListing(book.id, book.title);
    });
    layout->addWidget(deleteBtn);

    return card;
}

void ListingsWindow::refreshMyReviews()
{
    if (!reviewsLayout) return;

    auto *reply = ApiClient::instance().get(QStringLiteral("/api/reviews/my"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!reviewsLayout) return;

        QLayoutItem *item;
        while ((item = reviewsLayout->takeAt(0)) != nullptr) {
            if (item->widget()) delete item->widget();
            delete item;
        }

        if (!ok || !json.contains(QStringLiteral("reviews"))) {
            QLabel *err = new QLabel(errorMsg.isEmpty() ? "Unable to load reviews." : errorMsg);
            err->setStyleSheet(QString("color: %1; font-size: 14px;").arg(AppStyle::Danger));
            reviewsLayout->addWidget(err);
            return;
        }

        const auto arr = json[QStringLiteral("reviews")].toArray();
        if (arr.isEmpty()) {
            QLabel *empty = new QLabel("You haven't written any reviews yet.\nOnce you receive a delivered order, you can rate and review the book!");
            empty->setAlignment(Qt::AlignCenter);
            empty->setMinimumHeight(200);
            empty->setStyleSheet(QString(R"(
                QLabel {
                    background-color: #FFFFFF;
                    border: 1px dashed %1;
                    border-radius: 14px;
                    color: %2;
                    font-size: 14px;
                    font-weight: 600;
                    padding: 32px;
                }
            )").arg(AppStyle::BorderStrong, AppStyle::TextSecondary));
            reviewsLayout->addWidget(empty);
            return;
        }

        for (const auto &v : arr) {
            ReviewModel rev = ReviewModel::fromJson(v.toObject());
            reviewsLayout->addWidget(createReviewCard(rev));
        }
    });
}

QWidget* ListingsWindow::createReviewCard(const ReviewModel &rev)
{
    QFrame *card = new QFrame();
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 14, 3, 10);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(10);

    QHBoxLayout *topRow = new QHBoxLayout();

    QString stars;
    for (int s = 0; s < 5; ++s) {
        stars += (s < rev.rating) ? "★" : "☆";
    }
    QLabel *starLabel = new QLabel(stars);
    starLabel->setStyleSheet("color: #D97706; font-size: 16px; font-weight: 800;");
    topRow->addWidget(starLabel);

    QLabel *scoreLabel = new QLabel(QString("(%1/5)").arg(rev.rating));
    scoreLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600;").arg(AppStyle::TextMuted));
    topRow->addWidget(scoreLabel);

    topRow->addSpacing(16);
    QLabel *bookRef = new QLabel("Book ID: " + rev.bookId);
    bookRef->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600;").arg(AppStyle::TextSecondary));
    topRow->addWidget(bookRef);

    topRow->addStretch();

    QLabel *dateLabel = new QLabel(rev.createdAt.isValid() ? rev.createdAt.toString("MMM d, yyyy") : "");
    dateLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(AppStyle::TextMuted));
    topRow->addWidget(dateLabel);

    layout->addLayout(topRow);

    QLabel *commentLabel = new QLabel(rev.comment);
    commentLabel->setWordWrap(true);
    commentLabel->setStyleSheet(QString("color: %1; font-size: 13px; line-height: 1.4;").arg(AppStyle::TextPrimary));
    layout->addWidget(commentLabel);

    QHBoxLayout *actionsRow = new QHBoxLayout();
    actionsRow->addStretch();

    QPushButton *editBtn = new QPushButton("Edit Review");
    editBtn->setFixedSize(92, 34);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(editBtn, &QPushButton::clicked, this, [this, rev]() {
        handleEditReview(rev);
    });
    actionsRow->addWidget(editBtn);

    QPushButton *deleteBtn = new QPushButton("Delete");
    deleteBtn->setFixedSize(76, 34);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setStyleSheet(AppStyle::dangerButtonStyle());
    connect(deleteBtn, &QPushButton::clicked, this, [this, rev]() {
        handleDeleteReview(rev.id);
    });
    actionsRow->addWidget(deleteBtn);

    layout->addLayout(actionsRow);
    return card;
}

void ListingsWindow::handleDeleteListing(const QString &bookId, const QString &bookTitle)
{
    bool confirmed = StyledMessageBox::question(
        this, "Delete Listing",
        QString("Are you sure you want to remove \"%1\" from your listings?").arg(bookTitle),
        "Yes, Remove", "Cancel"
    );

    if (confirmed) {
        auto *reply = ApiClient::instance().deleteResource(QStringLiteral("/api/books/") + bookId);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
            reply->deleteLater();

            if (ok) {
                StyledMessageBox::success(this, "Listing Removed", "Your book listing has been removed successfully.");
                refreshListings();
            } else {
                StyledMessageBox::warning(this, "Error", errorMsg.isEmpty() ? "Could not delete this book listing." : errorMsg);
            }
        });
    }
}

void ListingsWindow::handleEditListing(const BookModel &book)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Edit Book Listing");
    dialog.resize(520, 560);
    dialog.setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    QVBoxLayout layout(&dialog);
    layout.setSpacing(14);
    layout.setContentsMargins(28, 24, 28, 24);

    QLabel *hdr = new QLabel("Edit Book Details", &dialog);
    hdr->setStyleSheet(QString("font-size: 18px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    layout.addWidget(hdr);

    QLineEdit titleEdit(book.title, &dialog);
    titleEdit.setStyleSheet(AppStyle::inputStyle());

    QLineEdit authorEdit(book.author, &dialog);
    authorEdit.setStyleSheet(AppStyle::inputStyle());

    QLineEdit priceEdit(QString::number(book.price, 'f', 0), &dialog);
    priceEdit.setStyleSheet(AppStyle::inputStyle());

    QComboBox categoryCombo(&dialog);
    categoryCombo.setStyleSheet(AppStyle::comboBoxStyle());
    categoryCombo.addItems({"Fiction", "Non-Fiction", "Academic", "Programming", "Engineering", "Competitive Exams", "School", "Novels", "Other"});
    int cIdx = categoryCombo.findText(book.category);
    if (cIdx >= 0) categoryCombo.setCurrentIndex(cIdx);

    QComboBox conditionCombo(&dialog);
    conditionCombo.setStyleSheet(AppStyle::comboBoxStyle());
    conditionCombo.addItems({"New", "Like New", "Very Good", "Good", "Acceptable"});
    int condIdx = conditionCombo.findText(book.condition);
    if (condIdx >= 0) conditionCombo.setCurrentIndex(condIdx);

    QTextEdit descEdit(&dialog);
    descEdit.setStyleSheet(AppStyle::inputStyle());
    descEdit.setPlainText(book.description);
    descEdit.setMaximumHeight(90);

    QLineEdit imageEdit(book.coverImage, &dialog);
    imageEdit.setStyleSheet(AppStyle::inputStyle());

    auto addField = [&layout](const QString &label, QWidget *widget) {
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600;").arg(AppStyle::TextSecondary));
        layout.addWidget(lbl);
        layout.addWidget(widget);
    };

    addField("Title:", &titleEdit);
    addField("Author:", &authorEdit);
    addField("Price (₹):", &priceEdit);
    addField("Category:", &categoryCombo);
    addField("Condition:", &conditionCombo);
    addField("Description:", &descEdit);
    addField("Cover Image Path / URL:", &imageEdit);

    QHBoxLayout btnRow;
    btnRow.addStretch();
    QPushButton cancelBtn("Cancel", &dialog);
    cancelBtn.setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(&cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    btnRow.addWidget(&cancelBtn);

    QPushButton saveBtn("Save Changes", &dialog);
    saveBtn.setStyleSheet(AppStyle::primaryButtonStyle());
    connect(&saveBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnRow.addWidget(&saveBtn);
    layout.addLayout(&btnRow);

    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject body;
        body[QStringLiteral("title")] = titleEdit.text().trimmed();
        body[QStringLiteral("author")] = authorEdit.text().trimmed();
        body[QStringLiteral("price")] = priceEdit.text().toDouble();
        body[QStringLiteral("category")] = categoryCombo.currentText();
        body[QStringLiteral("condition")] = conditionCombo.currentText();
        body[QStringLiteral("description")] = descEdit.toPlainText().trimmed();
        body[QStringLiteral("coverImage")] = imageEdit.text().trimmed();

        auto *reply = ApiClient::instance().put(QStringLiteral("/api/books/") + book.id, body, true);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
            reply->deleteLater();

            if (ok) {
                StyledMessageBox::success(this, "Success", "Listing updated successfully!");
                refreshListings();
            } else {
                StyledMessageBox::warning(this, "Update Failed", errorMsg.isEmpty() ? "Could not update listing." : errorMsg);
            }
        });
    }
}

void ListingsWindow::handleEditReview(const ReviewModel &rev)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Edit Review");
    dialog.resize(450, 300);
    dialog.setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    QVBoxLayout layout(&dialog);
    layout.setSpacing(14);
    layout.setContentsMargins(28, 24, 28, 24);

    QLabel *hdr = new QLabel("Update Your Review", &dialog);
    hdr->setStyleSheet(QString("font-size: 18px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    layout.addWidget(hdr);

    QLabel *rateLbl = new QLabel("Rating (1-5 stars):", &dialog);
    rateLbl->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600;").arg(AppStyle::TextSecondary));
    layout.addWidget(rateLbl);

    QSpinBox ratingSpin(&dialog);
    ratingSpin.setRange(1, 5);
    ratingSpin.setValue(rev.rating);
    ratingSpin.setStyleSheet(AppStyle::inputStyle());
    layout.addWidget(&ratingSpin);

    QLabel *commentLbl = new QLabel("Your Comment:", &dialog);
    commentLbl->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600;").arg(AppStyle::TextSecondary));
    layout.addWidget(commentLbl);

    QTextEdit commentEdit(&dialog);
    commentEdit.setStyleSheet(AppStyle::inputStyle());
    commentEdit.setPlainText(rev.comment);
    commentEdit.setMaximumHeight(100);
    layout.addWidget(&commentEdit);

    QHBoxLayout btnRow;
    btnRow.addStretch();
    QPushButton cancelBtn("Cancel", &dialog);
    cancelBtn.setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(&cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    btnRow.addWidget(&cancelBtn);

    QPushButton saveBtn("Update Review", &dialog);
    saveBtn.setStyleSheet(AppStyle::primaryButtonStyle());
    connect(&saveBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnRow.addWidget(&saveBtn);
    layout.addLayout(&btnRow);

    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject body;
        body[QStringLiteral("rating")] = ratingSpin.value();
        body[QStringLiteral("comment")] = commentEdit.toPlainText().trimmed();

        auto *reply = ApiClient::instance().put(QStringLiteral("/api/reviews/") + rev.id, body, true);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
            reply->deleteLater();

            if (ok) {
                StyledMessageBox::success(this, "Success", "Review updated successfully!");
                refreshMyReviews();
            } else {
                StyledMessageBox::warning(this, "Update Failed", errorMsg.isEmpty() ? "Could not update review." : errorMsg);
            }
        });
    }
}

void ListingsWindow::handleDeleteReview(const QString &reviewId)
{
    bool confirmed = StyledMessageBox::question(
        this, "Delete Review",
        "Are you sure you want to delete this review?",
        "Yes, Delete", "Cancel"
    );

    if (confirmed) {
        auto *reply = ApiClient::instance().deleteResource(QStringLiteral("/api/reviews/") + reviewId);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
            reply->deleteLater();

            if (ok) {
                StyledMessageBox::success(this, "Review Deleted", "Your review has been removed.");
                refreshMyReviews();
            } else {
                StyledMessageBox::warning(this, "Error", errorMsg.isEmpty() ? "Could not delete this review." : errorMsg);
            }
        });
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
