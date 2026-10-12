#include "bookdetailswindow.h"
#include "windows/ImageInspectionDialog.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include "AppStyle.h"
#include "StyledMessageBox.h"
#include "ImageLoader.h"
#include "VideoPlayerWidget.h"
#include "VideoThumbnailHelper.h"
#include "ProposeExchangeDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrlQuery>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QFrame>
#include <QStackedWidget>
#include <QShortcut>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>
#include <QPainter>
#include <QEvent>
#include <QDialog>

BookDetailsWindow::BookDetailsWindow(const QString &bookId,
                                     const QString &userName,
                                     QWidget *parent)
    : QWidget(parent),
      bookId(bookId),
      userName(userName),
      previewBox_(nullptr),
      mediaStack_(nullptr),
      coverLabel(nullptr),
      videoPlayer_(nullptr),
      prevMediaBtn_(nullptr),
      nextMediaBtn_(nullptr),
      mediaCounterBadge_(nullptr),
      playVideoBtn(nullptr),
      thumbnailsLayout(nullptr),
      titleLabel(nullptr),
      authorLabel(nullptr),
      priceLabel(nullptr),
      conditionLabel(nullptr),
      categoryLabel(nullptr),
      isbnLabel(nullptr),
      sellerLabel(nullptr),
      ratingLabel(nullptr),
      descriptionLabel(nullptr),
      reviewFormCard_(nullptr),
      buyerNoticeLabel_(nullptr),
      reviewCommentEdit_(nullptr),
      submitReviewBtn_(nullptr),
      reviewsLayout(nullptr),
      orderButton(nullptr),
      exchangeButton(nullptr),
      backButton(nullptr)
{
    setWindowTitle("BookBazzar - Book Details");
    resize(1200, 860);
    setMinimumSize(980, 680);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));
    setFocusPolicy(Qt::StrongFocus);

    setupUI();
    setupShortcuts();
    loadBook();
    loadReviews();
    checkBuyerStatus();
}

BookDetailsWindow::~BookDetailsWindow()
{
    if (qApp) {
        qApp->removeEventFilter(this);
    }
}

void BookDetailsWindow::setupUI()
{
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    rootLayout->addWidget(createTopBar());

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setFocusPolicy(Qt::NoFocus);
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
    pageLayout->setSpacing(28);

    pageLayout->addWidget(createDetailsSection());
    pageLayout->addWidget(createReviewsSection());

    scrollArea->setWidget(page);
    rootLayout->addWidget(scrollArea);
}

