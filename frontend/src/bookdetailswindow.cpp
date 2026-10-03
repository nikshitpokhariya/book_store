#include "bookdetailswindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include "AppStyle.h"
#include "StyledMessageBox.h"
#include "ImageLoader.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QFrame>
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
      coverLabel(nullptr),
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

    setupUI();
    loadBook();
    loadReviews();
    checkBuyerStatus();
}

BookDetailsWindow::~BookDetailsWindow()
{
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
    QFrame *previewBox = new QFrame(leftCard);
    previewBox->setFixedHeight(320);
    previewBox->setStyleSheet(QString(R"(
        QFrame {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 12px;
        }
    )").arg(AppStyle::SurfaceSubtle, AppStyle::BorderSubtle));

    QVBoxLayout *previewLayout = new QVBoxLayout(previewBox);
    previewLayout->setContentsMargins(8, 8, 8, 8);
    previewLayout->setSpacing(8);

    coverLabel = new QLabel(previewBox);
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setStyleSheet("background: transparent; border: none;");
    coverLabel->installEventFilter(this);
    previewLayout->addWidget(coverLabel, 1);


    playVideoBtn = new QPushButton("▶ Play Video Preview", previewBox);
    playVideoBtn->setCursor(Qt::PointingHandCursor);
    playVideoBtn->setStyleSheet(AppStyle::dangerButtonStyle());
    playVideoBtn->setFixedHeight(36);
    playVideoBtn->setVisible(false);
    connect(playVideoBtn, &QPushButton::clicked, this, &BookDetailsWindow::handlePlayVideo);
    previewLayout->addWidget(playVideoBtn);

    leftLayout->addWidget(previewBox);

    // Thumbnails Row
    QLabel *galleryHeading = new QLabel("Media Gallery (Click to inspect):", leftCard);
    galleryHeading->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 700;").arg(AppStyle::TextMuted));
    leftLayout->addWidget(galleryHeading);

    QScrollArea *thumbScroll = new QScrollArea(leftCard);
    thumbScroll->setFixedHeight(86);
    thumbScroll->setWidgetResizable(true);
    thumbScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    thumbScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    thumbScroll->setFrameShape(QFrame::NoFrame);
    thumbScroll->setStyleSheet("background: transparent; border: none;");

    QWidget *thumbContainer = new QWidget();
    thumbContainer->setStyleSheet("background: transparent;");
    thumbnailsLayout = new QHBoxLayout(thumbContainer);
    thumbnailsLayout->setContentsMargins(0, 0, 0, 0);
    thumbnailsLayout->setSpacing(8);
    thumbScroll->setWidget(thumbContainer);

    leftLayout->addWidget(thumbScroll);

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

void BookDetailsWindow::updateGalleryUI()
{
    // Clear existing thumbnails
    while (thumbnailsLayout && thumbnailsLayout->count() > 0) {
        auto *item = thumbnailsLayout->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }
    thumbnailBtns.clear();

    QStringList imageList = book.images;
    if (imageList.isEmpty() && !book.coverImage.isEmpty()) {
        imageList.append(book.coverImage);
    }

    const bool hasVideo = !book.videoUrl.trimmed().isEmpty();
    currentVideoUrl_ = book.videoUrl.trimmed();

    int btnIndex = 0;

    // Flipkart / Amazon style: Video appears in the STARTING position (index 0) if available
    if (hasVideo) {
        QPushButton *vidBtn = new QPushButton();
        vidBtn->setFixedSize(60, 72);
        vidBtn->setCursor(Qt::PointingHandCursor);
        vidBtn->setText("🎬\nVideo");
        vidBtn->setStyleSheet(QString(R"(
            QPushButton {
                background-color: #0F172A;
                color: #FFFFFF;
                border: 2px solid %1;
                border-radius: 8px;
                font-size: 11px;
                font-weight: 800;
                line-height: 1.2;
            }
            QPushButton:hover {
                border-color: %2;
                background-color: #1E293B;
            }
        )").arg(AppStyle::Primary, AppStyle::Primary));

        connect(vidBtn, &QPushButton::clicked, this, [this]() {
            selectGalleryMedia(-1); // -1 signifies video
        });

        thumbnailsLayout->addWidget(vidBtn);
        thumbnailBtns.append(vidBtn);
        btnIndex++;
    }

    for (int i = 0; i < imageList.size(); ++i) {
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

        // Load thumbnail into icon
        QLabel *lbl = new QLabel(btn);
        lbl->setGeometry(2, 2, 52, 68);
        lbl->setAlignment(Qt::AlignCenter);
        ImageLoader::instance().load(imageList[i], lbl, QSize(52, 68), book.title, book.author);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            selectGalleryMedia(i);
        });

        thumbnailsLayout->addWidget(btn);
        thumbnailBtns.append(btn);
        btnIndex++;
    }

    thumbnailsLayout->addStretch();

    // If video is present, start with video preview at position 0, else first image
    if (hasVideo) {
        selectGalleryMedia(-1);
    } else {
        selectGalleryMedia(0);
    }
}

void BookDetailsWindow::selectGalleryMedia(int index)
{
    QStringList imageList = book.images;
    if (imageList.isEmpty() && !book.coverImage.isEmpty()) {
        imageList.append(book.coverImage);
    }

    const bool hasVideo = !currentVideoUrl_.isEmpty();
    const bool isVideoSelected = (index == -1);

    currentMediaIndex_ = index;

    // Highlight active thumbnail button
    int activeBtnIdx = isVideoSelected ? 0 : (hasVideo ? index + 1 : index);
    for (int i = 0; i < thumbnailBtns.size(); ++i) {
        if (i == activeBtnIdx) {
            thumbnailBtns[i]->setStyleSheet(QString(R"(
                QPushButton {
                    background-color: %1;
                    border: 2.5px solid %2;
                    border-radius: 8px;
                    color: #FFFFFF;
                }
            )").arg(i == 0 && hasVideo ? "#0F172A" : "#FFFFFF", AppStyle::Primary));
        } else {
            thumbnailBtns[i]->setStyleSheet(QString(R"(
                QPushButton {
                    background-color: %1;
                    border: 1.5px solid %2;
                    border-radius: 8px;
                    color: %3;
                }
            )").arg(i == 0 && hasVideo ? "#1E293B" : "#FFFFFF", AppStyle::BorderSubtle, i == 0 && hasVideo ? "#94A3B8" : AppStyle::TextPrimary));
        }
    }

    if (isVideoSelected) {
        // Video selected: Amazon/Flipkart style playable preview
        if (playVideoBtn) {
            playVideoBtn->setVisible(true);
            playVideoBtn->setText("▶ Play Book Video Preview");
        }

        QPixmap pix(320, 280);
        pix.fill(QColor("#0F172A"));
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);

        // Dark modern background with accent glow
        QRadialGradient glow(160, 120, 140);
        glow.setColorAt(0, QColor(79, 70, 229, 80));
        glow.setColorAt(1, QColor(15, 23, 42, 240));
        p.fillRect(0, 0, 320, 280, glow);

        // Circular Play Button
        p.setBrush(QColor(79, 70, 229));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(160, 110), 34, 34);

        // Play Triangle
        p.setBrush(Qt::white);
        QPolygon triangle;
        triangle << QPoint(153, 97) << QPoint(173, 110) << QPoint(153, 123);
        p.drawPolygon(triangle);

        // Text
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 13, QFont::Bold));
        p.drawText(QRect(10, 160, 300, 28), Qt::AlignCenter, "Video Preview (Verified Listing)");

        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Segoe UI", 10));
        p.drawText(QRect(10, 190, 300, 22), Qt::AlignCenter, "Click to play recorded clip of pages & condition");

        coverLabel->setPixmap(pix);
        coverLabel->setCursor(Qt::PointingHandCursor);
    } else if (index >= 0 && index < imageList.size()) {
        if (playVideoBtn) playVideoBtn->setVisible(false);
        coverLabel->setCursor(Qt::ArrowCursor);
        ImageLoader::instance().load(imageList[index], coverLabel, QSize(320, 280), book.title, book.author);
    }
}

