#include "CancelOrderDialog.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QFrame>

CancelOrderDialog::CancelOrderDialog(Mode mode, const QString &referenceText, QWidget *parent)
    : QDialog(parent),
      m_reasonCombo(nullptr),
      m_commentsEdit(nullptr)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);

    setupUI(mode, referenceText);
}

void CancelOrderDialog::setupUI(Mode mode, const QString &referenceText)
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);

    auto *card = new QFrame(this);
    card->setObjectName("cancelCard");
    card->setStyleSheet(QString(R"(
        #cancelCard {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 16px;
        }
    )").arg(AppStyle::BorderSubtle));
    AppStyle::applyElevation(card, 24, 8, 25);

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 24, 24, 20);
    cardLayout->setSpacing(16);

    // Top Header Row
    auto *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    auto *iconLabel = new QLabel(card);
    iconLabel->setFixedSize(40, 40);
    iconLabel->setAlignment(Qt::AlignCenter);

    QString titleText;
    QString subText;
    QString confirmBtnText;
    QString confirmBtnStyle;
    QString iconSymbol;
    QString iconBg;
    QString iconColor;
    QStringList reasonsList;

    if (mode == Cancel) {
        titleText = "Cancel Order";
        subText = "Are you sure you want to cancel this order? This action cannot be undone.";
        confirmBtnText = "Confirm Cancellation";
        confirmBtnStyle = AppStyle::dangerButtonStyle();
        iconSymbol = "✕";
        iconBg = AppStyle::DangerLight;
        iconColor = AppStyle::Danger;
        reasonsList << "Ordered by mistake"
                    << "Found a better price elsewhere"
                    << "Delivery time is too long"
                    << "Shipping address is incorrect"
                    << "Payment or billing reason"
                    << "Other reason";
    } else if (mode == Return) {
        titleText = "Request Order Return";
        subText = "Please specify the reason why you are requesting to return this item.";
        confirmBtnText = "Submit Return Request";
        confirmBtnStyle = AppStyle::primaryButtonStyle();
        iconSymbol = "↺";
        iconBg = AppStyle::WarningLight;
        iconColor = AppStyle::Warning;
        reasonsList << "Received wrong item / wrong book"
                    << "Item damaged or defective during transit"
                    << "Condition significantly differs from description"
                    << "No longer needed"
                    << "Other reason";
    } else { // RejectExchange
        titleText = "Decline Exchange";
        subText = "Please select a reason for declining this book exchange proposal.";
        confirmBtnText = "Decline Proposal";
        confirmBtnStyle = AppStyle::dangerButtonStyle();
        iconSymbol = "✕";
        iconBg = AppStyle::DangerLight;
        iconColor = AppStyle::Danger;
        reasonsList << "Not interested in the offered book"
                    << "Book already scheduled for another exchange"
                    << "Book condition does not meet expectations"
                    << "Decided to keep my book"
                    << "Other reason";
    }

    iconLabel->setText(iconSymbol);
    iconLabel->setStyleSheet(QString(R"(
        background-color: %1;
        color: %2;
        border-radius: 20px;
        font-size: 18px;
        font-weight: 800;
        border: none;
    )").arg(iconBg, iconColor));
    headerLayout->addWidget(iconLabel);

    auto *titlesLayout = new QVBoxLayout();
    titlesLayout->setSpacing(2);

    auto *titleLabel = new QLabel(titleText, card);
    titleLabel->setStyleSheet(QString("font-size: 17px; font-weight: 800; color: %1; border: none;").arg(AppStyle::TextPrimary));
    titlesLayout->addWidget(titleLabel);

    auto *subLabel = new QLabel(subText, card);
    subLabel->setWordWrap(true);
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

    // Reference pill if provided
    if (!referenceText.trimmed().isEmpty()) {
        auto *refPill = new QFrame(card);
        refPill->setStyleSheet("background-color: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 8px;");
        auto *refLayout = new QHBoxLayout(refPill);
        refLayout->setContentsMargins(12, 8, 12, 8);
        auto *refLbl = new QLabel(referenceText, refPill);
        refLbl->setStyleSheet("font-size: 12px; font-weight: 700; color: #475569; border: none;");
        refLayout->addWidget(refLbl);
        cardLayout->addWidget(refPill);
    }

    // Reason dropdown
    auto *reasonLbl = new QLabel(mode == Return ? "Return Reason *" : "Reason *", card);
    reasonLbl->setStyleSheet(QString("font-size: 13px; font-weight: 700; color: %1; border: none;").arg(AppStyle::TextPrimary));
    cardLayout->addWidget(reasonLbl);

    m_reasonCombo = new QComboBox(card);
    m_reasonCombo->addItems(reasonsList);
    m_reasonCombo->setFixedHeight(40);
    m_reasonCombo->setCursor(Qt::PointingHandCursor);
    m_reasonCombo->setStyleSheet(QString(R"(
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
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 28px;
            border-left-width: 0px;
            border-top-right-radius: 8px;
            border-bottom-right-radius: 8px;
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
    cardLayout->addWidget(m_reasonCombo);

    // Additional comments
    auto *commentsLbl = new QLabel("Additional Details (Optional)", card);
    commentsLbl->setStyleSheet(QString("font-size: 12px; font-weight: 600; color: %1; border: none;").arg(AppStyle::TextSecondary));
    cardLayout->addWidget(commentsLbl);

    m_commentsEdit = new QTextEdit(card);
    m_commentsEdit->setFixedHeight(72);
    m_commentsEdit->setPlaceholderText("Please provide any additional comments or context...");
    m_commentsEdit->setStyleSheet(QString(R"(
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
    cardLayout->addWidget(m_commentsEdit);

    // Notice info card
    if (mode == Cancel) {
        auto *noticeBox = new QFrame(card);
        noticeBox->setStyleSheet("background-color: #FEF2F2; border: 1px solid #FEE2E2; border-radius: 8px;");
        auto *noticeLayout = new QHBoxLayout(noticeBox);
        noticeLayout->setContentsMargins(12, 8, 12, 8);
        auto *noticeLbl = new QLabel("Any payment processed will be refunded automatically to your account.", noticeBox);
        noticeLbl->setStyleSheet("color: #991B1B; font-size: 11px; font-weight: 600; border: none;");
        noticeLayout->addWidget(noticeLbl);
        cardLayout->addWidget(noticeBox);
    }

    // Button Row
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    auto *keepBtn = new QPushButton(mode == Cancel ? "Keep Order" : "Cancel", card);
    keepBtn->setCursor(Qt::PointingHandCursor);
    keepBtn->setFixedHeight(40);
    keepBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(keepBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(keepBtn, 1);

    auto *confirmBtn = new QPushButton(confirmBtnText, card);
    confirmBtn->setCursor(Qt::PointingHandCursor);
    confirmBtn->setFixedHeight(40);
    confirmBtn->setStyleSheet(confirmBtnStyle);
    connect(confirmBtn, &QPushButton::clicked, this, [this]() {
        QString selected = m_reasonCombo ? m_reasonCombo->currentText() : "Other reason";
        QString details = m_commentsEdit ? m_commentsEdit->toPlainText().trimmed() : "";
        if (!details.isEmpty()) {
            m_selectedReason = QString("%1: %2").arg(selected, details);
        } else {
            m_selectedReason = selected;
        }
        accept();
    });
    btnLayout->addWidget(confirmBtn, 1);

    cardLayout->addLayout(btnLayout);

    setFixedSize(500, card->sizeHint().height() + 32);
    rootLayout->addWidget(card);
}

QString CancelOrderDialog::reason() const
{
    return m_selectedReason;
}

QString CancelOrderDialog::getCancelReason(QWidget *parent, const QString &orderNumber)
{
    QString ref = orderNumber.isEmpty() ? QString() : QString("Order: %1").arg(orderNumber);
    CancelOrderDialog dlg(Cancel, ref, parent);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.reason();
    }
    return QString();
}

QString CancelOrderDialog::getReturnReason(QWidget *parent, const QString &orderNumber)
{
    QString ref = orderNumber.isEmpty() ? QString() : QString("Order: %1").arg(orderNumber);
    CancelOrderDialog dlg(Return, ref, parent);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.reason();
    }
    return QString();
}

QString CancelOrderDialog::getRejectExchangeReason(QWidget *parent, const QString &proposalNumber)
{
    QString ref = proposalNumber.isEmpty() ? QString() : QString("Exchange Proposal: %1").arg(proposalNumber);
    CancelOrderDialog dlg(RejectExchange, ref, parent);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.reason();
    }
    return QString();
}