QWidget* BookDetailsWindow::createTopBar()
{
    QFrame *topBar = new QFrame(this);
    topBar->setFixedHeight(72);
    topBar->setStyleSheet(QString(R"(
        QFrame {
            background-color: #FFFFFF;
            border-bottom: 1px solid %1;
        }
    )").arg(AppStyle::BorderSubtle));

    QHBoxLayout *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(36, 0, 36, 0);
    layout->setSpacing(16);

    backButton = new QPushButton("← Back to Browse", topBar);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setStyleSheet(AppStyle::secondaryButtonStyle());
    backButton->setFixedHeight(40);
    connect(backButton, &QPushButton::clicked, this, &BookDetailsWindow::handleBack);

    QLabel *pageTitle = new QLabel("Book Overview", topBar);
    pageTitle->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: 800;").arg(AppStyle::TextPrimary));

    layout->addWidget(backButton);
    layout->addWidget(pageTitle);
    layout->addStretch();

    return topBar;
}

QWidget* BookDetailsWindow::createDetailsSection()
{
    QWidget *container = new QWidget();
    QHBoxLayout *mainLayout = new QHBoxLayout(container);
    mainLayout->setContentsMargins(36, 24, 36, 0);
    mainLayout->setSpacing(32);

    // Left Column: Media Gallery & Purchase Card
    QFrame *leftCard = new QFrame();
    leftCard->setFixedWidth(360);
    leftCard->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(leftCard, 20, 6, 18);

    QVBoxLayout *leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(20, 20, 20, 20);
    leftLayout->setSpacing(12);

    // Main Media Preview Box
    previewBox_ = new QFrame(leftCard);
    previewBox_->setFixedSize(320, 330);
    previewBox_->setStyleSheet(R"(
        QFrame {
            background-color: #0F172A;
            border: 1px solid #334155;
            border-radius: 12px;
        }
    )");

    QVBoxLayout *previewLayout = new QVBoxLayout(previewBox_);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(0);

    mediaStack_ = new QStackedWidget(previewBox_);
    mediaStack_->setStyleSheet("background-color: #0F172A; border-radius: 12px; border: none;");

    // Page 0: Image Cover / Photo preview
    coverLabel = new QLabel(mediaStack_);
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setStyleSheet("background-color: #0F172A; border-radius: 12px; border: none;");
    coverLabel->setAttribute(Qt::WA_OpaquePaintEvent, true);
    coverLabel->setCursor(Qt::PointingHandCursor);
    coverLabel->setToolTip("Click to view full size");
    coverLabel->installEventFilter(this);
    mediaStack_->addWidget(coverLabel);

    // Page 1: Native Embedded Video Player
    videoPlayer_ = new VideoPlayerWidget(mediaStack_);
    mediaStack_->addWidget(videoPlayer_);

    previewLayout->addWidget(mediaStack_, 1);

    // Legacy play button kept hidden for compatibility
    playVideoBtn = new QPushButton("▶ Play Video Preview", previewBox_);
    playVideoBtn->setVisible(false);

    // Overlay Elements: Floating < and > Carousel Controls & Counter Badge
    prevMediaBtn_ = new QPushButton("❮", previewBox_);
    prevMediaBtn_->setFixedSize(38, 38);
    prevMediaBtn_->setFocusPolicy(Qt::NoFocus);
    prevMediaBtn_->setCursor(Qt::PointingHandCursor);
    prevMediaBtn_->setToolTip("Previous Media (Shortcut: Left Arrow, A, or <)");
    prevMediaBtn_->setStyleSheet(R"(
        QPushButton {
            background-color: #1E293B;
            color: #FFFFFF;
            border: 1.5px solid #475569;
            border-radius: 19px;
            font-size: 15px;
            font-weight: 800;
            padding-bottom: 2px;
        }
        QPushButton:hover {
            background-color: #4F46E5;
            border-color: #818CF8;
        }
        QPushButton:pressed {
            background-color: #3730A3;
        }
    )");
    connect(prevMediaBtn_, &QPushButton::clicked, this, &BookDetailsWindow::showPreviousMedia);

    nextMediaBtn_ = new QPushButton("❯", previewBox_);
    nextMediaBtn_->setFixedSize(38, 38);
    nextMediaBtn_->setFocusPolicy(Qt::NoFocus);
    nextMediaBtn_->setCursor(Qt::PointingHandCursor);
    nextMediaBtn_->setToolTip("Next Media (Shortcut: Right Arrow, D, or >)");
    nextMediaBtn_->setStyleSheet(prevMediaBtn_->styleSheet());
    connect(nextMediaBtn_, &QPushButton::clicked, this, &BookDetailsWindow::showNextMedia);

    mediaCounterBadge_ = new QLabel("1 / 1", previewBox_);
    mediaCounterBadge_->setStyleSheet(R"(
        background-color: #1E293B;
        color: #FFFFFF;
        border: 1px solid #475569;
        border-radius: 10px;
        padding: 4px 10px;
        font-size: 11px;
        font-weight: 700;
    )");

    inspectBtn_ = new QPushButton("🔍 Inspect Full Size", previewBox_);
    inspectBtn_->setFocusPolicy(Qt::NoFocus);
    inspectBtn_->setCursor(Qt::PointingHandCursor);
    inspectBtn_->setToolTip("Click to view in full screen");
    inspectBtn_->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(15, 23, 42, 0.90);
            color: #FFFFFF;
            border: 1px solid #475569;
            border-radius: 6px;
            padding: 5px 12px;
            font-size: 11px;
            font-weight: 700;
        }
        QPushButton:hover {
            background-color: #4F46E5;
            border-color: #818CF8;
        }
        QPushButton:pressed {
            background-color: #3730A3;
        }
    )");
    connect(inspectBtn_, &QPushButton::clicked, this, &BookDetailsWindow::openImageInspection);

    updatePreviewOverlayPositions();

    leftLayout->addWidget(previewBox_);

    // Thumbnails Row
    QLabel *galleryHeading = new QLabel("Media Gallery (Click to inspect):", leftCard);
    galleryHeading->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 700;").arg(AppStyle::TextMuted));
    leftLayout->addWidget(galleryHeading);

    thumbScroll_ = new QScrollArea(leftCard);
    thumbScroll_->setFixedHeight(90);
    thumbScroll_->setWidgetResizable(true);
    thumbScroll_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    thumbScroll_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    thumbScroll_->setFrameShape(QFrame::NoFrame);
    thumbScroll_->setFocusPolicy(Qt::NoFocus);
    thumbScroll_->setStyleSheet(R"(
        QScrollArea {
            background: transparent;
            border: none;
        }
        QScrollBar:horizontal {
            border: none;
            background: transparent;
            height: 4px;
            margin: 0px;
        }
        QScrollBar::handle:horizontal {
            background: #334155;
            min-width: 20px;
            border-radius: 2px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #6366F1;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
    )");

    QWidget *thumbContainer = new QWidget();
    thumbContainer->setStyleSheet("background: transparent;");
    thumbnailsLayout = new QHBoxLayout(thumbContainer);
    thumbnailsLayout->setContentsMargins(0, 0, 0, 0);
    thumbnailsLayout->setSpacing(8);
    thumbScroll_->setWidget(thumbContainer);

    leftLayout->addWidget(thumbScroll_);

    // Purchase Actions
    orderButton = new QPushButton("Add to Cart", leftCard);
    orderButton->setMinimumHeight(44);
    orderButton->setCursor(Qt::PointingHandCursor);
    orderButton->setStyleSheet(AppStyle::primaryButtonStyle());
    leftLayout->addWidget(orderButton);

    exchangeButton = new QPushButton("⇄  Propose Exchange", leftCard);
    exchangeButton->setMinimumHeight(40);
    exchangeButton->setCursor(Qt::PointingHandCursor);
    exchangeButton->setStyleSheet(AppStyle::secondaryButtonStyle());
    leftLayout->addWidget(exchangeButton);

    connect(orderButton, &QPushButton::clicked, this, &BookDetailsWindow::handleOrder);
    connect(exchangeButton, &QPushButton::clicked, this, &BookDetailsWindow::handleProposeExchange);

    // Right Column: Details & Information
    QFrame *rightCard = new QFrame();
    rightCard->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(rightCard, 20, 6, 18);

    QVBoxLayout *rightLayout = new QVBoxLayout(rightCard);
    rightLayout->setContentsMargins(32, 28, 32, 28);
    rightLayout->setSpacing(16);

    titleLabel = new QLabel("Loading book details...", rightCard);
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: 800; line-height: 1.2;").arg(AppStyle::TextPrimary));
    rightLayout->addWidget(titleLabel);

    authorLabel = new QLabel("by ...", rightCard);
    authorLabel->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 600;").arg(AppStyle::TextSecondary));
    rightLayout->addWidget(authorLabel);

    QHBoxLayout *priceRow = new QHBoxLayout();
    priceLabel = new QLabel("₹0", rightCard);
    priceLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: 900;").arg(AppStyle::Primary));
    priceRow->addWidget(priceLabel);
    priceRow->addSpacing(20);

    ratingLabel = new QLabel("★ 0.0 (0 reviews)", rightCard);
    ratingLabel->setStyleSheet("color: #D97706; font-size: 15px; font-weight: 700;");
    priceRow->addWidget(ratingLabel);
    priceRow->addStretch();
    rightLayout->addLayout(priceRow);

    QFrame *divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet(QString("color: %1;").arg(AppStyle::BorderSubtle));
    rightLayout->addWidget(divider);

    // Metadata Grid
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

    QLabel *descHeader = new QLabel("About This Book", rightCard);
    descHeader->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: 800; margin-top: 8px;").arg(AppStyle::TextPrimary));
    rightLayout->addWidget(descHeader);

    descriptionLabel = new QLabel("No description provided.", rightCard);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet(QString("color: %1; font-size: 13.5px; line-height: 1.5;").arg(AppStyle::TextSecondary));
    rightLayout->addWidget(descriptionLabel);
    rightLayout->addStretch();

    mainLayout->addWidget(leftCard);
    mainLayout->addWidget(rightCard, 1);
    return container;
}

