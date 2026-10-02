#include "sellbookwindow.h"
#include "network/ApiClient.h"
#include <QJsonObject>
#include <QNetworkReply>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDoubleSpinBox>

#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QFileInfo>


SellBookWindow::SellBookWindow(
    const QString &sellerEmail,
    QWidget *parent
    )
    : QWidget(parent),
    sellerEmail(sellerEmail)
{
    setWindowTitle("BookBazzar - Sell Your Book");

    resize(1100, 750);

    setMinimumSize(900, 650);


    // =====================================================
    // MAIN LAYOUT
    // =====================================================

    QVBoxLayout *mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        30, 25, 30, 25
        );

    mainLayout->setSpacing(20);


    // =====================================================
    // HEADER
    // =====================================================

    QHBoxLayout *headerLayout =
        new QHBoxLayout();


    backButton =
        new QPushButton("← Back");

    backButton->setObjectName(
        "backButton"
        );

    backButton->setFixedSize(
        100, 40
        );


    QLabel *heading =
        new QLabel("Sell Your Book");

    heading->setObjectName(
        "heading"
        );


    QLabel *subtitle =
        new QLabel(
            "List your book and choose your own selling price"
            );

    subtitle->setObjectName(
        "subtitle"
        );


    QVBoxLayout *headingLayout =
        new QVBoxLayout();

    headingLayout->setSpacing(2);

    headingLayout->addWidget(
        heading
        );

    headingLayout->addWidget(
        subtitle
        );


    headerLayout->addWidget(
        backButton
        );

    headerLayout->addSpacing(
        20
        );

    headerLayout->addLayout(
        headingLayout
        );

    headerLayout->addStretch();


    mainLayout->addLayout(
        headerLayout
        );


    // =====================================================
    // CONTENT LAYOUT
    // =====================================================

    QHBoxLayout *contentLayout =
        new QHBoxLayout();

    contentLayout->setSpacing(
        25
        );


    // =====================================================
    // LEFT SIDE - IMAGE
    // =====================================================

    QFrame *imageFrame =
        new QFrame();

    imageFrame->setObjectName(
        "imageFrame"
        );

    imageFrame->setMinimumWidth(
        300
        );


    QVBoxLayout *imageLayout =
        new QVBoxLayout(imageFrame);

    imageLayout->setContentsMargins(
        20, 20, 20, 20
        );

    imageLayout->setSpacing(
        15
        );


    QLabel *imageTitle =
        new QLabel("Book Image");

    imageTitle->setObjectName(
        "sectionTitle"
        );


    imagePreview =
        new QLabel();

    imagePreview->setObjectName(
        "imagePreview"
        );

    imagePreview->setAlignment(
        Qt::AlignCenter
        );

    imagePreview->setText(
        "📚\n\n"
        "No image selected"
        );

    imagePreview->setMinimumSize(
        250, 330
        );

    imagePreview->setMaximumSize(
        280, 360
        );


    chooseImageButton =
        new QPushButton(
            "Choose Book Image"
            );

    chooseImageButton->setObjectName(
        "secondaryButton"
        );

    chooseImageButton->setMinimumHeight(
        45
        );


    QLabel *imageHint =
        new QLabel(
            "Upload a clear image of the book.\n"
            "JPG, PNG and JPEG are supported."
            );

    imageHint->setObjectName(
        "hintLabel"
        );

    imageHint->setWordWrap(
        true
        );

    imageHint->setAlignment(
        Qt::AlignCenter
        );


    imageLayout->addWidget(
        imageTitle
        );

    imageLayout->addWidget(
        imagePreview,
        0,
        Qt::AlignCenter
        );

    imageLayout->addWidget(
        chooseImageButton
        );

    imageLayout->addWidget(
        imageHint
        );

    imageLayout->addStretch();


    // =====================================================
    // RIGHT SIDE - FORM
    // =====================================================

    QFrame *formFrame =
        new QFrame();

    formFrame->setObjectName(
        "formFrame"
        );


    QVBoxLayout *formLayout =
        new QVBoxLayout(formFrame);

    formLayout->setContentsMargins(
        25, 20, 25, 20
        );

    formLayout->setSpacing(
        12
        );


    QLabel *informationTitle =
        new QLabel(
            "Book Information"
            );

    informationTitle->setObjectName(
        "sectionTitle"
        );


    formLayout->addWidget(
        informationTitle
        );


    // =====================================================
    // TITLE
    // =====================================================

    titleEdit =
        new QLineEdit();

    titleEdit->setPlaceholderText(
        "Enter book title"
        );

    titleEdit->setObjectName(
        "input"
        );

    titleEdit->setMinimumHeight(
        42
        );

    formLayout->addWidget(
        new QLabel("Book Title *")
        );

    formLayout->addWidget(
        titleEdit
        );


    // =====================================================
    // AUTHOR
    // =====================================================

    authorEdit =
        new QLineEdit();

    authorEdit->setPlaceholderText(
        "Enter author name"
        );

    authorEdit->setObjectName(
        "input"
        );

    authorEdit->setMinimumHeight(
        42
        );

    formLayout->addWidget(
        new QLabel("Author *")
        );

    formLayout->addWidget(
        authorEdit
        );


    // =====================================================
    // ISBN
    // =====================================================

    isbnEdit =
        new QLineEdit();

    isbnEdit->setPlaceholderText(
        "Enter ISBN"
        );

    isbnEdit->setObjectName(
        "input"
        );

    isbnEdit->setMinimumHeight(
        42
        );

    formLayout->addWidget(
        new QLabel("ISBN")
        );

    formLayout->addWidget(
        isbnEdit
        );


    // =====================================================
    // CATEGORY + CONDITION
    // =====================================================

    QHBoxLayout *categoryConditionLayout =
        new QHBoxLayout();


    // CATEGORY

    QVBoxLayout *categoryLayout =
        new QVBoxLayout();

    QLabel *categoryLabel =
        new QLabel("Category *");


    categoryCombo =
        new QComboBox();

    categoryCombo->setObjectName(
        "input"
        );

    categoryCombo->addItem(
        "Engineering"
        );

    categoryCombo->addItem(
        "Medical"
        );

    categoryCombo->addItem(
        "Competitive Exams"
        );

    categoryCombo->addItem(
        "School"
        );

    categoryCombo->addItem(
        "College"
        );

    categoryCombo->addItem(
        "Novel"
        );

    categoryCombo->addItem(
        "Fiction"
        );

    categoryCombo->addItem(
        "Non-Fiction"
        );

    categoryCombo->addItem(
        "Other"
        );


    categoryLayout->addWidget(
        categoryLabel
        );

    categoryLayout->addWidget(
        categoryCombo
        );


    // CONDITION

    QVBoxLayout *conditionLayout =
        new QVBoxLayout();

    QLabel *conditionLabel =
        new QLabel("Condition *");


    conditionCombo =
        new QComboBox();

    conditionCombo->setObjectName(
        "input"
        );

    conditionCombo->addItem(
        "Like New"
        );

    conditionCombo->addItem(
        "Excellent"
        );

    conditionCombo->addItem(
        "Good"
        );

    conditionCombo->addItem(
        "Fair"
        );

    conditionCombo->addItem(
        "Poor"
        );


    conditionLayout->addWidget(
        conditionLabel
        );

    conditionLayout->addWidget(
        conditionCombo
        );


    categoryConditionLayout->addLayout(
        categoryLayout
        );

    categoryConditionLayout->addLayout(
        conditionLayout
        );


    formLayout->addLayout(
        categoryConditionLayout
        );


    // =====================================================
    // EDITION
    // =====================================================

    editionEdit =
        new QLineEdit();

    editionEdit->setPlaceholderText(
        "Example: 5th Edition"
        );

    editionEdit->setObjectName(
        "input"
        );

    editionEdit->setMinimumHeight(
        42
        );

    formLayout->addWidget(
        new QLabel("Edition")
        );

    formLayout->addWidget(
        editionEdit
        );


    // =====================================================
    // PRICE
    // =====================================================

    priceSpinBox =
        new QDoubleSpinBox();

    priceSpinBox->setObjectName(
        "priceInput"
        );

    priceSpinBox->setMinimum(
        1
        );

    priceSpinBox->setMaximum(
        1000000
        );

    priceSpinBox->setDecimals(
        2
        );

    priceSpinBox->setPrefix(
        "₹ "
        );

    priceSpinBox->setMinimumHeight(
        45
        );


    formLayout->addWidget(
        new QLabel("Your Selling Price *")
        );

    formLayout->addWidget(
        priceSpinBox
        );


    // =====================================================
    // LOCATION
    // =====================================================

    locationEdit =
        new QLineEdit();

    locationEdit->setPlaceholderText(
        "Example: Nainital, Uttarakhand"
        );

    locationEdit->setObjectName(
        "input"
        );

    locationEdit->setMinimumHeight(
        42
        );


    formLayout->addWidget(
        new QLabel("Location *")
        );

    formLayout->addWidget(
        locationEdit
        );


    // =====================================================
    // DESCRIPTION
    // =====================================================

    descriptionEdit =
        new QTextEdit();

    descriptionEdit->setPlaceholderText(
        "Tell buyers about your book, "
        "its condition, markings, damage, etc."
        );

    descriptionEdit->setObjectName(
        "descriptionInput"
        );

    descriptionEdit->setMinimumHeight(
        90
        );

    descriptionEdit->setMaximumHeight(
        120
        );


    formLayout->addWidget(
        new QLabel("Description")
        );

    formLayout->addWidget(
        descriptionEdit
        );


    // =====================================================
    // PUBLISH BUTTON
    // =====================================================

    publishButton =
        new QPushButton(
            "Publish Book"
            );

    publishButton->setObjectName(
        "publishButton"
        );

    publishButton->setMinimumHeight(
        50
        );


    formLayout->addSpacing(
        5
        );

    formLayout->addWidget(
        publishButton
        );


    // =====================================================
    // ADD BOTH SIDES
    // =====================================================

    contentLayout->addWidget(
        imageFrame,
        1
        );

    contentLayout->addWidget(
        formFrame,
        2
        );


    mainLayout->addLayout(
        contentLayout
        );


    // =====================================================
    // STYLING
    // =====================================================

    setStyleSheet(R"(

        QWidget {
            font-family: "Segoe UI";
            background-color: #F8FAFC;
            color: #0F172A;
        }

        #heading {
            font-size: 30px;
            font-weight: bold;
            color: #0F172A;
        }

        #subtitle {
            font-size: 14px;
            color: #64748B;
        }

        #imageFrame,
        #formFrame {
            background-color: white;
            border: 1px solid #E2E8F0;
            border-radius: 16px;
        }

        #sectionTitle {
            font-size: 20px;
            font-weight: bold;
            color: #0F172A;
        }

        #imagePreview {
            background-color: #F1F5F9;
            border: 2px dashed #CBD5E1;
            border-radius: 12px;
            color: #64748B;
            font-size: 16px;
        }

        #input,
        #priceInput,
        #descriptionInput {

            background-color: #F8FAFC;

            border: 1px solid #CBD5E1;

            border-radius: 8px;

            padding: 8px;

            font-size: 14px;
        }

        #input:focus,
        #priceInput:focus,
        #descriptionInput:focus {

            border: 2px solid #2563EB;

            background-color: white;
        }

        #secondaryButton {

            background-color: #EFF6FF;

            color: #2563EB;

            border: 1px solid #BFDBFE;

            border-radius: 8px;

            font-weight: bold;
        }

        #secondaryButton:hover {

            background-color: #DBEAFE;
        }

        #publishButton {

            background-color: #2563EB;

            color: white;

            border: none;

            border-radius: 9px;

            font-size: 16px;

            font-weight: bold;
        }

        #publishButton:hover {

            background-color: #1D4ED8;
        }

        #publishButton:pressed {

            background-color: #1E40AF;
        }

        #backButton {

            background-color: white;

            color: #475569;

            border: 1px solid #CBD5E1;

            border-radius: 8px;

            font-size: 14px;
        }

        #backButton:hover {

            background-color: #F1F5F9;
        }

        QLabel {
            background-color: transparent;
        }

        QComboBox {
            background-color: #F8FAFC;
            border: 1px solid #CBD5E1;
            border-radius: 8px;
            padding: 8px;
            min-height: 25px;
        }

        QComboBox:focus {
            border: 2px solid #2563EB;
        }

        QComboBox QAbstractItemView {
            background-color: white;
            selection-background-color: #2563EB;
            selection-color: white;
        }

        #hintLabel {
            color: #64748B;
            font-size: 12px;
        }

    )");


    // =====================================================
    // CONNECTIONS
    // =====================================================

    connect(
        chooseImageButton,
        &QPushButton::clicked,
        this,
        &SellBookWindow::selectImage
        );


    connect(
        publishButton,
        &QPushButton::clicked,
        this,
        &SellBookWindow::publishBook
        );


    connect(
        backButton,
        &QPushButton::clicked,
        this,
        &SellBookWindow::goBack
        );
}