bool BookDetailsWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == coverLabel && event->type() == QEvent::MouseButtonPress) {
        if (currentMediaIndex_ == -1 && !currentVideoUrl_.isEmpty()) {
            handlePlayVideo();
            return true;
        }
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        if (obj && obj->property("fullImageUrl").isValid()) {
            QString url = obj->property("fullImageUrl").toString();
            QDialog previewDlg(this);
            previewDlg.setWindowTitle("Delivered Condition Photo");
            previewDlg.resize(540, 540);
            previewDlg.setStyleSheet("background-color: #0F172A;");
            QVBoxLayout *vbox = new QVBoxLayout(&previewDlg);
            vbox->setContentsMargins(16, 16, 16, 16);
            QLabel *imgLbl = new QLabel(&previewDlg);
            imgLbl->setAlignment(Qt::AlignCenter);
            ImageLoader::instance().load(url, imgLbl, QSize(500, 500));
            vbox->addWidget(imgLbl);
            previewDlg.exec();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void BookDetailsWindow::handlePlayVideo()

{
    if (currentVideoUrl_.isEmpty()) return;

    const QString local = ImageLoader::resolveLocalPath(currentVideoUrl_);
    if (!local.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(local));
    } else {
        QDesktopServices::openUrl(QUrl(currentVideoUrl_));
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
    // Keep existing behavior or emit signal
    StyledMessageBox::information(this, "Exchange Request", "Exchange request flow initiated.");
}

void BookDetailsWindow::handleBack()
{
    emit backRequested();
}