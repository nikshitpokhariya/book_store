#include "listingswindow.h"
#include "network/ApiClient.h"

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
#include <QMessageBox>
#include <QFileInfo>
#include <QPixmap>
#include <QNetworkReply>

ListingsWindow::ListingsWindow(const QString &userName, QWidget *parent)
    : QWidget(parent),
      userName(userName),
      totalCountLabel(nullptr),
      availableCountLabel(nullptr),
      soldCountLabel(nullptr),
      cardsLayout(nullptr)
{
    setWindowTitle("BookBazzar - My Book Listings");
    resize(1200, 800);
    setMinimumSize(950, 600);

    setupUI();
    refreshListings();
}

ListingsWindow::~ListingsWindow()
{
}

void ListingsWindow::setupUI()
{
    setStyleSheet(R"(
        QWidget {
            font-family: "Segoe UI";
            background-color: #F8FAFC;
        }
        QScrollArea {
            border: none;
            background-color: #F8FAFC;
        }
        QScrollBar:vertical {
            width: 8px;
            background: transparent;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #CBD5E1;
            border-radius: 4px;
            min-height: 40px;
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Top Navigation Bar
    mainLayout->addWidget(createTopBar());

    // Scrollable Body
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *content = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 30, 40, 40);
    contentLayout->setSpacing(24);

    // Stats bar
    contentLayout->addWidget(createStatsBar());

    // Cards container
    cardsLayout = new QVBoxLayout();
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(14);
    contentLayout->addLayout(cardsLayout);

    contentLayout->addStretch();
    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

QWidget* ListingsWindow::createTopBar()
{
    QFrame *bar = new QFrame();
    bar->setFixedHeight(74);
    bar->setStyleSheet("QFrame { background-color: white; border-bottom: 1px solid #E2E8F0; }");

    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(36, 12, 36, 12);
    layout->setSpacing(16);

    QPushButton *backButton = new QPushButton("← Back");
    backButton->setFixedSize(90, 40);
    backButton->setStyleSheet(R"(
        QPushButton {
            background-color: #F1F5F9;
            color: #334155;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
        }
        QPushButton:hover { background-color: #E2E8F0; }
    )");
    connect(backButton, &QPushButton::clicked, this, &ListingsWindow::handleBack);
    layout->addWidget(backButton);

    QLabel *title = new QLabel("My Book Listings");
    title->setStyleSheet("QLabel { color: #0F172A; font-size: 20px; font-weight: 800; }");
    layout->addWidget(title);

    layout->addStretch();

    QPushButton *addBtn = new QPushButton("+ Sell Another Book");
    addBtn->setFixedHeight(40);
    addBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #6366F1;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 0 18px;
            font-size: 13px;
            font-weight: 700;
        }
        QPushButton:hover { background-color: #4F46E5; }
    )");
    connect(addBtn, &QPushButton::clicked, this, &ListingsWindow::handleAddNew);
    layout->addWidget(addBtn);

    return bar;
}

QWidget* ListingsWindow::createStatsBar()
{
    QWidget *container = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(18);

    auto createStatCard = [](const QString &title, QLabel* &labelRef, const QString &color) {
        QFrame *card = new QFrame();
        card->setStyleSheet("QFrame { background-color: white; border: 1px solid #E2E8F0; border-radius: 12px; }");
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 16, 20, 16);
        cardLayout->setSpacing(4);

        QLabel *titleLabel = new QLabel(title);
        titleLabel->setStyleSheet("QLabel { color: #64748B; font-size: 12px; font-weight: 600; }");
        cardLayout->addWidget(titleLabel);

        labelRef = new QLabel("0");
        labelRef->setStyleSheet(QString("QLabel { color: %1; font-size: 26px; font-weight: 800; }").arg(color));
        cardLayout->addWidget(labelRef);

        return card;
    };

    layout->addWidget(createStatCard("Total Listings", totalCountLabel, "#0F172A"));
    layout->addWidget(createStatCard("Active / Available", availableCountLabel, "#10B981"));
    layout->addWidget(createStatCard("Books Sold", soldCountLabel, "#6366F1"));

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

        // Clear previous items
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

        // Use pagination.total for the real count across all pages
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
            empty->setMinimumHeight(220);
            empty->setStyleSheet(R"(
                QLabel {
                    background-color: white;
                    border: 1px dashed #CBD5E1;
                    border-radius: 16px;
                    color: #64748B;
                    font-size: 15px;
                    font-weight: 600;
                }
            )");
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
    card->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border: 1px solid #E2E8F0;
            border-radius: 14px;
        }
        QFrame:hover {
            border-color: #CBD5E1;
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(card);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(18);

    // Book Cover thumbnail
    QLabel *cover = new QLabel();
    cover->setFixedSize(60, 80);
    cover->setAlignment(Qt::AlignCenter);
    cover->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #4F46E5, stop:1 #7C3AED); border-radius: 6px; color: white; font-weight: 800; font-size: 14px;");

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
    titleLbl->setStyleSheet("QLabel { color: #0F172A; font-size: 16px; font-weight: 700; }");
    infoLayout->addWidget(titleLbl);

    QLabel *authorLbl = new QLabel("by " + book.author);
    authorLbl->setStyleSheet("QLabel { color: #64748B; font-size: 13px; }");
    infoLayout->addWidget(authorLbl);

    QHBoxLayout *metaLayout = new QHBoxLayout();
    metaLayout->setSpacing(8);

    auto makeBadge = [](const QString &text, const QString &bg, const QString &fg) {
        QLabel *b = new QLabel(text);
        b->setStyleSheet(QString("QLabel { background-color: %1; color: %2; border-radius: 6px; padding: 3px 8px; font-size: 11px; font-weight: 700; }").arg(bg, fg));
        return b;
    };

    metaLayout->addWidget(makeBadge(book.category.isEmpty() ? "General" : book.category, "#EFF6FF", "#2563EB"));
    metaLayout->addWidget(makeBadge(book.condition.isEmpty() ? "Good" : book.condition, "#F1F5F9", "#475569"));
    metaLayout->addStretch();

    infoLayout->addLayout(metaLayout);
    layout->addLayout(infoLayout, 1);

    // Status Badge
    QLabel *statusBadge = new QLabel(book.status.toUpper());
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setFixedSize(90, 30);
    if (book.status.compare("sold", Qt::CaseInsensitive) == 0) {
        statusBadge->setStyleSheet("QLabel { background-color: #FEF3C7; color: #D97706; border-radius: 6px; font-size: 11px; font-weight: 800; }");
    } else if (book.status.compare("exchanged", Qt::CaseInsensitive) == 0) {
        statusBadge->setStyleSheet("QLabel { background-color: #F3E8FF; color: #7E22CE; border-radius: 6px; font-size: 11px; font-weight: 800; }");
    } else {
        statusBadge->setStyleSheet("QLabel { background-color: #ECFDF5; color: #059669; border-radius: 6px; font-size: 11px; font-weight: 800; }");
    }
    layout->addWidget(statusBadge);

    // Price
    QLabel *priceLbl = new QLabel(QString("₹%1").arg(QString::number(book.price, 'f', 0)));
    priceLbl->setStyleSheet("QLabel { color: #0F172A; font-size: 18px; font-weight: 900; }");
    layout->addWidget(priceLbl);

    layout->addSpacing(10);

    const bool isAvailable = (book.status.compare("available", Qt::CaseInsensitive) == 0);

    // Edit Button
    QPushButton *editBtn = new QPushButton("Edit");
    editBtn->setFixedSize(70, 36);
    editBtn->setEnabled(isAvailable);
    editBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #F1F5F9;
            color: #334155;
            border: 1px solid #CBD5E1;
            border-radius: 8px;
            font-size: 12px;
            font-weight: 700;
        }
        QPushButton:hover { background-color: #E2E8F0; }
        QPushButton:disabled { color: #94A3B8; border-color: #E2E8F0; }
    )");
    connect(editBtn, &QPushButton::clicked, this, [this, book]() {
        handleEditListing(book);
    });
    layout->addWidget(editBtn);

    // Delete Button
    QPushButton *deleteBtn = new QPushButton("Delete");
    deleteBtn->setFixedSize(75, 36);
    deleteBtn->setEnabled(isAvailable);
    deleteBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #FFF1F2;
            color: #E11D48;
            border: 1px solid #FFE4E6;
            border-radius: 8px;
            font-size: 12px;
            font-weight: 700;
        }
        QPushButton:hover { background-color: #FFE4E6; color: #BE123C; }
        QPushButton:disabled { color: #FDA4AF; border-color: #FFF1F2; }
    )");
    connect(deleteBtn, &QPushButton::clicked, this, [this, book]() {
        handleDeleteListing(book.id, book.title);
    });
    layout->addWidget(deleteBtn);

    return card;
}

void ListingsWindow::handleDeleteListing(const QString &bookId, const QString &bookTitle)
{
    QMessageBox::StandardButton res = QMessageBox::question(
        this, "Delete Listing",
        QString("Are you sure you want to remove \"%1\" from your listings?").arg(bookTitle),
        QMessageBox::Yes | QMessageBox::No
    );

    if (res == QMessageBox::Yes) {
        auto *reply = ApiClient::instance().deleteResource(QStringLiteral("/api/books/") + bookId);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
            reply->deleteLater();

            if (ok) {
                QMessageBox::information(this, "Listing Removed", "Your book listing has been removed successfully.");
                refreshListings();
            } else {
                QMessageBox::warning(this, "Error", errorMsg.isEmpty() ? "Could not delete this book listing." : errorMsg);
            }
        });
    }
}