// =========================================================
// SELECT IMAGE
// =========================================================

void SellBookWindow::selectImage()
{
    QString fileName =
        QFileDialog::getOpenFileName(
            this,
            "Select Book Image",
            "",
            "Images (*.png *.jpg *.jpeg)"
            );


    if (fileName.isEmpty())
    {
        return;
    }


    imagePath =
        fileName;


    QPixmap pixmap(
        fileName
        );


    if (pixmap.isNull())
    {
        QMessageBox::warning(
            this,
            "Image Error",
            "Unable to load the selected image."
            );

        imagePath.clear();

        return;
    }


    QPixmap scaledPixmap =
        pixmap.scaled(
            imagePreview->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            );


    imagePreview->setPixmap(
        scaledPixmap
        );


    imagePreview->setText(
        ""
        );
}


// =========================================================
// PUBLISH BOOK
// =========================================================

void SellBookWindow::publishBook()
{
    QString title =
        titleEdit->text().trimmed();

    QString author =
        authorEdit->text().trimmed();

    QString isbn =
        isbnEdit->text().trimmed();

    QString edition =
        editionEdit->text().trimmed();

    QString category =
        categoryCombo->currentText();

    QString condition =
        conditionCombo->currentText();

    double price =
        priceSpinBox->value();

    QString location =
        locationEdit->text().trimmed();

    QString description =
        descriptionEdit->toPlainText().trimmed();


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


    if (price <= 0)
    {
        QMessageBox::warning(
            this,
            "Invalid Price",
            "Please enter a valid selling price."
            );

        priceSpinBox->setFocus();

        return;
    }


    if (location.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Missing Information",
            "Please enter your location."
            );

        locationEdit->setFocus();

        return;
    }


    publishButton->setEnabled(false);
    publishButton->setText("Publishing...");

    QJsonObject body;
    body[QStringLiteral("title")] = title;
    body[QStringLiteral("author")] = author;
    body[QStringLiteral("price")] = price;
    body[QStringLiteral("condition")] = condition;
    body[QStringLiteral("category")] = category;
    body[QStringLiteral("isbn")] = isbn;
    body[QStringLiteral("description")] = description;
    body[QStringLiteral("coverImage")] = imagePath;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/books"), body, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply, title, price]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        publishButton->setEnabled(true);
        publishButton->setText("Publish Book");

        if (ok) {
            QMessageBox::information(
                this,
                "Book Published",
                "Your book has been successfully listed!\n\n"
                "Title: " + title +
                "\nPrice: ₹" + QString::number(price, 'f', 2)
            );

            emit bookPublished();
            close();
        } else {
            QMessageBox::critical(
                this,
                "Unable to Publish",
                errorMsg.isEmpty() ? "Failed to publish listing." : errorMsg
            );
        }
    });
}


// =========================================================
// GO BACK
// =========================================================

void SellBookWindow::goBack()
{
    emit backRequested();

    close();
}