QFrame* BookDetailsWindow::createInfoCard(const QString &label, const QString &value)
{
    QFrame *card = new QFrame();
    card->setStyleSheet(QString(R"(
        .QFrame {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 10px;
        }
        QLabel {
            border: none;
            background: transparent;
        }
    )").arg(AppStyle::SurfaceSubtle, AppStyle::BorderSubtle));

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(4);

    QLabel *lbl = new QLabel(label.toUpper());
    lbl->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: 800; letter-spacing: 0.5px;").arg(AppStyle::TextMuted));
    layout->addWidget(lbl);

    QLabel *val = new QLabel(value);
    val->setObjectName("infoValue");
    val->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 700;").arg(AppStyle::TextPrimary));
    layout->addWidget(val);

    return card;
}

QWidget* BookDetailsWindow::createReviewsSection()
{
    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setContentsMargins(36, 12, 36, 0);
    layout->setSpacing(20);

    QLabel *header = new QLabel("Reader Reviews & Ratings");
    header->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: 800;").arg(AppStyle::TextPrimary));
    layout->addWidget(header);

    // ==========================================
    // Buyer Review Form (Issue 4)
    // ==========================================
    reviewFormCard_ = new QFrame();
    reviewFormCard_->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(reviewFormCard_, 16, 4, 14);
    reviewFormCard_->setVisible(false); // only shown for verified buyers

    QVBoxLayout *formLayout = new QVBoxLayout(reviewFormCard_);
    formLayout->setContentsMargins(24, 20, 24, 20);
    formLayout->setSpacing(12);

    QLabel *formTitle = new QLabel("✍ Rate & Review This Book (Verified Buyer)");
    formTitle->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 700;").arg(AppStyle::TextPrimary));
    formLayout->addWidget(formTitle);

    // Star Selection Row
    QHBoxLayout *starLayout = new QHBoxLayout();
    starLayout->setSpacing(6);
    QLabel *starLabel = new QLabel("Your Rating:");
    starLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600;").arg(AppStyle::TextSecondary));
    starLayout->addWidget(starLabel);

    starButtons_.clear();
    for (int i = 1; i <= 5; ++i) {
        QPushButton *sBtn = new QPushButton("★");
        sBtn->setFixedSize(32, 32);
        sBtn->setCursor(Qt::PointingHandCursor);
        sBtn->setStyleSheet(QString(R"(
            QPushButton {
                background: transparent;
                border: none;
                font-size: 22px;
                color: #D97706;
            }
            QPushButton:hover {
                color: #F59E0B;
            }
        )"));
        connect(sBtn, &QPushButton::clicked, this, [this, i]() {
            handleSetRating(i);
        });
        starButtons_.append(sBtn);
        starLayout->addWidget(sBtn);
    }
    starLayout->addStretch();
    formLayout->addLayout(starLayout);

    // Comment Input
    reviewCommentEdit_ = new QTextEdit();
    reviewCommentEdit_->setPlaceholderText("Write your honest review about book condition, packaging, or reading experience...");
    reviewCommentEdit_->setFixedHeight(80);
    reviewCommentEdit_->setStyleSheet(QString(R"(
        QTextEdit {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
            padding: 8px 12px;
            font-size: 13px;
            color: %3;
        }
        QTextEdit:focus {
            border-color: %4;
            background-color: #FFFFFF;
        }
    )").arg(AppStyle::SurfaceSubtle, AppStyle::BorderSubtle, AppStyle::TextPrimary, AppStyle::Primary));
    formLayout->addWidget(reviewCommentEdit_);

    // Submit Button
    QHBoxLayout *submitRow = new QHBoxLayout();
    submitRow->addStretch();
    submitReviewBtn_ = new QPushButton("Submit Verified Review");
    submitReviewBtn_->setCursor(Qt::PointingHandCursor);
    submitReviewBtn_->setStyleSheet(AppStyle::primaryButtonStyle());
    submitReviewBtn_->setFixedHeight(36);
    connect(submitReviewBtn_, &QPushButton::clicked, this, &BookDetailsWindow::handleSubmitReview);
    submitRow->addWidget(submitReviewBtn_);
    formLayout->addLayout(submitRow);

    layout->addWidget(reviewFormCard_);

    // Buyer Notice (shown if not verified buyer)
    buyerNoticeLabel_ = new QLabel("Verified buyers can leave a star rating and written review after purchasing this book.");
    buyerNoticeLabel_->setStyleSheet(QString(R"(
        background-color: %1;
        color: %2;
        border: 1px solid %3;
        border-radius: 8px;
        padding: 10px 16px;
        font-size: 12.5px;
    )").arg(AppStyle::InfoLight, AppStyle::InfoText, AppStyle::InfoBorder));
    layout->addWidget(buyerNoticeLabel_);

    // Reviews List Card
    QFrame *reviewsCard = new QFrame();
    reviewsCard->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(reviewsCard, 20, 6, 18);

    reviewsLayout = new QVBoxLayout(reviewsCard);
    reviewsLayout->setContentsMargins(24, 24, 24, 24);
    reviewsLayout->setSpacing(14);

    QLabel *empty = new QLabel("Loading reviews...");
    empty->setStyleSheet(QString("color: %1; font-size: 13px;").arg(AppStyle::TextMuted));
    reviewsLayout->addWidget(empty);

    layout->addWidget(reviewsCard);
    return container;
}

void BookDetailsWindow::handleSetRating(int stars)
{
    selectedRating_ = stars;
    for (int i = 0; i < starButtons_.size(); ++i) {
        if (i < stars) {
            starButtons_[i]->setText("★");
            starButtons_[i]->setStyleSheet("background: transparent; border: none; font-size: 22px; color: #D97706;");
        } else {
            starButtons_[i]->setText("☆");
            starButtons_[i]->setStyleSheet(QString("background: transparent; border: none; font-size: 22px; color: %1;").arg(AppStyle::TextMuted));
        }
    }
}

void BookDetailsWindow::handleSubmitReview()
{
    if (buyerOrderId_.isEmpty()) {
        StyledMessageBox::warning(this, "Review Not Allowed", "You must have an order for this book to submit a review.");
        return;
    }

    const QString comment = reviewCommentEdit_ ? reviewCommentEdit_->toPlainText().trimmed() : QString();
    if (comment.isEmpty()) {
        StyledMessageBox::warning(this, "Empty Comment", "Please write a brief comment describing your experience.");
        return;
    }

    submitReviewBtn_->setEnabled(false);
    submitReviewBtn_->setText("Submitting...");

    QJsonObject body;
    body[QStringLiteral("orderId")] = buyerOrderId_;
    body[QStringLiteral("bookId")] = bookId;
    body[QStringLiteral("rating")] = selectedRating_;
    body[QStringLiteral("comment")] = comment;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/reviews"), body, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        submitReviewBtn_->setEnabled(true);
        submitReviewBtn_->setText("Submit Verified Review");

        if (ok) {
            StyledMessageBox::success(this, "Review Published", "Thank you! Your verified review has been submitted successfully.");
            if (reviewCommentEdit_) reviewCommentEdit_->clear();
            loadReviews();
            loadBook();
        } else {
            StyledMessageBox::critical(this, "Submission Failed", errorMsg.isEmpty() ? "Unable to submit your review." : errorMsg);
        }
    });
}

void BookDetailsWindow::checkBuyerStatus()
{
    if (!SessionManager::instance().isLoggedIn()) {
        hasPurchased_ = false;
        if (reviewFormCard_) reviewFormCard_->setVisible(false);
        if (buyerNoticeLabel_) buyerNoticeLabel_->setVisible(true);
        return;
    }

    auto *reply = ApiClient::instance().get(QStringLiteral("/api/orders?limit=50"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        hasPurchased_ = false;
        buyerOrderId_.clear();

        if (ok && json.contains(QStringLiteral("orders")) && json[QStringLiteral("orders")].isArray()) {
            const auto ordersArr = json[QStringLiteral("orders")].toArray();
            for (const auto &ordVal : ordersArr) {
                const auto ordObj = ordVal.toObject();
                const QString ordId = ordObj[QStringLiteral("id")].toString();
                if (ordObj.contains(QStringLiteral("items")) && ordObj[QStringLiteral("items")].isArray()) {
                    for (const auto &itemVal : ordObj[QStringLiteral("items")].toArray()) {
                        const auto itemObj = itemVal.toObject();
                        if (itemObj[QStringLiteral("bookId")].toString() == bookId) {
                            hasPurchased_ = true;
                            buyerOrderId_ = ordId;
                            break;
                        }
                    }
                }
                if (hasPurchased_) break;
            }
        }

        if (reviewFormCard_) reviewFormCard_->setVisible(hasPurchased_);
        if (buyerNoticeLabel_) buyerNoticeLabel_->setVisible(!hasPurchased_);
    });
}

void BookDetailsWindow::setupShortcuts()
{
    setFocusPolicy(Qt::StrongFocus);
    if (qApp) {
        qApp->installEventFilter(this);
    }
    if (previewBox_) previewBox_->installEventFilter(this);
    if (coverLabel) coverLabel->installEventFilter(this);

    auto *leftSc = new QShortcut(QKeySequence(Qt::Key_Left), this, nullptr, nullptr, Qt::WindowShortcut);
    auto *rightSc = new QShortcut(QKeySequence(Qt::Key_Right), this, nullptr, nullptr, Qt::WindowShortcut);
    auto *keyASc = new QShortcut(QKeySequence(Qt::Key_A), this, nullptr, nullptr, Qt::WindowShortcut);
    auto *keyDSc = new QShortcut(QKeySequence(Qt::Key_D), this, nullptr, nullptr, Qt::WindowShortcut);
    auto *commaSc = new QShortcut(QKeySequence(Qt::Key_Comma), this, nullptr, nullptr, Qt::WindowShortcut);
    auto *periodSc = new QShortcut(QKeySequence(Qt::Key_Period), this, nullptr, nullptr, Qt::WindowShortcut);
    auto *spaceSc = new QShortcut(QKeySequence(Qt::Key_Space), this, nullptr, nullptr, Qt::WindowShortcut);
    auto *escSc = new QShortcut(QKeySequence(Qt::Key_Escape), this, nullptr, nullptr, Qt::WindowShortcut);

    connect(leftSc, &QShortcut::activated, this, &BookDetailsWindow::showPreviousMedia);
    connect(rightSc, &QShortcut::activated, this, &BookDetailsWindow::showNextMedia);
    connect(keyASc, &QShortcut::activated, this, &BookDetailsWindow::showPreviousMedia);
    connect(keyDSc, &QShortcut::activated, this, &BookDetailsWindow::showNextMedia);
    connect(commaSc, &QShortcut::activated, this, &BookDetailsWindow::showPreviousMedia);
    connect(periodSc, &QShortcut::activated, this, &BookDetailsWindow::showNextMedia);
    connect(escSc, &QShortcut::activated, this, &BookDetailsWindow::handleBack);
    connect(spaceSc, &QShortcut::activated, this, [this]() {
        if (currentMediaIndex_ >= 0 && currentMediaIndex_ < mediaItems_.size()) {
            if (mediaItems_[currentMediaIndex_].isVideo && videoPlayer_) {
                if (videoPlayer_->isPlaying()) videoPlayer_->pause();
                else videoPlayer_->play();
            }
        }
    });

    auto *inspectSc = new QShortcut(QKeySequence(Qt::Key_I), this, nullptr, nullptr, Qt::WindowShortcut);
    connect(inspectSc, &QShortcut::activated, this, [this]() {
        if (currentMediaIndex_ >= 0 && currentMediaIndex_ < mediaItems_.size() && !mediaItems_[currentMediaIndex_].isVideo) {
            openImageInspection();
        }
    });

    auto *refreshSc = new QShortcut(QKeySequence::Refresh, this, nullptr, nullptr, Qt::WindowShortcut);
    connect(refreshSc, &QShortcut::activated, this, &BookDetailsWindow::loadBook);
}

void BookDetailsWindow::updatePreviewOverlayPositions()
{
    if (!previewBox_) return;

    const int w = previewBox_->width();
    const int h = previewBox_->height();

    if (mediaCounterBadge_) {
        mediaCounterBadge_->adjustSize();
        mediaCounterBadge_->move(12, 12);
        mediaCounterBadge_->raise();
    }
    if (prevMediaBtn_) {
        prevMediaBtn_->move(10, (h - prevMediaBtn_->height()) / 2);
        prevMediaBtn_->raise();
    }
    if (nextMediaBtn_) {
        nextMediaBtn_->move(w - nextMediaBtn_->width() - 10, (h - nextMediaBtn_->height()) / 2);
        nextMediaBtn_->raise();
    }
    if (inspectBtn_) {
        inspectBtn_->adjustSize();
        inspectBtn_->move(w - inspectBtn_->width() - 10, h - inspectBtn_->height() - 10);
        inspectBtn_->raise();
    }
}

void BookDetailsWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updatePreviewOverlayPositions();
}

void BookDetailsWindow::keyPressEvent(QKeyEvent *event)
{
    QWidget *focused = QApplication::focusWidget();
    const bool inText = focused && (
        qobject_cast<QLineEdit*>(focused) ||
        qobject_cast<QTextEdit*>(focused) ||
        qobject_cast<QPlainTextEdit*>(focused) ||
        qobject_cast<QComboBox*>(focused)
    );

    if (!inText) {
        const int key = event->key();
        if (key == Qt::Key_Left || key == Qt::Key_A || key == Qt::Key_Less || key == Qt::Key_Comma) {
            showPreviousMedia();
            event->accept();
            return;
        }
        if (key == Qt::Key_Right || key == Qt::Key_D || key == Qt::Key_Greater || key == Qt::Key_Period) {
            showNextMedia();
            event->accept();
            return;
        }
        if (key == Qt::Key_Space) {
            if (currentMediaIndex_ >= 0 && currentMediaIndex_ < mediaItems_.size() &&
                mediaItems_[currentMediaIndex_].isVideo && videoPlayer_) {
                if (videoPlayer_->isPlaying()) videoPlayer_->pause();
                else videoPlayer_->play();
                event->accept();
                return;
            }
        }
        if (key == Qt::Key_M) {
            if (currentMediaIndex_ >= 0 && currentMediaIndex_ < mediaItems_.size() &&
                mediaItems_[currentMediaIndex_].isVideo && videoPlayer_) {
                videoPlayer_->onMuteToggled();
                event->accept();
                return;
            }
        }
        if (key == Qt::Key_R) {
            if (currentMediaIndex_ >= 0 && currentMediaIndex_ < mediaItems_.size() &&
                mediaItems_[currentMediaIndex_].isVideo && videoPlayer_) {
                videoPlayer_->onReplayClicked();
                event->accept();
                return;
            }
        }
        if (key == Qt::Key_Escape || key == Qt::Key_Backspace) {
            handleBack();
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void BookDetailsWindow::showPreviousMedia()
{
    if (mediaItems_.isEmpty()) return;
    const int prev = (currentMediaIndex_ - 1 + mediaItems_.size()) % mediaItems_.size();
    selectGalleryMedia(prev);
}

void BookDetailsWindow::showNextMedia()
{
    if (mediaItems_.isEmpty()) return;
    const int next = (currentMediaIndex_ + 1) % mediaItems_.size();
    selectGalleryMedia(next);
}

void BookDetailsWindow::updateGalleryUI()
{
    // Clear existing thumbnails
    while (thumbnailsLayout && thumbnailsLayout->count() > 0) {
        auto *item = thumbnailsLayout->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }
    thumbnailBtns.clear();
    mediaItems_.clear();

    // 1. Build unified media items list: Video first if present, then book images
    const QString vidUrl = book.videoUrl.trimmed();
    currentVideoUrl_ = vidUrl;
    if (!vidUrl.isEmpty()) {
        mediaItems_.append({true, vidUrl});
    }

    QStringList imageList = book.images;
    if (imageList.isEmpty() && !book.coverImage.isEmpty()) {
        imageList.append(book.coverImage);
    }
    for (const auto &img : imageList) {
        if (!img.trimmed().isEmpty()) {
            mediaItems_.append({false, img.trimmed()});
        }
    }

    if (mediaItems_.isEmpty()) {
        mediaItems_.append({false, QString()});
    }

    // 2. Build thumbnails strip
    for (int i = 0; i < mediaItems_.size(); ++i) {
        const auto &item = mediaItems_[i];
        QPushButton *btn = new QPushButton();
        btn->setFixedSize(56, 72);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: #FFFFFF;
                border: 2px solid %1;
                border-radius: 8px;
            }
            QPushButton:hover {
                border-color: %2;
            }
        )").arg(AppStyle::BorderSubtle, AppStyle::Primary));

        QLabel *lbl = new QLabel(btn);
        lbl->setGeometry(2, 2, 52, 68);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("background: transparent; border-radius: 6px;");

        if (item.isVideo) {
            VideoThumbnailHelper::loadThumbnailAsync(item.url, lbl, QSize(52, 68), book.title, book.coverImage);
        } else {
            ImageLoader::instance().load(item.url, lbl, QSize(52, 68), book.title, book.author);
        }

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            selectGalleryMedia(i);
        });

        thumbnailsLayout->addWidget(btn);
        thumbnailBtns.append(btn);
    }

    thumbnailsLayout->addStretch();
    selectGalleryMedia(0);
}

