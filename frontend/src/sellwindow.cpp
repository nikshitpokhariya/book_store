#include "sellwindow.h"
#include "database.h"
#include "network/ApiClient.h"
#include <QJsonObject>
#include <QNetworkReply>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>

#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QFrame>
#include <QScrollArea>
#include <QSpacerItem>

#include <QPixmap>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>

#include <QDoubleValidator>


// =========================================================
// CONSTRUCTOR
// =========================================================

SellWindow::SellWindow(
    const QString &sellerName,
    QWidget *parent
    )
    : QWidget(parent),
    sellerName(sellerName),
    titleEdit(nullptr),
    authorEdit(nullptr),
    isbnEdit(nullptr),
    categoryCombo(nullptr),
    conditionCombo(nullptr),
    priceEdit(nullptr),
    descriptionEdit(nullptr),
    locationEdit(nullptr),
    imagePreview(nullptr),
    imagePathLabel(nullptr),
    chooseImageButton(nullptr),
    sellButton(nullptr),
    backButton(nullptr)
{
    setWindowTitle("BookBazzar - Sell Your Book");

    resize(1200, 800);

    setMinimumSize(950, 650);

    setupUI();
}


// =========================================================
// DESTRUCTOR
// =========================================================

SellWindow::~SellWindow()
{
}


// =========================================================
// SETUP UI
// =========================================================

