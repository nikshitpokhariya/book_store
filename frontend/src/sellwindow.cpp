#include "sellwindow.h"
#include "AppStyle.h"
#include "StyledMessageBox.h"
#include "network/ApiClient.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QFrame>
#include <QScrollArea>
#include <QPixmap>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkReply>

namespace {
const QStringList SLOT_TITLES = {
    QStringLiteral("Cover (Required)"),
    QStringLiteral("Back Cover (Required)"),
    QStringLiteral("Spine / Angle (Required)"),
    QStringLiteral("Sample Page (Required)"),
    QStringLiteral("Extra Photo 1 (Optional)"),
    QStringLiteral("Extra Photo 2 (Optional)")
};
}

SellWindow::SellWindow(const QString &sellerName, QWidget *parent)
    : QWidget(parent),
      sellerName(sellerName),
      titleEdit(nullptr),
      authorEdit(nullptr),
      isbnEdit(nullptr),
      categoryCombo(nullptr),
      conditionCombo(nullptr),
      priceEdit(nullptr),
      descriptionEdit(nullptr),
      photoCountBadge(nullptr),
      videoCard(nullptr),
      videoStatusLabel(nullptr),
      chooseVideoBtn(nullptr),
      removeVideoBtn(nullptr),
      chooseAllImagesBtn(nullptr),
      sellButton(nullptr),
      backButton(nullptr)
{
    setWindowTitle("BookBazzar - Sell Your Book");
    resize(1200, 860);
    setMinimumSize(980, 680);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    setupUI();
}

SellWindow::~SellWindow()
{
}