void BookDetailsWindow::selectGalleryMedia(int index)
{
    if (mediaItems_.isEmpty()) return;
    if (index < 0) index = 0;
    if (index >= mediaItems_.size()) index = mediaItems_.size() - 1;

    currentMediaIndex_ = index;
    const auto &item = mediaItems_[index];

    // Highlight active thumbnail button
    for (int i = 0; i < thumbnailBtns.size(); ++i) {
        if (i == index) {
            thumbnailBtns[i]->setStyleSheet(QString(R"(
                QPushButton {
                    background-color: %1;
                    border: 2.5px solid %2;
                    border-radius: 8px;
                }
            )").arg(item.isVideo ? "#0F172A" : "#FFFFFF", AppStyle::Primary));
        } else {
            thumbnailBtns[i]->setStyleSheet(QString(R"(
                QPushButton {
                    background-color: %1;
                    border: 1.5px solid %2;
                    border-radius: 8px;
                }
            )").arg(item.isVideo ? "#1E293B" : "#FFFFFF", AppStyle::BorderSubtle));
        }
    }

    if (thumbScroll_ && index >= 0 && index < thumbnailBtns.size()) {
        thumbScroll_->ensureWidgetVisible(thumbnailBtns[index]);
    }

    // Update < > buttons and counter badge
    const bool hasMultiple = mediaItems_.size() > 1;
    if (prevMediaBtn_) prevMediaBtn_->setVisible(hasMultiple);
    if (nextMediaBtn_) nextMediaBtn_->setVisible(hasMultiple);
    if (mediaCounterBadge_) {
        mediaCounterBadge_->setVisible(hasMultiple);
        if (item.isVideo) {
            mediaCounterBadge_->setText(QString("🎬 Video  %1 / %2").arg(index + 1).arg(mediaItems_.size()));
        } else {
            mediaCounterBadge_->setText(QString("%1 / %2").arg(index + 1).arg(mediaItems_.size()));
        }
    }

    if (item.isVideo) {
        // Switch to Native Embedded Video Player
        if (coverLabel) {
            coverLabel->clear();
            coverLabel->setVisible(false);
        }
        if (inspectBtn_) {
            inspectBtn_->setText("⤢ Full Video Preview");
            inspectBtn_->setVisible(true);
        }
        if (mediaStack_ && videoPlayer_) {
            mediaStack_->setCurrentWidget(videoPlayer_);
            videoPlayer_->setVisible(true);
            videoPlayer_->setVideoSource(item.url, book.title, book.coverImage);
        }
        if (playVideoBtn) playVideoBtn->setVisible(false);
    } else {
        // Switch to Image Cover
        if (videoPlayer_) {
            videoPlayer_->stop();
            videoPlayer_->setVisible(false);
        }
        if (mediaStack_ && coverLabel) {
            coverLabel->clear();
            coverLabel->update();
            coverLabel->setVisible(true);
            mediaStack_->setCurrentWidget(coverLabel);
            coverLabel->setCursor(Qt::PointingHandCursor);
            ImageLoader::instance().load(item.url, coverLabel, QSize(320, 330), book.title, book.author);
        }
        if (inspectBtn_) {
            inspectBtn_->setText("🔍 Inspect Full Size");
            inspectBtn_->setVisible(true);
        }
        if (playVideoBtn) playVideoBtn->setVisible(false);
    }

    // Always reposition and raise overlays AFTER active stack widget switch!
    updatePreviewOverlayPositions();
    if (prevMediaBtn_) prevMediaBtn_->raise();
    if (nextMediaBtn_) nextMediaBtn_->raise();
    if (mediaCounterBadge_) mediaCounterBadge_->raise();
    if (inspectBtn_) inspectBtn_->raise();
}

bool BookDetailsWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == previewBox_ && event->type() == QEvent::Resize) {
        updatePreviewOverlayPositions();
    }

    if (event->type() == QEvent::KeyPress) {
        if (isVisible()) {
            QKeyEvent *ke = static_cast<QKeyEvent *>(event);
            QWidget *focused = QApplication::focusWidget();
            const bool inText = focused && (
                qobject_cast<QLineEdit*>(focused) ||
                qobject_cast<QTextEdit*>(focused) ||
                qobject_cast<QPlainTextEdit*>(focused) ||
                qobject_cast<QComboBox*>(focused)
            );

            if (!inText) {
                const int key = ke->key();
                if (key == Qt::Key_Left || key == Qt::Key_A || key == Qt::Key_Less || key == Qt::Key_Comma) {
                    showPreviousMedia();
                    return true;
                }
                if (key == Qt::Key_Right || key == Qt::Key_D || key == Qt::Key_Greater || key == Qt::Key_Period) {
                    showNextMedia();
                    return true;
                }
                if (key == Qt::Key_Space) {
                    if (currentMediaIndex_ >= 0 && currentMediaIndex_ < mediaItems_.size() &&
                        mediaItems_[currentMediaIndex_].isVideo && videoPlayer_) {
                        if (videoPlayer_->isPlaying()) videoPlayer_->pause();
                        else videoPlayer_->play();
                        return true;
                    }
                }
                if (key == Qt::Key_M) {
                    if (currentMediaIndex_ >= 0 && currentMediaIndex_ < mediaItems_.size() &&
                        mediaItems_[currentMediaIndex_].isVideo && videoPlayer_) {
                        videoPlayer_->onMuteToggled();
                        return true;
                    }
                }
                if (key == Qt::Key_R) {
                    if (currentMediaIndex_ >= 0 && currentMediaIndex_ < mediaItems_.size() &&
                        mediaItems_[currentMediaIndex_].isVideo && videoPlayer_) {
                        videoPlayer_->onReplayClicked();
                        return true;
                    }
                }
                if (key == Qt::Key_Escape || key == Qt::Key_Backspace) {
                    handleBack();
                    return true;
                }
            }
        }
    }

    if (obj == coverLabel && event->type() == QEvent::MouseButtonPress) {
        if (currentMediaIndex_ >= 0 && currentMediaIndex_ < mediaItems_.size()) {
            if (mediaItems_[currentMediaIndex_].isVideo) {
                handlePlayVideo();
                return true;
            } else {
                openImageInspection();
                return true;
            }
        }
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        if (obj && obj->property("fullImageUrl").isValid()) {
            QString url = obj->property("fullImageUrl").toString();
            QDialog previewDlg(this);
            previewDlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
            previewDlg.setAttribute(Qt::WA_TranslucentBackground);
            previewDlg.setModal(true);

            QVBoxLayout *rootLayout = new QVBoxLayout(&previewDlg);
            rootLayout->setContentsMargins(16, 16, 16, 16);

            QFrame *card = new QFrame(&previewDlg);
            card->setStyleSheet(R"(
                background-color: #0F172A;
                border: 1px solid #334155;
                border-radius: 16px;
            )");
            AppStyle::applyElevation(card, 24, 8, 25);

            QVBoxLayout *cardLayout = new QVBoxLayout(card);
            cardLayout->setContentsMargins(16, 12, 16, 16);
            cardLayout->setSpacing(12);

            QHBoxLayout *topRow = new QHBoxLayout();
            QLabel *title = new QLabel("Delivered Condition Photo", card);
            title->setStyleSheet("color: #FFFFFF; font-size: 15px; font-weight: 700;");
            topRow->addWidget(title);
            topRow->addStretch();

            QPushButton *closeBtn = new QPushButton("✕", card);
            closeBtn->setCursor(Qt::PointingHandCursor);
            closeBtn->setFixedSize(28, 28);
            closeBtn->setStyleSheet(R"(
                QPushButton {
                    background-color: rgba(255, 255, 255, 0.1);
                    color: #FFFFFF;
                    border: none;
                    border-radius: 14px;
                    font-size: 13px;
                    font-weight: bold;
                }
                QPushButton:hover {
                    background-color: rgba(255, 255, 255, 0.2);
                }
            )");
            connect(closeBtn, &QPushButton::clicked, &previewDlg, &QDialog::accept);
            topRow->addWidget(closeBtn);
            cardLayout->addLayout(topRow);

            QLabel *imgLbl = new QLabel(card);
            imgLbl->setAlignment(Qt::AlignCenter);
            imgLbl->setStyleSheet("background-color: #070A11; border-radius: 8px;");
            ImageLoader::instance().load(url, imgLbl, QSize(520, 520));
            cardLayout->addWidget(imgLbl);

            rootLayout->addWidget(card);
            previewDlg.adjustSize();
            previewDlg.exec();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void BookDetailsWindow::openImageInspection()
{
    if (mediaItems_.isEmpty()) return;

    QVector<InspectionMediaItem> inspectionList;
    for (const auto &item : mediaItems_) {
        inspectionList.append({item.isVideo, item.url, book.title});
    }

    ImageInspectionDialog dlg(inspectionList, currentMediaIndex_, book.title, this);
    dlg.exec();
}

void BookDetailsWindow::handlePlayVideo()
{
    if (videoPlayer_) {
        videoPlayer_->play();
    }
}

void BookDetailsWindow::loadBook()
{
    auto *reply = ApiClient::instance().getPublic(QStringLiteral("/api/books/") + bookId);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!ok || !json.contains(QStringLiteral("book"))) {
            StyledMessageBox::critical(this, "Book Not Found", errorMsg.isEmpty() ? "The selected book could not be found." : errorMsg);
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
        descriptionLabel->setText(book.description.isEmpty() ? "No description provided." : book.description);

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

        if (!book.owner.id.isEmpty()) {
            auto *sRep = ApiClient::instance().getPublic(QStringLiteral("/api/reviews/seller/") + book.owner.id);
            connect(sRep, &QNetworkReply::finished, this, [this, sRep]() {
                auto [sOk, sJson, sErr] = ApiClient::parseReply(sRep);
                sRep->deleteLater();
                if (sOk && sJson.contains(QStringLiteral("averageRating"))) {
                    double sAvg = sJson.value(QStringLiteral("averageRating")).toDouble(0.0);
                    int sCount = sJson.value(QStringLiteral("reviewCount")).toInt(0);
                    auto cards = findChildren<QFrame*>();
                    for (auto *c : cards) {
                        auto labels = c->findChildren<QLabel*>();
                        if (labels.size() >= 2 && labels[0]->text() == "LISTED BY") {
                            if (sCount > 0) {
                                labels[1]->setText(QString("%1  ★%2 (%3)").arg(book.owner.username).arg(sAvg, 0, 'f', 1).arg(sCount));
                            }
                        }
                    }
                }
            });
        }

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

        updateGalleryUI();
    });
}

void BookDetailsWindow::loadReviews()
{
    auto *reply = ApiClient::instance().getPublic(QStringLiteral("/api/reviews/book/") + bookId);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        // Clear existing reviews
        QLayoutItem *child;
        while ((child = reviewsLayout->takeAt(0)) != nullptr) {
            if (child->widget()) delete child->widget();
            delete child;
        }
        reviews.clear();

        if (ok && json.contains(QStringLiteral("reviews"))) {
            const auto arr = json[QStringLiteral("reviews")].toArray();
            for (const auto &v : arr) {
                reviews.append(ReviewModel::fromJson(v.toObject()));
            }
        }

        // If sellerId is available, also fetch seller reviews
        QString sellerId = book.owner.id;
        if (!sellerId.isEmpty()) {
            auto *sReply = ApiClient::instance().getPublic(QStringLiteral("/api/reviews/seller/") + sellerId);
            connect(sReply, &QNetworkReply::finished, this, [this, sReply]() {
                auto [sOk, sJson, sErrorMsg] = ApiClient::parseReply(sReply);
                sReply->deleteLater();

                QVector<ReviewModel> sellerReviews;
                double sellerAvgRating = 0.0;
                int sellerReviewCount = 0;

                if (sOk) {
                    sellerAvgRating = sJson.value(QStringLiteral("averageRating")).toDouble(0.0);
                    sellerReviewCount = sJson.value(QStringLiteral("reviewCount")).toInt(0);
                    if (sJson.contains(QStringLiteral("reviews")) && sJson[QStringLiteral("reviews")].isArray()) {
                        const auto sArr = sJson[QStringLiteral("reviews")].toArray();
                        for (const auto &v : sArr) {
                            ReviewModel rm = ReviewModel::fromJson(v.toObject());
                            bool exists = false;
                            for (const auto &r : reviews) {
                                if (r.id == rm.id) { exists = true; break; }
                            }
                            if (!exists) sellerReviews.append(rm);
                        }
                    }
                }

                renderReviewsList(sellerReviews, sellerAvgRating, sellerReviewCount);
            });
        } else {
            renderReviewsList(QVector<ReviewModel>(), 0.0, 0);
        }
    });
}