void SellWindow::setupUI()
{
    // =====================================================
    // GLOBAL STYLE
    // =====================================================

    setStyleSheet(R"(

        QWidget {
            font-family: "Segoe UI";
            background-color: #F5F7FB;
            color: #172033;
        }

        QScrollArea {
            border: none;
            background-color: #F5F7FB;
        }

        QScrollBar:vertical {
            background: #EEF1F6;
            width: 9px;
            margin: 4px;
            border-radius: 4px;
        }

        QScrollBar::handle:vertical {
            background: #C7CEDB;
            min-height: 40px;
            border-radius: 4px;
        }

        QScrollBar::handle:vertical:hover {
            background: #AEB7C7;
        }

        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0px;
        }

        QLabel {
            background: transparent;
        }

        QLineEdit,
        QTextEdit,
        QComboBox {

            background-color: #FFFFFF;

            color: #172033;

            border: 1px solid #D9DEE8;

            border-radius: 9px;

            font-size: 14px;
        }

        QLineEdit {

            min-height: 44px;

            padding-left: 13px;
            padding-right: 13px;
        }

        QTextEdit {

            padding: 10px 12px;

            min-height: 110px;
        }

        QComboBox {

            min-height: 44px;

            padding-left: 13px;
            padding-right: 13px;
        }

        QComboBox::drop-down {

            border: none;

            width: 35px;
        }

        QComboBox QAbstractItemView {

            background-color: white;

            color: #172033;

            border: 1px solid #D9DEE8;

            selection-background-color: #EEF2FF;

            selection-color: #4F46E5;
        }

        QLineEdit:focus,
        QTextEdit:focus,
        QComboBox:focus {

            border: 2px solid #6366F1;

            background-color: #FFFFFF;
        }

        QPushButton {

            border-radius: 9px;

            font-size: 14px;

            font-weight: 700;

            min-height: 44px;

            padding-left: 18px;
            padding-right: 18px;
        }

    )");


    // =====================================================
    // MAIN WINDOW LAYOUT
    // =====================================================

    QVBoxLayout *outerLayout =
        new QVBoxLayout(this);

    outerLayout->setContentsMargins(
        0, 0, 0, 0
        );

    outerLayout->setSpacing(0);


    // =====================================================
    // HEADER
    // =====================================================

    QFrame *header =
        new QFrame();

    header->setFixedHeight(82);

    header->setStyleSheet(R"(
        QFrame {
            background-color: #FFFFFF;
            border-bottom: 1px solid #E5E7EB;
        }
    )");


    QHBoxLayout *headerLayout =
        new QHBoxLayout(header);

    headerLayout->setContentsMargins(
        35, 0, 35, 0
        );


    // Logo
    QLabel *logo =
        new QLabel("BookBazzar");

    logo->setStyleSheet(R"(
        QLabel {
            color: #4F46E5;
            font-size: 24px;
            font-weight: 800;
        }
    )");

    headerLayout->addWidget(logo);


    // Separator
    QFrame *separator =
        new QFrame();

    separator->setFrameShape(QFrame::VLine);

    separator->setStyleSheet(
        "color: #E5E7EB;"
        );

    headerLayout->addSpacing(18);

    headerLayout->addWidget(separator);

    headerLayout->addSpacing(18);


    // Page title
    QLabel *pageTitle =
        new QLabel("Sell Your Book");

    pageTitle->setStyleSheet(R"(
        QLabel {
            color: #172033;
            font-size: 19px;
            font-weight: 700;
        }
    )");

    headerLayout->addWidget(pageTitle);

    headerLayout->addStretch();


    // Seller
    QLabel *sellerLabel =
        new QLabel(
            "Seller: " + sellerName
            );

    sellerLabel->setStyleSheet(R"(
        QLabel {
            color: #64748B;
            font-size: 13px;
            font-weight: 600;
        }
    )");

    headerLayout->addWidget(sellerLabel);


    headerLayout->addSpacing(20);


    // Back button
    backButton =
        new QPushButton("←  Back");

    backButton->setFixedHeight(40);

    backButton->setStyleSheet(R"(
        QPushButton {
            background-color: #FFFFFF;
            color: #475569;
            border: 1px solid #D9DEE8;
        }

        QPushButton:hover {
            background-color: #F8FAFC;
            color: #4F46E5;
            border-color: #A5B4FC;
        }

        QPushButton:pressed {
            background-color: #EEF2FF;
        }
    )");

    headerLayout->addWidget(backButton);


    outerLayout->addWidget(header);


    // =====================================================
    // SCROLL AREA
    // =====================================================

    QScrollArea *scrollArea =
        new QScrollArea();

    scrollArea->setWidgetResizable(true);

    scrollArea->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff
        );


    QWidget *scrollContent =
        new QWidget();

    scrollContent->setStyleSheet(
        "background-color: #F5F7FB;"
        );


    QVBoxLayout *mainLayout =
        new QVBoxLayout(scrollContent);

    mainLayout->setContentsMargins(
        35, 30, 35, 40
        );

    mainLayout->setSpacing(22);


    // =====================================================
    // INTRODUCTION
    // =====================================================

    QLabel *heading =
        new QLabel("List a book for sale");

    heading->setStyleSheet(R"(
        QLabel {
            font-size: 28px;
            font-weight: 800;
            color: #172033;
        }
    )");

    mainLayout->addWidget(heading);


    QLabel *subtitle =
        new QLabel(
            "Provide accurate details about your book to help buyers "
            "find and purchase it easily."
            );

    subtitle->setStyleSheet(R"(
        QLabel {
            color: #64748B;
            font-size: 14px;
        }
    )");

    mainLayout->addWidget(subtitle);


    // =====================================================
    // MAIN CONTENT
    // =====================================================

    QHBoxLayout *contentLayout =
        new QHBoxLayout();

    contentLayout->setSpacing(24);


    // =====================================================
    // LEFT CARD
    // =====================================================

    QFrame *detailsCard =
        new QFrame();

    detailsCard->setStyleSheet(R"(
        QFrame {
            background-color: #FFFFFF;
            border: 1px solid #E3E7EF;
            border-radius: 16px;
        }
    )");


    QVBoxLayout *detailsLayout =
        new QVBoxLayout(detailsCard);

    detailsLayout->setContentsMargins(
        25, 25, 25, 25
        );

    detailsLayout->setSpacing(20);


    // =====================================================
    // SECTION HEADER
    // =====================================================

    QHBoxLayout *detailsHeader =
        new QHBoxLayout();


    QLabel *detailsTitle =
        new QLabel("Book Details");

    detailsTitle->setStyleSheet(R"(
        QLabel {
            font-size: 19px;
            font-weight: 800;
            color: #172033;
        }
    )");

    detailsHeader->addWidget(detailsTitle);

    detailsHeader->addStretch();


    QLabel *requiredLabel =
        new QLabel("* Required");

    requiredLabel->setStyleSheet(R"(
        QLabel {
            color: #EF4444;
            font-size: 12px;
            font-weight: 600;
        }
    )");

    detailsHeader->addWidget(requiredLabel);

    detailsLayout->addLayout(detailsHeader);


    // =====================================================
    // TITLE + AUTHOR
    // =====================================================

    QHBoxLayout *row1 =
        new QHBoxLayout();

    row1->setSpacing(15);


    // Title
    QVBoxLayout *titleLayout =
        new QVBoxLayout();

    QLabel *titleLabel =
        new QLabel("Book Title *");

    titleLabel->setStyleSheet(
        "font-size: 13px; font-weight: 700; color: #374151;"
        );

    titleEdit =
        new QLineEdit();

    titleEdit->setPlaceholderText(
        "e.g. The C++ Programming Language"
        );

    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(titleEdit);


    // Author
    QVBoxLayout *authorLayout =
        new QVBoxLayout();

    QLabel *authorLabel =
        new QLabel("Author *");

    authorLabel->setStyleSheet(
        "font-size: 13px; font-weight: 700; color: #374151;"
        );

    authorEdit =
        new QLineEdit();

    authorEdit->setPlaceholderText(
        "e.g. Bjarne Stroustrup"
        );

    authorLayout->addWidget(authorLabel);
    authorLayout->addWidget(authorEdit);


    row1->addLayout(titleLayout, 1);
    row1->addLayout(authorLayout, 1);

    detailsLayout->addLayout(row1);


    // =====================================================
    // ISBN
    // =====================================================

    QLabel *isbnLabel =
        new QLabel("ISBN");

    isbnLabel->setStyleSheet(
        "font-size: 13px; font-weight: 700; color: #374151;"
        );

    isbnEdit =
        new QLineEdit();

    isbnEdit->setPlaceholderText(
        "Optional ISBN number"
        );

    detailsLayout->addWidget(isbnLabel);
    detailsLayout->addWidget(isbnEdit);


    // =====================================================
    // CATEGORY + CONDITION
    // =====================================================

    QHBoxLayout *row2 =
        new QHBoxLayout();

    row2->setSpacing(15);


    // Category
    QVBoxLayout *categoryLayout =
        new QVBoxLayout();

    QLabel *categoryLabel =
        new QLabel("Category *");

    categoryLabel->setStyleSheet(
        "font-size: 13px; font-weight: 700; color: #374151;"
        );

    categoryCombo =
        new QComboBox();

    categoryCombo->addItem("Academic");
    categoryCombo->addItem("Programming");
    categoryCombo->addItem("Engineering");
    categoryCombo->addItem("Fiction");
    categoryCombo->addItem("Competitive Exams");
    categoryCombo->addItem("School");
    categoryCombo->addItem("Novels");

    categoryLayout->addWidget(categoryLabel);
    categoryLayout->addWidget(categoryCombo);


    // Condition
    QVBoxLayout *conditionLayout =
        new QVBoxLayout();

    QLabel *conditionLabel =
        new QLabel("Condition *");

    conditionLabel->setStyleSheet(
        "font-size: 13px; font-weight: 700; color: #374151;"
        );

    conditionCombo =
        new QComboBox();

    conditionCombo->addItem("New");
    conditionCombo->addItem("Like New");
    conditionCombo->addItem("Good");
    conditionCombo->addItem("Fair");
    conditionCombo->addItem("Used");

    conditionLayout->addWidget(conditionLabel);
    conditionLayout->addWidget(conditionCombo);


    row2->addLayout(categoryLayout, 1);
    row2->addLayout(conditionLayout, 1);

    detailsLayout->addLayout(row2);


    // =====================================================
    // PRICE + LOCATION
    // =====================================================

    QHBoxLayout *row3 =
        new QHBoxLayout();

    row3->setSpacing(15);


    // Price
    QVBoxLayout *priceLayout =
        new QVBoxLayout();

    QLabel *priceLabel =
        new QLabel("Selling Price *");

    priceLabel->setStyleSheet(
        "font-size: 13px; font-weight: 700; color: #374151;"
        );

    priceEdit =
        new QLineEdit();

    priceEdit->setPlaceholderText(
        "₹ Enter price"
        );


    QDoubleValidator *priceValidator =
        new QDoubleValidator(
            0.0,
            10000000.0,
            2,
            this
            );

    priceValidator->setNotation(
        QDoubleValidator::StandardNotation
        );

    priceEdit->setValidator(
        priceValidator
        );


    priceLayout->addWidget(priceLabel);
    priceLayout->addWidget(priceEdit);


    // Location
    QVBoxLayout *locationLayout =
        new QVBoxLayout();

    QLabel *locationLabel =
        new QLabel("Location");

    locationLabel->setStyleSheet(
        "font-size: 13px; font-weight: 700; color: #374151;"
        );

    locationEdit =
        new QLineEdit();

    locationEdit->setPlaceholderText(
        "e.g. Nainital"
        );

    locationLayout->addWidget(locationLabel);
    locationLayout->addWidget(locationEdit);


    row3->addLayout(priceLayout, 1);
    row3->addLayout(locationLayout, 1);

    detailsLayout->addLayout(row3);


    // =====================================================
    // DESCRIPTION
    // =====================================================

    QLabel *descriptionLabel =
        new QLabel("Description");

    descriptionLabel->setStyleSheet(
        "font-size: 13px; font-weight: 700; color: #374151;"
        );

    descriptionEdit =
        new QTextEdit();

    descriptionEdit->setPlaceholderText(
        "Describe the book's condition, edition, "
        "highlights, included material, etc."
        );

    detailsLayout->addWidget(
        descriptionLabel
        );

    detailsLayout->addWidget(
        descriptionEdit
        );


    // =====================================================
    // INFO BOX
    // =====================================================

    QFrame *infoBox =
        new QFrame();

    infoBox->setStyleSheet(R"(
        QFrame {
            background-color: #F8FAFC;
            border: 1px solid #E2E8F0;
            border-radius: 10px;
        }
    )");


    QHBoxLayout *infoLayout =
        new QHBoxLayout(infoBox);

    infoLayout->setContentsMargins(
        14, 12, 14, 12
        );


    QLabel *infoIcon =
        new QLabel("ⓘ");

    infoIcon->setStyleSheet(R"(
        QLabel {
            color: #4F46E5;
            font-size: 18px;
            font-weight: bold;
        }
    )");


    QLabel *infoText =
        new QLabel(
            "Tip: Clear descriptions and accurate book details "
            "help buyers make confident decisions."
            );

    infoText->setWordWrap(true);

    infoText->setStyleSheet(R"(
        QLabel {
            color: #64748B;
            font-size: 12px;
            border: none;
        }
    )");


    infoLayout->addWidget(infoIcon);
    infoLayout->addSpacing(8);
    infoLayout->addWidget(infoText, 1);

    detailsLayout->addWidget(infoBox);


    contentLayout->addWidget(
        detailsCard,
        2
        );


    // =====================================================
    // RIGHT IMAGE CARD
    // =====================================================

    QFrame *imageCard =
        new QFrame();

    imageCard->setFixedWidth(350);

    imageCard->setStyleSheet(R"(
        QFrame {
            background-color: #FFFFFF;
            border: 1px solid #E3E7EF;
            border-radius: 16px;
        }
    )");


    QVBoxLayout *imageLayout =
        new QVBoxLayout(imageCard);

    imageLayout->setContentsMargins(
        25, 25, 25, 25
        );

    imageLayout->setSpacing(14);


    // =====================================================
    // IMAGE HEADER
    // =====================================================

    QLabel *imageTitle =
        new QLabel("Book Cover");

    imageTitle->setStyleSheet(R"(
        QLabel {
            font-size: 19px;
            font-weight: 800;
            color: #172033;
        }
    )");

    imageLayout->addWidget(imageTitle);


    QLabel *imageSubtitle =
        new QLabel(
            "Upload a clear front-cover image."
            );

    imageSubtitle->setStyleSheet(R"(
        QLabel {
            color: #64748B;
            font-size: 12px;
            border: none;
        }
    )");

    imageLayout->addWidget(
        imageSubtitle
        );


    // =====================================================
    // IMAGE PREVIEW
    // =====================================================

    imagePreview =
        new QLabel();

    imagePreview->setFixedSize(
        290,
        350
        );

    imagePreview->setAlignment(
        Qt::AlignCenter
        );

    imagePreview->setText(
        "📚\n\nNo image selected\n\n"
        "Your book cover will appear here"
        );

    imagePreview->setWordWrap(true);

    imagePreview->setStyleSheet(R"(
        QLabel {
            background-color: #F8FAFC;
            color: #94A3B8;
            border: 2px dashed #CBD5E1;
            border-radius: 14px;
            font-size: 13px;
            font-weight: 600;
        }
    )");


    imageLayout->addWidget(
        imagePreview,
        0,
        Qt::AlignCenter
        );


    // =====================================================
    // CHOOSE IMAGE BUTTON
    // =====================================================

    chooseImageButton =
        new QPushButton(
            "＋  Choose Book Image"
            );

    chooseImageButton->setMinimumHeight(
        46
        );

    chooseImageButton->setStyleSheet(R"(
        QPushButton {
            background-color: #EEF2FF;
            color: #4F46E5;
            border: 1px solid #C7D2FE;
        }

        QPushButton:hover {
            background-color: #E0E7FF;
            border-color: #A5B4FC;
        }

        QPushButton:pressed {
            background-color: #C7D2FE;
        }
    )");


    imageLayout->addWidget(
        chooseImageButton
        );


    // =====================================================
    // IMAGE PATH
    // =====================================================

    imagePathLabel =
        new QLabel(
            "No image selected"
            );

    imagePathLabel->setAlignment(
        Qt::AlignCenter
        );

    imagePathLabel->setWordWrap(
        true
        );

    imagePathLabel->setStyleSheet(R"(
        QLabel {
            color: #94A3B8;
            font-size: 11px;
            border: none;
        }
    )");


    imageLayout->addWidget(
        imagePathLabel
        );


    imageLayout->addStretch();


    // =====================================================
    // PUBLISH BUTTON
    // =====================================================

    sellButton =
        new QPushButton(
            "Publish Book"
            );

    sellButton->setMinimumHeight(
        52
        );

    sellButton->setStyleSheet(R"(
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

        QPushButton:pressed {
            background-color: #3730A3;
        }

        QPushButton:disabled {
            background-color: #A5B4FC;
        }
    )");


    imageLayout->addWidget(
        sellButton
        );


    contentLayout->addWidget(
        imageCard,
        0
        );


    mainLayout->addLayout(
        contentLayout
        );


    // =====================================================
    // FOOTER
    // =====================================================

    QLabel *footer =
        new QLabel(
            "By publishing this listing, you confirm that "
            "the information provided is accurate."
            );

    footer->setAlignment(
        Qt::AlignCenter
        );

    footer->setStyleSheet(R"(
        QLabel {
            color: #94A3B8;
            font-size: 11px;
        }
    )");


    mainLayout->addWidget(
        footer
        );


    // =====================================================
    // SET SCROLL WIDGET
    // =====================================================

    scrollArea->setWidget(
        scrollContent
        );

    outerLayout->addWidget(
        scrollArea,
        1
        );


    // =====================================================
    // CONNECTIONS
    // =====================================================

    connect(
        chooseImageButton,
        &QPushButton::clicked,
        this,
        &SellWindow::chooseImage
        );


    connect(
        sellButton,
        &QPushButton::clicked,
        this,
        &SellWindow::sellBook
        );


    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &SellWindow::handleBack
        );
}