void ListingsWindow::handleEditListing(const BookModel &book)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Edit Book Listing");
    dialog.resize(500, 520);
    dialog.setStyleSheet("background-color: white;");

    QVBoxLayout layout(&dialog);
    layout.setSpacing(12);
    layout.setContentsMargins(25, 20, 25, 20);

    QLabel *hdr = new QLabel("Edit Book Details", &dialog);
    hdr->setStyleSheet("font-size: 18px; font-weight: 800; color: #1E293B;");
    layout.addWidget(hdr);

    QLineEdit titleEdit(book.title, &dialog);
    titleEdit.setPlaceholderText("Book Title");

    QLineEdit authorEdit(book.author, &dialog);
    authorEdit.setPlaceholderText("Author");

    QLineEdit priceEdit(QString::number(book.price, 'f', 2), &dialog);
    priceEdit.setPlaceholderText("Price (INR)");

    QComboBox categoryCombo(&dialog);
    categoryCombo.addItems({"Fiction", "Non-Fiction", "Academic", "Programming", "Engineering", "Competitive Exams", "School", "Novels", "Other"});
    int cIdx = categoryCombo.findText(book.category);
    if (cIdx >= 0) categoryCombo.setCurrentIndex(cIdx);

    QComboBox conditionCombo(&dialog);
    conditionCombo.addItems({"New", "Like New", "Very Good", "Good", "Acceptable"});
    int condIdx = conditionCombo.findText(book.condition);
    if (condIdx >= 0) conditionCombo.setCurrentIndex(condIdx);

    QTextEdit descEdit(&dialog);
    descEdit.setPlaceholderText("Description...");
    descEdit.setPlainText(book.description);
    descEdit.setMaximumHeight(90);

    QLineEdit imageEdit(book.coverImage, &dialog);
    imageEdit.setPlaceholderText("Cover Image URL");

    layout.addWidget(new QLabel("Title:"));
    layout.addWidget(&titleEdit);
    layout.addWidget(new QLabel("Author:"));
    layout.addWidget(&authorEdit);
    layout.addWidget(new QLabel("Price:"));
    layout.addWidget(&priceEdit);
    layout.addWidget(new QLabel("Category:"));
    layout.addWidget(&categoryCombo);
    layout.addWidget(new QLabel("Condition:"));
    layout.addWidget(&conditionCombo);
    layout.addWidget(new QLabel("Description:"));
    layout.addWidget(&descEdit);
    layout.addWidget(new QLabel("Cover Image URL:"));
    layout.addWidget(&imageEdit);

    QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout.addWidget(&buttons);

    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

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
                QMessageBox::information(this, "Success", "Listing updated successfully!");
                refreshListings();
            } else {
                QMessageBox::warning(this, "Update Failed", errorMsg.isEmpty() ? "Could not update listing." : errorMsg);
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