void BookDetailsWindow::renderReviewsList(const QVector<ReviewModel> &sellerReviews, double sellerAvgRating, int sellerReviewCount)
{
    // Clear existing reviews layout
    QLayoutItem *child;
    while ((child = reviewsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    if (sellerReviewCount > 0) {
        QFrame *sellerBanner = new QFrame();
        sellerBanner->setStyleSheet(QString(R"(
            QFrame {
                background-color: #EEF2FF;
                border: 1px solid #C7D2FE;
                border-radius: 12px;
            }
        )"));
        QHBoxLayout *sbLayout = new QHBoxLayout(sellerBanner);
        sbLayout->setContentsMargins(16, 12, 16, 12);
        sbLayout->setSpacing(12);

        QLabel *starIcon = new QLabel(QString("★ %1").arg(sellerAvgRating, 0, 'f', 1));
        starIcon->setStyleSheet("color: #4F46E5; font-size: 17px; font-weight: 800;");

        QLabel *infoTxt = new QLabel(QString("Seller Reputation • %1 verified buyer rating%2 for %3")
                                     .arg(sellerReviewCount)
                                     .arg(sellerReviewCount == 1 ? "" : "s")
                                     .arg(book.owner.username.isEmpty() ? "this seller" : book.owner.username));
        infoTxt->setStyleSheet("color: #312E81; font-size: 13px; font-weight: 700;");

        sbLayout->addWidget(starIcon);
        sbLayout->addWidget(infoTxt, 1);
        reviewsLayout->addWidget(sellerBanner);
    }

    QVector<ReviewModel> allReviews = reviews;
    for (const auto &sr : sellerReviews) {
        allReviews.append(sr);
    }

    if (allReviews.isEmpty()) {
        QLabel *empty = new QLabel("No reader reviews yet. Verified buyers will be able to leave reviews and condition photos here upon delivery!");
        empty->setStyleSheet(QString("color: %1; font-size: 13px; padding: 12px 0;").arg(AppStyle::TextMuted));
        reviewsLayout->addWidget(empty);
        return;
    }

    for (const auto &r : allReviews) {
        QFrame *item = new QFrame();
        item->setStyleSheet(QString(R"(
            QFrame {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 12px;
            }
        )").arg(AppStyle::SurfaceSubtle, AppStyle::BorderSubtle));

        QVBoxLayout *iLayout = new QVBoxLayout(item);
        iLayout->setContentsMargins(16, 14, 16, 14);
        iLayout->setSpacing(8);

        QHBoxLayout *top = new QHBoxLayout();
        QLabel *userLbl = new QLabel(r.reviewerUsername.isEmpty() ? "Anonymous Buyer" : "👤 " + r.reviewerUsername);
        userLbl->setStyleSheet(QString("color: %1; font-size: 13.5px; font-weight: 700;").arg(AppStyle::TextPrimary));

        QLabel *verifiedBadge = new QLabel("✓ Verified Buyer");
        verifiedBadge->setStyleSheet(QString(R"(
            background-color: %1;
            color: %2;
            border-radius: 4px;
            padding: 2px 6px;
            font-size: 10px;
            font-weight: 700;
        )").arg(AppStyle::SuccessLight, AppStyle::SuccessText));

        QString stars;
        for (int i = 0; i < 5; ++i) {
            stars += (i < r.rating) ? "★" : "☆";
        }
        QLabel *starsLbl = new QLabel(stars);
        starsLbl->setStyleSheet("color: #D97706; font-size: 13px; font-weight: 700;");

        QLabel *dateLbl = new QLabel(r.createdAt.toString("MMM d, yyyy"));
        dateLbl->setStyleSheet(QString("color: %1; font-size: 11px;").arg(AppStyle::TextMuted));

        top->addWidget(userLbl);
        top->addWidget(verifiedBadge);
        top->addSpacing(8);
        top->addWidget(starsLbl);
        top->addStretch();
        top->addWidget(dateLbl);
        iLayout->addLayout(top);

        QLabel *commentLbl = new QLabel(r.comment);
        commentLbl->setWordWrap(true);
        commentLbl->setStyleSheet(QString("color: %1; font-size: 13px; line-height: 1.4;").arg(AppStyle::TextSecondary));
        iLayout->addWidget(commentLbl);

        if (!r.images.isEmpty()) {
            QHBoxLayout *imgsRow = new QHBoxLayout();
            imgsRow->setSpacing(8);
            QLabel *photosTag = new QLabel("Received Condition:", item);
            photosTag->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 700;").arg(AppStyle::TextMuted));
            imgsRow->addWidget(photosTag);

            for (const auto &imgUrl : r.images) {
                QLabel *thumb = new QLabel(item);
                thumb->setFixedSize(56, 56);
                thumb->setCursor(Qt::PointingHandCursor);
                thumb->setStyleSheet("border: 1px solid #CBD5E1; border-radius: 6px; background-color: #F8FAFC;");
                ImageLoader::instance().load(imgUrl, thumb, QSize(52, 52));
                thumb->installEventFilter(this);
                thumb->setProperty("fullImageUrl", imgUrl);
                imgsRow->addWidget(thumb);
            }
            imgsRow->addStretch();
            iLayout->addLayout(imgsRow);
        }

        reviewsLayout->addWidget(item);
    }
}

void BookDetailsWindow::handleOrder()
{
    if (!SessionManager::instance().isLoggedIn()) {
        StyledMessageBox::warning(this, "Authentication Required", "Please log in to add books to your cart.");
        return;
    }

    orderButton->setEnabled(false);
    orderButton->setText("Adding...");

    QJsonObject body;
    body[QStringLiteral("bookId")] = bookId;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/cart/items"), body, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        orderButton->setEnabled(true);
        orderButton->setText("Add to Cart");

        if (ok) {
            StyledMessageBox::success(
                this,
                "Added to Cart",
                QString("<b>%1</b> has been added to your shopping cart!").arg(book.title)
            );
            emit addToCartRequested(bookId);
        } else {
            StyledMessageBox::critical(
                this,
                "Cart Error",
                errorMsg.isEmpty() ? "Could not add this book to cart." : errorMsg
            );
        }
    });
}

void BookDetailsWindow::handleProposeExchange()
{
    if (!SessionManager::instance().isLoggedIn()) {
        StyledMessageBox::information(this, "Sign In Required", "Please sign in to your account to propose a book exchange.");
        return;
    }

    if (book.owner.id == SessionManager::instance().userId()) {
        StyledMessageBox::warning(this, "Cannot Exchange", "You cannot propose an exchange for your own listed book.");
        return;
    }

    // Strictly fetch available listings belonging to the user from the backend
    QUrlQuery query;
    query.addQueryItem("status", "available");
    query.addQueryItem("limit", "50");

    auto *reply = ApiClient::instance().get(QStringLiteral("/api/books/my"), query);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!ok) {
            StyledMessageBox::critical(this, "Inventory Check Failed", errorMsg.isEmpty() ? "Could not retrieve your listings." : errorMsg);
            return;
        }

        QJsonArray booksArr = json[QStringLiteral("books")].toArray();
        QVector<BookModel> userBooks;
        for (const auto &val : booksArr) {
            userBooks.append(BookModel::fromJson(val.toObject()));
        }

        if (userBooks.isEmpty()) {
            StyledMessageBox::warning(
                this,
                "No Books Listed For Exchange",
                "You must have at least one active book listed in your inventory to propose an exchange.\n\nPlease list a book first before sending an exchange proposal."
            );
            return;
        }

        ProposeExchangeDialog dlg(book, userBooks, this);
        if (dlg.exec() != QDialog::Accepted) {
            return;
        }

        QString offeredBookId = dlg.selectedOfferedBookId();
        QString message = dlg.proposalMessage();

        if (offeredBookId.isEmpty()) {
            StyledMessageBox::warning(this, "Selection Required", "Please select one of your books to offer for exchange.");
            return;
        }

        QJsonObject payload;
        payload[QStringLiteral("requestedBookId")] = book.id;
        payload[QStringLiteral("offeredBookId")] = offeredBookId;
        if (!message.isEmpty()) {
            payload[QStringLiteral("message")] = message;
        }

        auto *postReply = ApiClient::instance().post(QStringLiteral("/api/exchanges"), payload, true);
        connect(postReply, &QNetworkReply::finished, this, [this, postReply]() {
            auto [postOk, postJson, postError] = ApiClient::parseReply(postReply);
            postReply->deleteLater();

            if (postOk) {
                StyledMessageBox::success(
                    this,
                    "Proposal Sent!",
                    "Your exchange proposal has been submitted to the book owner.\n\nYou can track it anytime under Peer-to-Peer Book Exchanges (Sent Proposals)."
                );
                emit exchangeRequested(book.id);
            } else {
                StyledMessageBox::warning(
                    this,
                    "Proposal Failed",
                    postError.isEmpty() ? "Failed to create exchange proposal." : postError
                );
            }
        });
    });
}

void BookDetailsWindow::handleBack()
{
    emit backRequested();
}