void SellWindow::setupUI()
{
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ==========================================
    // Top Bar
    // ==========================================
    QFrame *topBar = new QFrame(this);
    topBar->setFixedHeight(72);
    topBar->setStyleSheet(QString(R"(
        QFrame {
            background-color: #FFFFFF;
            border-bottom: 1px solid %1;
        }
    )").arg(AppStyle::BorderSubtle));

    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(32, 0, 32, 0);
    topLayout->setSpacing(16);

    backButton = new QPushButton("← Back to Dashboard", topBar);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setStyleSheet(AppStyle::secondaryButtonStyle());
    backButton->setFixedHeight(40);
    connect(backButton, &QPushButton::clicked, this, &SellWindow::handleBack);

    QLabel *pageTitle = new QLabel("List a Book for Sale", topBar);
    pageTitle->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: 700;").arg(AppStyle::TextPrimary));

    topLayout->addWidget(backButton);
    topLayout->addWidget(pageTitle);
    topLayout->addStretch();

    rootLayout->addWidget(topBar);

    // ==========================================
    // Scroll Area for Content
    // ==========================================
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

    QWidget *content = new QWidget();
    content->setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));
    QHBoxLayout *contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(36, 28, 36, 40);
    contentLayout->setSpacing(28);

    // ==========================================
    // Left Column: Book Details Form Card
    // ==========================================
    QFrame *formCard = new QFrame(content);
    formCard->setStyleSheet(QString(R"(
        QFrame#formCard {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 16px;
        }
        QLabel {
            color: %2;
            font-size: 13px;
            font-weight: 600;
        }
    )").arg(AppStyle::BorderSubtle, AppStyle::TextSecondary));
    formCard->setObjectName("formCard");
    AppStyle::applyElevation(formCard, 16, 4, 15);

    QVBoxLayout *formLayout = new QVBoxLayout(formCard);
    formLayout->setContentsMargins(28, 28, 28, 28);
    formLayout->setSpacing(14);

    QLabel *formHeading = new QLabel("Book Information", formCard);
    formHeading->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: 700; margin-bottom: 6px;").arg(AppStyle::TextPrimary));
    formLayout->addWidget(formHeading);

    // Title
    formLayout->addWidget(new QLabel("Title *", formCard));
    titleEdit = new QLineEdit(formCard);
    titleEdit->setPlaceholderText("e.g. Clean Code, Introduction to Algorithms");
    titleEdit->setStyleSheet(AppStyle::inputStyle());
    formLayout->addWidget(titleEdit);

    // Author
    formLayout->addWidget(new QLabel("Author *", formCard));
    authorEdit = new QLineEdit(formCard);
    authorEdit->setPlaceholderText("e.g. Robert C. Martin");
    authorEdit->setStyleSheet(AppStyle::inputStyle());
    formLayout->addWidget(authorEdit);

    // Row: Category & Condition
    QHBoxLayout *rowCatCond = new QHBoxLayout();
    rowCatCond->setSpacing(14);

    QVBoxLayout *catBox = new QVBoxLayout();
    catBox->setSpacing(6);
    catBox->addWidget(new QLabel("Category *", formCard));
    categoryCombo = new QComboBox(formCard);
    categoryCombo->addItems({"Academic", "Engineering", "Fiction", "Non-Fiction", "Science", "Technology", "Business", "Self-Help", "Other"});
    categoryCombo->setStyleSheet(AppStyle::comboBoxStyle());
    catBox->addWidget(categoryCombo);
    rowCatCond->addLayout(catBox);

    QVBoxLayout *condBox = new QVBoxLayout();
    condBox->setSpacing(6);
    condBox->addWidget(new QLabel("Condition *", formCard));
    conditionCombo = new QComboBox(formCard);
    conditionCombo->addItems({"New", "Like New", "Very Good", "Good", "Acceptable"});
    conditionCombo->setStyleSheet(AppStyle::comboBoxStyle());
    condBox->addWidget(conditionCombo);
    rowCatCond->addLayout(condBox);

    formLayout->addLayout(rowCatCond);

    // Row: Price & ISBN
    QHBoxLayout *rowPriceIsbn = new QHBoxLayout();
    rowPriceIsbn->setSpacing(14);

    QVBoxLayout *priceBox = new QVBoxLayout();
    priceBox->setSpacing(6);
    priceBox->addWidget(new QLabel("Price (₹) *", formCard));
    priceEdit = new QLineEdit(formCard);
    priceEdit->setPlaceholderText("e.g. 450");
    priceEdit->setStyleSheet(AppStyle::inputStyle());
    priceBox->addWidget(priceEdit);
    rowPriceIsbn->addLayout(priceBox);

    QVBoxLayout *isbnBox = new QVBoxLayout();
    isbnBox->setSpacing(6);
    isbnBox->addWidget(new QLabel("ISBN (Optional)", formCard));
    isbnEdit = new QLineEdit(formCard);
    isbnEdit->setPlaceholderText("e.g. 9780132350884");
    isbnEdit->setStyleSheet(AppStyle::inputStyle());
    isbnBox->addWidget(isbnEdit);
    rowPriceIsbn->addLayout(isbnBox);

    formLayout->addLayout(rowPriceIsbn);

    // Description
    formLayout->addWidget(new QLabel("Description & Details", formCard));
    descriptionEdit = new QTextEdit(formCard);
    descriptionEdit->setPlaceholderText("Mention edition, markings, physical condition, highlighting, etc.");
    descriptionEdit->setStyleSheet(QString(R"(
        QTextEdit {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 10px;
            color: %3;
            font-size: 13.5px;
            padding: 10px 12px;
        }
        QTextEdit:focus {
            border: 1px solid %4;
            background-color: #FFFFFF;
        }
    )").arg(AppStyle::SurfaceSubtle, AppStyle::BorderSubtle, AppStyle::TextPrimary, AppStyle::Primary));
    descriptionEdit->setFixedHeight(120);
    formLayout->addWidget(descriptionEdit);

    formLayout->addStretch();
    contentLayout->addWidget(formCard, 1);

    // ==========================================
    // Right Column: Media Upload Card
    // ==========================================
    QFrame *mediaCard = new QFrame(content);
    mediaCard->setStyleSheet(QString(R"(
        QFrame#mediaCard {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 16px;
        }
    )").arg(AppStyle::BorderSubtle));
    mediaCard->setObjectName("mediaCard");
    AppStyle::applyElevation(mediaCard, 16, 4, 15);

    QVBoxLayout *mediaLayout = new QVBoxLayout(mediaCard);
    mediaLayout->setContentsMargins(28, 28, 28, 28);
    mediaLayout->setSpacing(18);

    // Media Header
    QHBoxLayout *mediaHeader = new QHBoxLayout();
    QLabel *mediaTitle = new QLabel("Photos & Video", mediaCard);
    mediaTitle->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: 700;").arg(AppStyle::TextPrimary));

    photoCountBadge = new QLabel("Photos: 0 / 6 (Min 4 Required)", mediaCard);
    photoCountBadge->setStyleSheet(QString(R"(
        background-color: %1;
        color: %2;
        border: 1px solid %3;
        border-radius: 12px;
        padding: 4px 12px;
        font-size: 12px;
        font-weight: 700;
    )").arg(AppStyle::WarningLight, AppStyle::WarningText, AppStyle::WarningBorder));

    mediaHeader->addWidget(mediaTitle);
    mediaHeader->addStretch();
    mediaHeader->addWidget(photoCountBadge);
    mediaLayout->addLayout(mediaHeader);

    QLabel *mediaSubtext = new QLabel("Upload 4 to 6 photos of your book. Buyers trust listings with clear cover, back, spine, and sample page photos.", mediaCard);
    mediaSubtext->setStyleSheet(QString("color: %1; font-size: 12.5px;").arg(AppStyle::TextSecondary));
    mediaSubtext->setWordWrap(true);
    mediaLayout->addWidget(mediaSubtext);

    // Select Multiple Button
    chooseAllImagesBtn = new QPushButton("📁 Select Multiple Photos at Once...", mediaCard);
    chooseAllImagesBtn->setCursor(Qt::PointingHandCursor);
    chooseAllImagesBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    chooseAllImagesBtn->setFixedHeight(38);
    connect(chooseAllImagesBtn, &QPushButton::clicked, this, &SellWindow::chooseMultipleImages);
    mediaLayout->addWidget(chooseAllImagesBtn);

    // 6-Slot Grid (2 rows x 3 cols)
    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(12);

    imageSlots.clear();
    for (int i = 0; i < 6; ++i) {
        ImageSlotUI slot;

        slot.frame = new QFrame(mediaCard);
        slot.frame->setFixedSize(140, 185);
        slot.frame->setObjectName(QString("slotFrame_%1").arg(i));
        slot.frame->setStyleSheet(QString(R"(
            QFrame#slotFrame_%1 {
                background-color: %2;
                border: 2px dashed %3;
                border-radius: 12px;
            }
            QFrame#slotFrame_%1:hover {
                border-color: %4;
            }
        )").arg(QString::number(i), AppStyle::SurfaceSubtle, AppStyle::BorderStrong, AppStyle::Primary));

        QVBoxLayout *slotLayout = new QVBoxLayout(slot.frame);
        slotLayout->setContentsMargins(6, 6, 6, 6);
        slotLayout->setSpacing(4);

        slot.previewLabel = new QLabel(slot.frame);
        slot.previewLabel->setAlignment(Qt::AlignCenter);
        slot.previewLabel->setFixedSize(128, 95);
        slot.previewLabel->setStyleSheet("background: transparent; border: none;");

        slot.titleLabel = new QLabel(SLOT_TITLES.value(i), slot.frame);
        slot.titleLabel->setAlignment(Qt::AlignCenter);
        slot.titleLabel->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: 600; border: none;").arg(AppStyle::TextMuted));
        slot.titleLabel->setWordWrap(true);

        QHBoxLayout *btnRow = new QHBoxLayout();
        btnRow->setSpacing(4);
        btnRow->setContentsMargins(0, 0, 0, 0);

        slot.thumbnailButton = new QPushButton("★ Cover", slot.frame);
        slot.thumbnailButton->setCursor(Qt::PointingHandCursor);
        slot.thumbnailButton->setFixedHeight(24);
        slot.thumbnailButton->setVisible(false);

        slot.actionButton = new QPushButton("+ Add", slot.frame);
        slot.actionButton->setCursor(Qt::PointingHandCursor);
        slot.actionButton->setFixedHeight(24);
        slot.actionButton->setStyleSheet(AppStyle::secondaryButtonStyle());

        connect(slot.thumbnailButton, &QPushButton::clicked, this, [this, i]() {
            selectedThumbnailIndex = i;
            updateImageSlotsUI();
        });

        connect(slot.actionButton, &QPushButton::clicked, this, [this, i]() {
            if (i < selectedImagePaths.size()) {
                removeImage(i);
            } else {
                chooseImageSlot(i);
            }
        });

        btnRow->addWidget(slot.thumbnailButton, 1);
        btnRow->addWidget(slot.actionButton, 1);

        slotLayout->addWidget(slot.previewLabel);
        slotLayout->addWidget(slot.titleLabel);
        slotLayout->addLayout(btnRow);

        grid->addWidget(slot.frame, i / 3, i % 3);
        imageSlots.append(slot);
    }
    mediaLayout->addLayout(grid);


    // ==========================================
    // Optional Video Section
    // ==========================================
    QLabel *videoHeading = new QLabel("Optional Video Preview (≤ 15 sec, max 50MB)", mediaCard);
    videoHeading->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: 700; margin-top: 6px;").arg(AppStyle::TextPrimary));
    mediaLayout->addWidget(videoHeading);

    videoCard = new QFrame(mediaCard);
    videoCard->setStyleSheet(QString(R"(
        QFrame {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 12px;
        }
    )").arg(AppStyle::SurfaceSubtle, AppStyle::BorderSubtle));

    QHBoxLayout *videoLayout = new QHBoxLayout(videoCard);
    videoLayout->setContentsMargins(14, 12, 14, 12);
    videoLayout->setSpacing(12);

    videoStatusLabel = new QLabel("No video selected (Optional clip showing book pages)", videoCard);
    videoStatusLabel->setStyleSheet(QString("color: %1; font-size: 12.5px;").arg(AppStyle::TextSecondary));

    chooseVideoBtn = new QPushButton("🎥 Select Video...", videoCard);
    chooseVideoBtn->setCursor(Qt::PointingHandCursor);
    chooseVideoBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    chooseVideoBtn->setFixedHeight(34);
    connect(chooseVideoBtn, &QPushButton::clicked, this, &SellWindow::chooseVideo);

    removeVideoBtn = new QPushButton("✕ Remove", videoCard);
    removeVideoBtn->setCursor(Qt::PointingHandCursor);
    removeVideoBtn->setStyleSheet(AppStyle::dangerButtonStyle());
    removeVideoBtn->setFixedHeight(34);
    removeVideoBtn->setVisible(false);
    connect(removeVideoBtn, &QPushButton::clicked, this, &SellWindow::removeVideo);

    videoLayout->addWidget(videoStatusLabel, 1);
    videoLayout->addWidget(chooseVideoBtn);
    videoLayout->addWidget(removeVideoBtn);
    mediaLayout->addWidget(videoCard);

    // Publish Button
    sellButton = new QPushButton("Publish Book Listing", mediaCard);
    sellButton->setCursor(Qt::PointingHandCursor);
    sellButton->setStyleSheet(AppStyle::primaryButtonStyle());
    sellButton->setFixedHeight(48);
    connect(sellButton, &QPushButton::clicked, this, &SellWindow::sellBook);
    mediaLayout->addWidget(sellButton);

    contentLayout->addWidget(mediaCard, 1);

    scrollArea->setWidget(content);
    rootLayout->addWidget(scrollArea);

    updateImageSlotsUI();
    updateVideoUI();
}

void SellWindow::updateImageSlotsUI()
{
    const int count = selectedImagePaths.size();

    // Update count badge
    if (count >= 4) {
        photoCountBadge->setText(QString("Photos: %1 / 6 (Requirement Met)").arg(count));
        photoCountBadge->setStyleSheet(QString(R"(
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 12px;
            padding: 4px 12px;
            font-size: 12px;
            font-weight: 700;
        )").arg(AppStyle::SuccessLight, AppStyle::SuccessText, AppStyle::SuccessBorder));
    } else {
        photoCountBadge->setText(QString("Photos: %1 / 6 (%2 more required)").arg(count).arg(4 - count));
        photoCountBadge->setStyleSheet(QString(R"(
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 12px;
            padding: 4px 12px;
            font-size: 12px;
            font-weight: 700;
        )").arg(AppStyle::WarningLight, AppStyle::WarningText, AppStyle::WarningBorder));
    }

    if (selectedThumbnailIndex >= selectedImagePaths.size()) {
        selectedThumbnailIndex = 0;
    }

    for (int i = 0; i < 6; ++i) {
        if (i >= imageSlots.size()) break;
        auto &slot = imageSlots[i];

        if (i < selectedImagePaths.size()) {
            const QString path = selectedImagePaths[i];
            QPixmap pix(path);
            if (!pix.isNull()) {
                slot.previewLabel->setPixmap(pix.scaled(128, 95, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                slot.previewLabel->setText("Image loaded");
            }
            slot.actionButton->setText("✕ Remove");
            slot.actionButton->setStyleSheet(AppStyle::dangerButtonStyle());
            slot.actionButton->setFixedHeight(24);

            slot.thumbnailButton->setVisible(true);
            if (i == selectedThumbnailIndex) {
                slot.thumbnailButton->setText("★ Cover");
                slot.thumbnailButton->setStyleSheet(QString(R"(
                    QPushButton {
                        background-color: %1;
                        color: #FFFFFF;
                        font-size: 10px;
                        font-weight: 700;
                        border: none;
                        border-radius: 6px;
                    }
                )").arg(AppStyle::Primary));
                slot.titleLabel->setText("★ PRIMARY COVER");
                slot.titleLabel->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: 800;").arg(AppStyle::Primary));
                slot.frame->setStyleSheet(QString(R"(
                    QFrame#slotFrame_%1 {
                        background-color: %2;
                        border: 2px solid %3;
                        border-radius: 12px;
                    }
                )").arg(QString::number(i), AppStyle::PrimaryLight, AppStyle::Primary));
            } else {
                slot.thumbnailButton->setText("Set Cover");
                slot.thumbnailButton->setStyleSheet(AppStyle::secondaryButtonStyle());
                slot.titleLabel->setText(SLOT_TITLES.value(i));
                slot.titleLabel->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: 600;").arg(AppStyle::TextPrimary));
                slot.frame->setStyleSheet(QString(R"(
                    QFrame#slotFrame_%1 {
                        background-color: #FFFFFF;
                        border: 1.5px solid %2;
                        border-radius: 12px;
                    }
                )").arg(QString::number(i), AppStyle::BorderSubtle));
            }
        } else {
            slot.thumbnailButton->setVisible(false);
            slot.previewLabel->clear();
            slot.previewLabel->setText(i < 4 ? "📷 Required" : "📷 Optional");
            slot.previewLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: 600;").arg(i < 4 ? AppStyle::Primary : AppStyle::TextMuted));
            slot.titleLabel->setText(SLOT_TITLES.value(i));
            slot.titleLabel->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: 600;").arg(AppStyle::TextMuted));
            slot.actionButton->setText("+ Add");
            slot.actionButton->setStyleSheet(AppStyle::secondaryButtonStyle());
            slot.actionButton->setFixedHeight(24);
            slot.frame->setStyleSheet(QString(R"(
                QFrame#slotFrame_%1 {
                    background-color: %2;
                    border: 2px dashed %3;
                    border-radius: 12px;
                }
                QFrame#slotFrame_%1:hover {
                    border-color: %4;
                }
            )").arg(QString::number(i), AppStyle::SurfaceSubtle, AppStyle::BorderStrong, AppStyle::Primary));
        }
    }
}


void SellWindow::chooseImageSlot(int index)
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Book Image",
        QString(),
        "Images (*.png *.jpg *.jpeg *.webp *.bmp)"
    );

    if (filePath.isEmpty()) return;

    if (index < selectedImagePaths.size()) {
        selectedImagePaths[index] = filePath;
    } else if (selectedImagePaths.size() < 6) {
        selectedImagePaths.append(filePath);
    }
    updateImageSlotsUI();
}

void SellWindow::chooseMultipleImages()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Select Book Images (Min 4, Max 6)",
        QString(),
        "Images (*.png *.jpg *.jpeg *.webp *.bmp)"
    );

    if (files.isEmpty()) return;

    for (const auto &file : files) {
        if (selectedImagePaths.size() >= 6) break;
        if (!selectedImagePaths.contains(file)) {
            selectedImagePaths.append(file);
        }
    }

    updateImageSlotsUI();
}

void SellWindow::removeImage(int index)
{
    if (index >= 0 && index < selectedImagePaths.size()) {
        selectedImagePaths.removeAt(index);
        updateImageSlotsUI();
    }
}

void SellWindow::updateVideoUI()
{
    if (!selectedVideoPath.isEmpty()) {
        QFileInfo fi(selectedVideoPath);
        const double mb = static_cast<double>(fi.size()) / (1024.0 * 1024.0);
        videoStatusLabel->setText(QString("✓ %1 (%2 MB)").arg(fi.fileName()).arg(QString::number(mb, 'f', 1)));
        videoStatusLabel->setStyleSheet(QString("color: %1; font-weight: 700; font-size: 13px;").arg(AppStyle::Success));
        chooseVideoBtn->setText("Change Video...");
        removeVideoBtn->setVisible(true);
    } else {
        videoStatusLabel->setText("No video selected (Optional clip showing book pages)");
        videoStatusLabel->setStyleSheet(QString("color: %1; font-size: 12.5px;").arg(AppStyle::TextSecondary));
        chooseVideoBtn->setText("🎥 Select Video...");
        removeVideoBtn->setVisible(false);
    }
}

void SellWindow::chooseVideo()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Select Short Book Video (≤ 15 seconds, max 50MB)",
        QString(),
        "Video Files (*.mp4 *.webm *.mov *.avi *.mkv)"
    );

    if (filePath.isEmpty()) return;

    QFileInfo fi(filePath);
    constexpr qint64 maxBytes = 50 * 1024 * 1024; // 50MB
    if (fi.size() > maxBytes) {
        StyledMessageBox::warning(
            this,
            "Video Too Large",
            QString("The selected video is %1 MB. Video size must be less than 50MB and 15 seconds or less.")
                .arg(QString::number(static_cast<double>(fi.size()) / (1024.0 * 1024.0), 'f', 1))
        );
        return;
    }

    selectedVideoPath = filePath;
    updateVideoUI();
}

