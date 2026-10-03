#include "sellbookwindow.h"
#include "network/ApiClient.h"
#include "AppStyle.h"

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
#include "StyledMessageBox.h"
#include <QPixmap>
#include <QFileInfo>
#include <QScrollArea>
#include <QFrame>

SellBookWindow::SellBookWindow(const QString &sellerEmail, QWidget *parent)
    : QWidget(parent),
    sellerEmail(sellerEmail),
    imagePreview(nullptr),
    chooseImageButton(nullptr),
    titleEdit(nullptr),
    authorEdit(nullptr),
    isbnEdit(nullptr),
    editionEdit(nullptr),
    categoryCombo(nullptr),
    conditionCombo(nullptr),
    priceSpinBox(nullptr),
    descriptionEdit(nullptr),
    publishButton(nullptr),
    backButton(nullptr)
{
    setWindowTitle("BookBazzar - List a Book for Sale");
    resize(1200, 800);
    setMinimumSize(960, 640);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    setupUI();
}

void SellBookWindow::setupUI()
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
    pageLayout->setSpacing(28);

    // Top Bar
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

    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(36, 10, 36, 10);
    topLayout->setSpacing(16);

    backButton = new QPushButton("← Back");
    backButton->setMinimumHeight(38);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setStyleSheet(AppStyle::secondaryButtonStyle());

    QLabel *pageTitle = new QLabel("List a Book for Sale or Exchange");
    pageTitle->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: 800;").arg(AppStyle::TextPrimary));

    topLayout->addWidget(backButton);
    topLayout->addWidget(pageTitle);
    topLayout->addStretch();
    pageLayout->addWidget(topBar);

    // Form Container
    QWidget *formContainer = new QWidget();
    QHBoxLayout *formLayout = new QHBoxLayout(formContainer);
    formLayout->setContentsMargins(36, 0, 36, 0);
    formLayout->setSpacing(32);

    // Left Column: Cover Image Card
    QFrame *leftCard = new QFrame();
    leftCard->setFixedWidth(320);
    leftCard->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(leftCard, 20, 6, 18);

    QVBoxLayout *leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(24, 24, 24, 24);
    leftLayout->setSpacing(16);

    QLabel *coverTitle = new QLabel("Book Cover Photo");
    coverTitle->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 700;").arg(AppStyle::TextPrimary));
    leftLayout->addWidget(coverTitle);

    imagePreview = new QLabel("No Image Selected\n\n(A stylized cover will be generated automatically if none is chosen)");
    imagePreview->setAlignment(Qt::AlignCenter);
    imagePreview->setWordWrap(true);
    imagePreview->setFixedHeight(320);
    imagePreview->setStyleSheet(QString(R"(
        QLabel {
            background-color: %1;
            border: 2px dashed %2;
            border-radius: 12px;
            color: %3;
            font-size: 12px;
            padding: 16px;
        }
    )").arg(AppStyle::SurfaceSubtle, AppStyle::BorderStrong, AppStyle::TextMuted));
    leftLayout->addWidget(imagePreview);

    chooseImageButton = new QPushButton("Choose Cover Image");
    chooseImageButton->setMinimumHeight(42);
    chooseImageButton->setCursor(Qt::PointingHandCursor);
    chooseImageButton->setStyleSheet(AppStyle::secondaryButtonStyle());
    leftLayout->addWidget(chooseImageButton);
    leftLayout->addStretch();

    formLayout->addWidget(leftCard);

    // Right Column: Information Card
    QFrame *rightCard = new QFrame();
    rightCard->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(rightCard, 20, 6, 18);

    QVBoxLayout *rightLayout = new QVBoxLayout(rightCard);
    rightLayout->setContentsMargins(32, 28, 32, 28);
    rightLayout->setSpacing(16);

    QLabel *infoHeading = new QLabel("Listing Details");
    infoHeading->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: 800;").arg(AppStyle::TextPrimary));
    rightLayout->addWidget(infoHeading);

    auto addLabel = [](const QString &text) {
        QLabel *lbl = new QLabel(text);
        lbl->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600;").arg(AppStyle::TextSecondary));
        return lbl;
    };

    // Title & Author row
    QHBoxLayout *row1 = new QHBoxLayout();
    row1->setSpacing(16);

    QVBoxLayout *colTitle = new QVBoxLayout();
    colTitle->addWidget(addLabel("Book Title *"));
    titleEdit = new QLineEdit();
    titleEdit->setPlaceholderText("e.g. Introduction to Algorithms");
    titleEdit->setStyleSheet(AppStyle::inputStyle());
    titleEdit->setMinimumHeight(42);
    colTitle->addWidget(titleEdit);

    QVBoxLayout *colAuthor = new QVBoxLayout();
    colAuthor->addWidget(addLabel("Author *"));
    authorEdit = new QLineEdit();
    authorEdit->setPlaceholderText("e.g. Cormen, Leiserson, Rivest");
    authorEdit->setStyleSheet(AppStyle::inputStyle());
    authorEdit->setMinimumHeight(42);
    colAuthor->addWidget(authorEdit);

    row1->addLayout(colTitle, 1);
    row1->addLayout(colAuthor, 1);
    rightLayout->addLayout(row1);

    // Category, Condition, Price row
    QHBoxLayout *row2 = new QHBoxLayout();
    row2->setSpacing(16);

    QVBoxLayout *colCat = new QVBoxLayout();
    colCat->addWidget(addLabel("Category"));
    categoryCombo = new QComboBox();
    categoryCombo->setStyleSheet(AppStyle::comboBoxStyle());
    categoryCombo->addItems({"Academic", "Programming", "Engineering", "Fiction", "Competitive Exams", "School", "Novels", "Non-Fiction", "Other"});
    colCat->addWidget(categoryCombo);

    QVBoxLayout *colCond = new QVBoxLayout();
    colCond->addWidget(addLabel("Condition"));
    conditionCombo = new QComboBox();
    conditionCombo->setStyleSheet(AppStyle::comboBoxStyle());
    conditionCombo->addItems({"Like New", "Very Good", "Good", "Acceptable", "New"});
    colCond->addWidget(conditionCombo);

    QVBoxLayout *colPrice = new QVBoxLayout();
    colPrice->addWidget(addLabel("Price (₹) *"));
    priceSpinBox = new QDoubleSpinBox();
    priceSpinBox->setRange(1.0, 50000.0);
    priceSpinBox->setPrefix("₹ ");
    priceSpinBox->setValue(199.0);
    priceSpinBox->setDecimals(0);
    priceSpinBox->setSingleStep(25.0);
    priceSpinBox->setStyleSheet(AppStyle::inputStyle());
    priceSpinBox->setMinimumHeight(42);
    colPrice->addWidget(priceSpinBox);

    row2->addLayout(colCat, 1);
    row2->addLayout(colCond, 1);
    row2->addLayout(colPrice, 1);
    rightLayout->addLayout(row2);

    // ISBN & Edition row
    QHBoxLayout *row3 = new QHBoxLayout();
    row3->setSpacing(16);

    QVBoxLayout *colIsbn = new QVBoxLayout();
    colIsbn->addWidget(addLabel("ISBN (Optional)"));
    isbnEdit = new QLineEdit();
    isbnEdit->setPlaceholderText("e.g. 9780262033848");
    isbnEdit->setStyleSheet(AppStyle::inputStyle());
    isbnEdit->setMinimumHeight(42);
    colIsbn->addWidget(isbnEdit);

    QVBoxLayout *colEdition = new QVBoxLayout();
    colEdition->addWidget(addLabel("Edition / Year (Optional)"));
    editionEdit = new QLineEdit();
    editionEdit->setPlaceholderText("e.g. 3rd Edition, 2021");
    editionEdit->setStyleSheet(AppStyle::inputStyle());
    editionEdit->setMinimumHeight(42);
    colEdition->addWidget(editionEdit);

    row3->addLayout(colIsbn, 1);
    row3->addLayout(colEdition, 1);
    rightLayout->addLayout(row3);

    // Description
    rightLayout->addWidget(addLabel("Book Description (Optional)"));
    descriptionEdit = new QTextEdit();
    descriptionEdit->setPlaceholderText("Provide details about the book's contents, highlight marks, missing pages, or extra study materials included...");
    descriptionEdit->setStyleSheet(AppStyle::inputStyle());
    descriptionEdit->setMinimumHeight(100);
    rightLayout->addWidget(descriptionEdit);

    // Publish button
    publishButton = new QPushButton("Publish Listing");
    publishButton->setMinimumHeight(48);
    publishButton->setCursor(Qt::PointingHandCursor);
    publishButton->setStyleSheet(AppStyle::primaryButtonStyle());
    rightLayout->addWidget(publishButton);

    formLayout->addWidget(rightCard, 1);
    pageLayout->addWidget(formContainer);

    scrollArea->setWidget(page);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(scrollArea);

    connect(chooseImageButton, &QPushButton::clicked, this, &SellBookWindow::selectImage);
    connect(publishButton, &QPushButton::clicked, this, &SellBookWindow::publishBook);
    connect(backButton, &QPushButton::clicked, this, &SellBookWindow::goBack);
}