// =========================================================
// CHOOSE IMAGE
// =========================================================

void SellWindow::chooseImage()
{
    QString filePath =
        QFileDialog::getOpenFileName(
            this,
            "Select Book Cover",
            QString(),
            "Images (*.png *.jpg *.jpeg *.bmp *.webp)"
            );


    if (filePath.isEmpty())
    {
        return;
    }


    QPixmap pixmap(filePath);


    if (pixmap.isNull())
    {
        QMessageBox::warning(
            this,
            "Invalid Image",
            "The selected file could not be loaded."
            );

        return;
    }


    selectedImagePath =
        filePath;


    QPixmap scaled =
        pixmap.scaled(
            imagePreview->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            );


    imagePreview->setPixmap(
        scaled
        );


    imagePreview->setText(
        QString()
        );


    imagePathLabel->setText(
        QFileInfo(filePath).fileName()
        );


    imagePathLabel->setStyleSheet(R"(
        QLabel {
            color: #475569;
            font-size: 11px;
            font-weight: 600;
            border: none;
        }
    )");
}


// =========================================================
// COPY IMAGE TO APPLICATION FOLDER
// =========================================================

QString SellWindow::copyImageToAppFolder(
    const QString &sourcePath
    )
{
    if (sourcePath.isEmpty())
    {
        return QString();
    }


    QFileInfo sourceInfo(
        sourcePath
        );


    if (!sourceInfo.exists())
    {
        return QString();
    }


    QString appDataPath =
        QStandardPaths::writableLocation(
            QStandardPaths::AppDataLocation
            );


    if (appDataPath.isEmpty())
    {
        return QString();
    }


    QDir appDirectory(
        appDataPath
        );


    if (!appDirectory.exists())
    {
        if (!appDirectory.mkpath("."))
        {
            return QString();
        }
    }


    QString imagesPath =
        appDataPath + "/images";


    QDir imagesDirectory(
        imagesPath
        );


    if (!imagesDirectory.exists())
    {
        if (!imagesDirectory.mkpath("."))
        {
            return QString();
        }
    }


    // =====================================================
    // UNIQUE FILE NAME
    // =====================================================

    QString extension =
        sourceInfo.suffix().toLower();


    QString baseName =
        sourceInfo.completeBaseName();


    QString fileName =
        baseName +
        "_" +
        QString::number(
            QDateTime::currentMSecsSinceEpoch()
            ) +
        "." +
        extension;


    QString destinationPath = imagesPath + "/" + fileName;

    QString destinationDir;
    QStringList candidates = {
        "d:/1Hello-World/1pbl-oops/backend/public/uploads",
        QDir::currentPath() + "/../backend/public/uploads",
        QDir::currentPath() + "/../../backend/public/uploads"
    };
    for (const auto &c : candidates) {
        if (QDir(c).exists()) {
            destinationDir = QDir(c).canonicalPath();
            break;
        }
    }

    if (!destinationDir.isEmpty()) {
        QString destFile = destinationDir + "/" + fileName;
        if (QFile::copy(sourcePath, destFile)) {
            return QStringLiteral("http://localhost:8080/uploads/") + fileName;
        }
    }

    if (!QFile::copy(sourcePath, destinationPath))
    {
        return QString();
    }

    return destinationPath;
}