void SellWindow::removeVideo()
{
    selectedVideoPath.clear();
    updateVideoUI();
}

QString SellWindow::copyMediaToUploads(const QString &sourcePath)
{
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists()) {
        return QString();
    }

    const QString extension = sourceInfo.suffix().toLower();
    const QString fileName = sourceInfo.completeBaseName() + "_" +
                             QString::number(QDateTime::currentMSecsSinceEpoch()) +
                             "." + extension;

    // Target upload locations
    const QStringList candidates = {
        QStringLiteral("d:/1Hello-World/1pbl-oops/backend/public/uploads"),
        QDir::currentPath() + QStringLiteral("/../backend/public/uploads"),
        QDir::currentPath() + QStringLiteral("/../../backend/public/uploads"),
        QDir::currentPath() + QStringLiteral("/backend/public/uploads")
    };

    QString targetDir;
    for (const auto &c : candidates) {
        QDir d(c);
        if (d.exists()) {
            targetDir = d.canonicalPath();
            break;
        }
    }

    if (!targetDir.isEmpty()) {
        const QString destFile = targetDir + "/" + fileName;
        if (QFile::copy(sourcePath, destFile)) {
            return QStringLiteral("http://localhost:8080/uploads/") + fileName;
        }
    }

    // Fallback to app data
    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/uploads";
    QDir(appDataPath).mkpath(".");
    const QString fallbackFile = appDataPath + "/" + fileName;
    if (QFile::copy(sourcePath, fallbackFile)) {
        return fallbackFile;
    }

    return QString();
}