void SellBookWindow::selectImage()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "Select Book Cover Image",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.webp)"
    );

    if (file.isEmpty()) return;

    imagePath = file;
    QPixmap p(imagePath);
    if (!p.isNull()) {
        imagePreview->setPixmap(p.scaled(260, 320, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void SellBookWindow::publishBook()
{
    QString title = titleEdit->text().trimmed();
    QString author = authorEdit->text().trimmed();
    QString isbn = isbnEdit->text().trimmed();
    QString category = categoryCombo->currentText();
    QString condition = conditionCombo->currentText();
    double price = priceSpinBox->value();
    QString description = descriptionEdit->toPlainText().trimmed();

    if (!editionEdit->text().trimmed().isEmpty()) {
        if (!description.isEmpty()) description += "\n\n";
        description += "Edition: " + editionEdit->text().trimmed();
    }

    if (title.isEmpty()) {
        StyledMessageBox::warning(this, "Missing Information", "Please enter the book title.");
        titleEdit->setFocus();
        return;
    }

    if (author.isEmpty()) {
        StyledMessageBox::warning(this, "Missing Information", "Please enter the author name.");
        authorEdit->setFocus();
        return;
    }

    if (price <= 0) {
        StyledMessageBox::warning(this, "Invalid Price", "Please enter a valid selling price greater than 0.");
        priceSpinBox->setFocus();
        return;
    }

    publishButton->setEnabled(false);
    publishButton->setText("Publishing Listing...");

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
        publishButton->setText("Publish Listing");

        if (ok) {
            StyledMessageBox::success(
                this,
                "Book Listed Successfully",
                QString("Your book has been published to the marketplace!\n\nTitle: %1\nPrice: ₹%2")
                    .arg(title)
                    .arg(QString::number(price, 'f', 0))
            );
            emit bookPublished();
            close();
        } else {
            StyledMessageBox::critical(
                this,
                "Unable to Publish",
                errorMsg.isEmpty() ? "Failed to publish listing." : errorMsg
            );
        }
    });
}

void SellBookWindow::goBack()
{
    emit backRequested();
    close();
}