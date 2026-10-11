#include "ProposeExchangeDialog.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QFrame>

ProposeExchangeDialog::ProposeExchangeDialog(const BookModel &requestedBook,
                                             const QVector<BookModel> &userBooks,
                                             QWidget *parent)
    : QDialog(parent),
      m_userBooks(userBooks),
      m_booksCombo(nullptr),
      m_messageEdit(nullptr),
      m_offeredTitle(nullptr),
      m_offeredMeta(nullptr),
      m_offeredPrice(nullptr)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);

    setupUI(requestedBook, userBooks);
}

void ProposeExchangeDialog::setupUI(const BookModel &requestedBook, const QVector<BookModel> &userBooks)
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);

    auto *card = new QFrame(this);
    card->setObjectName("dialogCard");
    card->setStyleSheet(QString(R"(
        #dialogCard {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 16px;
        }
    )").arg(AppStyle::BorderSubtle));
    AppStyle::applyElevation(card, 24, 8, 25);

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 24, 24, 20);
    cardLayout->setSpacing(14);

    // Header
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    auto *iconLabel = new QLabel(card);
    iconLabel->setFixedSize(40, 40);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setText("⇄");
    iconLabel->setStyleSheet(QString(R"(
        background-color: %1;
        color: %2;
        border-radius: 20px;
        font-size: 20px;
        font-weight: 800;
        border: none;
    )").arg(AppStyle::PrimaryLight, AppStyle::Primary));
    headerLayout->addWidget(iconLabel);

    auto *titlesLayout = new QVBoxLayout();
    titlesLayout->setSpacing(2);

    auto *titleLabel = new QLabel("Propose Book Exchange", card);
    titleLabel->setStyleSheet(QString("font-size: 17px; font-weight: 800; color: %1; border: none;").arg(AppStyle::TextPrimary));
    titlesLayout->addWidget(titleLabel);

    auto *subLabel = new QLabel("Select one of your listed books to offer in exchange.", card);
    subLabel->setStyleSheet(QString("font-size: 12px; color: %1; border: none;").arg(AppStyle::TextSecondary));
    titlesLayout->addWidget(subLabel);

    headerLayout->addLayout(titlesLayout, 1);

    auto *closeBtn = new QPushButton("✕", card);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedSize(28, 28);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #F1F5F9;
            color: #64748B;
            border: none;
            border-radius: 14px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #E2E8F0;
            color: #0F172A;
        }
    )");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    headerLayout->addWidget(closeBtn, 0, Qt::AlignTop);

    cardLayout->addLayout(headerLayout);

    // Target Book Card (The book the user wants)
    auto *targetCard = new QFrame(card);
    targetCard->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 10px;");
    auto *targetLayout = new QVBoxLayout(targetCard);
    targetLayout->setContentsMargins(12, 10, 12, 10);
    targetLayout->setSpacing(4);

    auto *targetTag = new QLabel("YOU WILL RECEIVE", targetCard);
    targetTag->setStyleSheet(QString("font-size: 11px; font-weight: 800; color: %1; letter-spacing: 0.5px; border: none;").arg(AppStyle::Primary));
    targetLayout->addWidget(targetTag);

    auto *targetTitle = new QLabel(requestedBook.title, targetCard);
    targetTitle->setStyleSheet(QString("font-size: 15px; font-weight: 700; color: %1; border: none;").arg(AppStyle::TextPrimary));
    targetLayout->addWidget(targetTitle);

    auto *targetMeta = new QLabel(QString("by %1 • Condition: %2 • Value: ₹%3 • Seller: %4")
                                      .arg(requestedBook.author, requestedBook.condition, QString::number(requestedBook.price, 'f', 0), requestedBook.owner.username), targetCard);
    targetMeta->setStyleSheet(QString("font-size: 12px; color: %1; border: none;").arg(AppStyle::TextSecondary));
    targetLayout->addWidget(targetMeta);

    cardLayout->addWidget(targetCard);

    // Exchange Arrow Icon
    auto *arrowRow = new QHBoxLayout();
    auto *arrowLbl = new QLabel("⇅", card);
    arrowLbl->setStyleSheet(QString("font-size: 20px; font-weight: 800; color: %1; border: none;").arg(AppStyle::TextMuted));
    arrowLbl->setAlignment(Qt::AlignCenter);
    arrowRow->addWidget(arrowLbl);
    cardLayout->addLayout(arrowRow);

    // Offered Book Section
    auto *offerSectionLbl = new QLabel("YOU WILL OFFER *", card);
    offerSectionLbl->setStyleSheet(QString("font-size: 11px; font-weight: 800; color: %1; letter-spacing: 0.5px; border: none;").arg(AppStyle::Success));
    cardLayout->addWidget(offerSectionLbl);

    m_booksCombo = new QComboBox(card);
    m_booksCombo->setFixedHeight(40);
    m_booksCombo->setCursor(Qt::PointingHandCursor);
    m_booksCombo->setStyleSheet(QString(R"(
        QComboBox {
            background-color: #F8FAFC;
            border: 1px solid %1;
            border-radius: 8px;
            padding: 4px 12px;
            font-size: 13px;
            color: %2;
        }
        QComboBox:hover {
            border-color: %3;
        }
        QComboBox QAbstractItemView {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 8px;
            selection-background-color: %4;
            selection-color: %3;
            outline: none;
            padding: 4px;
        }
    )").arg(AppStyle::BorderStrong, AppStyle::TextPrimary, AppStyle::Primary, AppStyle::PrimaryLight));

    for (int i = 0; i < userBooks.size(); ++i) {
        const auto &b = userBooks[i];
        QString displayText = QString("%1 by %2 (₹%3, %4)")
                                  .arg(b.title, b.author, QString::number(b.price, 'f', 0), b.condition);
        m_booksCombo->addItem(displayText, b.id);
    }
    cardLayout->addWidget(m_booksCombo);

    // Offered Book Details Preview Card
    auto *offeredCard = new QFrame(card);
    offeredCard->setStyleSheet("background-color: #F8FAFC; border: 1px dashed #CBD5E1; border-radius: 8px;");
    auto *offeredLayout = new QVBoxLayout(offeredCard);
    offeredLayout->setContentsMargins(12, 10, 12, 10);
    offeredLayout->setSpacing(2);

    m_offeredTitle = new QLabel("", offeredCard);
    m_offeredTitle->setStyleSheet(QString("font-size: 14px; font-weight: 700; color: %1; border: none;").arg(AppStyle::TextPrimary));
    offeredLayout->addWidget(m_offeredTitle);

    m_offeredMeta = new QLabel("", offeredCard);
    m_offeredMeta->setStyleSheet(QString("font-size: 12px; color: %1; border: none;").arg(AppStyle::TextSecondary));
    offeredLayout->addWidget(m_offeredMeta);

    m_offeredPrice = new QLabel("", offeredCard);
    m_offeredPrice->setStyleSheet(QString("font-size: 12px; font-weight: 700; color: %1; border: none;").arg(AppStyle::Primary));
    offeredLayout->addWidget(m_offeredPrice);

    cardLayout->addWidget(offeredCard);

    connect(m_booksCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProposeExchangeDialog::onBookSelectionChanged);

    if (!userBooks.isEmpty()) {
        updateOfferedBookPreview(userBooks.first());
    }

    // Message Input
    auto *msgLbl = new QLabel("Message for Book Owner (Optional)", card);
    msgLbl->setStyleSheet(QString("font-size: 12px; font-weight: 600; color: %1; border: none;").arg(AppStyle::TextSecondary));
    cardLayout->addWidget(msgLbl);

    m_messageEdit = new QTextEdit(card);
    m_messageEdit->setFixedHeight(68);
    m_messageEdit->setPlaceholderText("Hi! I would love to trade my copy for yours. It's in great condition.");
    m_messageEdit->setStyleSheet(QString(R"(
        QTextEdit {
            background-color: #F8FAFC;
            border: 1px solid %1;
            border-radius: 8px;
            padding: 8px;
            font-size: 13px;
            color: %2;
        }
        QTextEdit:focus {
            border-color: %3;
            background-color: #FFFFFF;
        }
    )").arg(AppStyle::BorderStrong, AppStyle::TextPrimary, AppStyle::Primary));
    cardLayout->addWidget(m_messageEdit);

    // Button Row
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    auto *cancelBtn = new QPushButton("Cancel", card);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setFixedHeight(40);
    cancelBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn, 1);

    auto *submitBtn = new QPushButton("Send Exchange Proposal", card);
    submitBtn->setCursor(Qt::PointingHandCursor);
    submitBtn->setFixedHeight(40);
    submitBtn->setStyleSheet(AppStyle::primaryButtonStyle());
    connect(submitBtn, &QPushButton::clicked, this, [this]() {
        int idx = m_booksCombo ? m_booksCombo->currentIndex() : -1;
        if (idx >= 0 && idx < m_userBooks.size()) {
            m_selectedBookId = m_userBooks[idx].id;
        }
        if (m_messageEdit) {
            m_message = m_messageEdit->toPlainText().trimmed();
        }
        accept();
    });
    btnLayout->addWidget(submitBtn, 1);

    cardLayout->addLayout(btnLayout);

    setFixedSize(540, card->sizeHint().height() + 32);
    rootLayout->addWidget(card);
}

void ProposeExchangeDialog::onBookSelectionChanged(int index)
{
    if (index >= 0 && index < m_userBooks.size()) {
        updateOfferedBookPreview(m_userBooks[index]);
    }
}

void ProposeExchangeDialog::updateOfferedBookPreview(const BookModel &book)
{
    if (m_offeredTitle) {
        m_offeredTitle->setText(book.title);
    }
    if (m_offeredMeta) {
        m_offeredMeta->setText(QString("by %1 • Condition: %2 • Category: %3").arg(book.author, book.condition, book.category));
    }
    if (m_offeredPrice) {
        m_offeredPrice->setText(QString("Listing Value: ₹%1").arg(QString::number(book.price, 'f', 0)));
    }
}

QString ProposeExchangeDialog::selectedOfferedBookId() const
{
    return m_selectedBookId;
}

QString ProposeExchangeDialog::proposalMessage() const
{
    return m_message;
}