// =========================================================
// SELL BOOK
// =========================================================

void SellWindow::sellBook()
{
    QString title =
        titleEdit->text().trimmed();


    QString author =
        authorEdit->text().trimmed();


    QString isbn =
        isbnEdit->text().trimmed();


    QString category =
        categoryCombo->currentText();


    QString condition =
        conditionCombo->currentText();


    QString priceText =
        priceEdit->text().trimmed();


    QString description =
        descriptionEdit->toPlainText().trimmed();


    QString location =
        locationEdit->text().trimmed();


    // =====================================================
    // VALIDATION
    // =====================================================

    if (title.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Missing Information",
            "Please enter the book title."
            );

        titleEdit->setFocus();

        return;
    }


    if (author.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Missing Information",
            "Please enter the author name."
            );

        authorEdit->setFocus();

        return;
    }


    if (priceText.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Missing Information",
            "Please enter the selling price."
            );

        priceEdit->setFocus();

        return;
    }


    bool ok = false;


    double price =
        priceText.toDouble(
            &ok
            );


    if (!ok || price <= 0)
    {
        QMessageBox::warning(
            this,
            "Invalid Price",
            "Please enter a valid price greater than ₹0."
            );

        priceEdit->setFocus();

        return;
    }


    // =====================================================
    // COPY IMAGE
    // =====================================================

    QString storedImagePath;


    if (!selectedImagePath.isEmpty())
    {
        storedImagePath =
            copyImageToAppFolder(
                selectedImagePath
                );


        if (storedImagePath.isEmpty())
        {
            QMessageBox::warning(
                this,
                "Image Error",
                "The selected image could not be saved.\n\n"
                "Please choose the image again."
                );

            return;
        }
    }


    // =====================================================
    // POST TO BACKEND API
    // =====================================================

    sellButton->setEnabled(false);
    sellButton->setText("Publishing...");

    QJsonObject body;
    body[QStringLiteral("title")] = title;
    body[QStringLiteral("author")] = author;
    body[QStringLiteral("price")] = price;
    body[QStringLiteral("condition")] = condition;
    body[QStringLiteral("category")] = category;
    body[QStringLiteral("isbn")] = isbn;
    body[QStringLiteral("description")] = description;
    body[QStringLiteral("coverImage")] = storedImagePath;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/books"), body, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply, storedImagePath]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        sellButton->setEnabled(true);
        sellButton->setText("Publish Book");

        if (ok) {
            QMessageBox::information(
                this,
                "Book Published",
                "Your book has been successfully listed on BookBazzar!"
            );

            emit bookAdded();
            emit backRequested();
        } else {
            if (!storedImagePath.isEmpty() && !storedImagePath.startsWith("http")) {
                QFile::remove(storedImagePath);
            }

            QMessageBox::critical(
                this,
                "Unable to Publish",
                errorMsg.isEmpty() ? "The book could not be added to the marketplace." : errorMsg
            );
        }
    });
}


// =========================================================
// BACK
// =========================================================

void SellWindow::handleBack()
{
    emit backRequested();
}