void SellWindow::sellBook()
{
    const QString title = titleEdit ? titleEdit->text().trimmed() : QString();
    const QString author = authorEdit ? authorEdit->text().trimmed() : QString();
    const QString priceText = priceEdit ? priceEdit->text().trimmed() : QString();
    const QString category = categoryCombo ? categoryCombo->currentText() : QString();
    const QString condition = conditionCombo ? conditionCombo->currentText() : QString();
    const QString isbn = isbnEdit ? isbnEdit->text().trimmed() : QString();
    const QString description = descriptionEdit ? descriptionEdit->toPlainText().trimmed() : QString();

    if (title.isEmpty()) {
        StyledMessageBox::warning(this, "Missing Title", "Please enter the book title.");
        if (titleEdit) titleEdit->setFocus();
        return;
    }

    if (author.isEmpty()) {
        StyledMessageBox::warning(this, "Missing Author", "Please enter the author name.");
        if (authorEdit) authorEdit->setFocus();
        return;
    }

    bool priceOk = false;
    const double price = priceText.toDouble(&priceOk);
    if (!priceOk || price <= 0) {
        StyledMessageBox::warning(this, "Invalid Price", "Please enter a valid price greater than ₹0.");
        if (priceEdit) priceEdit->setFocus();
        return;
    }

    // Validate images: minimum 4, maximum 6 required
    if (selectedImagePaths.size() < 4) {
        StyledMessageBox::warning(
            this,
            "Photos Required",
            QString("You must upload at least 4 photos (Cover, Back, Spine, Sample Page). You currently have %1 of 4 required.")
                .arg(selectedImagePaths.size())
        );
        return;
    }

    if (selectedImagePaths.size() > 6) {
        StyledMessageBox::warning(this, "Too Many Photos", "Maximum 6 photos are allowed per listing.");
        return;
    }

    sellButton->setEnabled(false);
    sellButton->setText("Uploading & Publishing...");

    // Copy photos
    QStringList uploadedImageUrls;
    for (const auto &imgPath : selectedImagePaths) {
        QString uploaded = copyMediaToUploads(imgPath);
        if (!uploaded.isEmpty()) {
            uploadedImageUrls.append(uploaded);
        }
    }

    if (uploadedImageUrls.size() < 4) {
        sellButton->setEnabled(true);
        sellButton->setText("Publish Book Listing");
        StyledMessageBox::critical(this, "Upload Failed", "Could not process all book photos. Please try selecting the files again.");
        return;
    }

    // Copy video if provided
    QString uploadedVideoUrl;
    if (!selectedVideoPath.isEmpty()) {
        uploadedVideoUrl = copyMediaToUploads(selectedVideoPath);
    }

    // Build JSON request
    QJsonObject body;
    body[QStringLiteral("title")] = title;
    body[QStringLiteral("author")] = author;
    body[QStringLiteral("price")] = price;
    body[QStringLiteral("condition")] = condition;
    body[QStringLiteral("category")] = category;
    body[QStringLiteral("isbn")] = isbn;
    if (selectedThumbnailIndex >= uploadedImageUrls.size()) {
        selectedThumbnailIndex = 0;
    }
    const QString primaryCover = uploadedImageUrls.value(selectedThumbnailIndex);
    body[QStringLiteral("coverImage")] = primaryCover;

    QJsonArray imgArray;
    imgArray.append(primaryCover);
    for (int i = 0; i < uploadedImageUrls.size(); ++i) {
        if (i != selectedThumbnailIndex) {
            imgArray.append(uploadedImageUrls[i]);
        }
    }
    body[QStringLiteral("images")] = imgArray;

    if (!uploadedVideoUrl.isEmpty()) {
        body[QStringLiteral("videoUrl")] = uploadedVideoUrl;
    }


    auto *reply = ApiClient::instance().post(QStringLiteral("/api/books"), body, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        sellButton->setEnabled(true);
        sellButton->setText("Publish Book Listing");

        if (ok) {
            StyledMessageBox::success(
                this,
                "Book Published",
                "Your book has been successfully listed on BookBazzar!"
            );
            emit bookAdded();
            emit bookPublished();
            emit backRequested();
        } else {
            StyledMessageBox::critical(
                this,
                "Unable to Publish",
                errorMsg.isEmpty() ? "The book could not be added to the marketplace." : errorMsg
            );
        }
    });
}

void SellWindow::handleBack()
{
    emit backRequested();
}