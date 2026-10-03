#include "StyledMessageBox.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QGraphicsDropShadowEffect>

StyledMessageBox::StyledMessageBox(Type type,
                                   const QString &title,
                                   const QString &message,
                                   QWidget *parent,
                                   bool hasCancel,
                                   const QString &okText,
                                   const QString &cancelText)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);

    setupUI(type, title, message, hasCancel, okText, cancelText);
}

void StyledMessageBox::setupUI(Type type,
                               const QString &title,
                               const QString &message,
                               bool hasCancel,
                               const QString &okText,
                               const QString &cancelText)
{
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);

    QFrame *card = new QFrame(this);
    card->setObjectName("dialogCard");
    card->setStyleSheet(QString(R"(
        #dialogCard {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 16px;
        }
    )").arg(AppStyle::BorderSubtle));

    AppStyle::applyElevation(card, 24, 8, 25);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 24, 24, 20);
    cardLayout->setSpacing(16);

    // Top icon + title row
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    QLabel *iconLabel = new QLabel(card);
    iconLabel->setFixedSize(40, 40);
    iconLabel->setAlignment(Qt::AlignCenter);

    QString iconSymbol;
    QString iconBg;
    QString iconColor;

    switch (type) {
    case Success:
        iconSymbol = "✓";
        iconBg = AppStyle::SuccessLight;
        iconColor = AppStyle::Success;
        break;
    case Warning:
        iconSymbol = "!";
        iconBg = AppStyle::WarningLight;
        iconColor = AppStyle::Warning;
        break;
    case Error:
        iconSymbol = "✕";
        iconBg = AppStyle::DangerLight;
        iconColor = AppStyle::Danger;
        break;
    case Question:
        iconSymbol = "?";
        iconBg = AppStyle::PrimaryLight;
        iconColor = AppStyle::Primary;
        break;
    case Info:
    default:
        iconSymbol = "i";
        iconBg = AppStyle::InfoLight;
        iconColor = AppStyle::Info;
        break;
    }

    iconLabel->setText(iconSymbol);
    iconLabel->setStyleSheet(QString(R"(
        background-color: %1;
        color: %2;
        border-radius: 20px;
        font-size: 18px;
        font-weight: 800;
    )").arg(iconBg, iconColor));

    QLabel *titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 17px; font-weight: 700;").arg(AppStyle::TextPrimary));
    titleLabel->setWordWrap(true);

    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(titleLabel, 1);
    cardLayout->addLayout(headerLayout);

    // Message body
    QLabel *messageLabel = new QLabel(message, card);
    messageLabel->setStyleSheet(QString("color: %1; font-size: 13.5px; line-height: 1.4;").arg(AppStyle::TextSecondary));
    messageLabel->setWordWrap(true);
    messageLabel->setMinimumWidth(320);
    messageLabel->setMaximumWidth(420);
    cardLayout->addWidget(messageLabel);

    // Action buttons row
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch();

    if (hasCancel) {
        QPushButton *cancelBtn = new QPushButton(cancelText, card);
        cancelBtn->setMinimumHeight(38);
        cancelBtn->setMinimumWidth(90);
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        btnLayout->addWidget(cancelBtn);
    }

    QPushButton *okBtn = new QPushButton(okText, card);
    okBtn->setMinimumHeight(38);
    okBtn->setMinimumWidth(100);
    okBtn->setCursor(Qt::PointingHandCursor);
    if (type == Error) {
        okBtn->setStyleSheet(AppStyle::dangerButtonStyle());
    } else {
        okBtn->setStyleSheet(AppStyle::primaryButtonStyle());
    }
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(okBtn);

    cardLayout->addLayout(btnLayout);
    rootLayout->addWidget(card);

    adjustSize();
}

void StyledMessageBox::information(QWidget *parent, const QString &title, const QString &message)
{
    StyledMessageBox box(Info, title, message, parent);
    box.exec();
}

void StyledMessageBox::success(QWidget *parent, const QString &title, const QString &message)
{
    StyledMessageBox box(Success, title, message, parent);
    box.exec();
}

void StyledMessageBox::warning(QWidget *parent, const QString &title, const QString &message)
{
    StyledMessageBox box(Warning, title, message, parent);
    box.exec();
}

void StyledMessageBox::critical(QWidget *parent, const QString &title, const QString &message)
{
    StyledMessageBox box(Error, title, message, parent);
    box.exec();
}

bool StyledMessageBox::question(QWidget *parent,
                                const QString &title,
                                const QString &message,
                                const QString &okText,
                                const QString &cancelText)
{
    StyledMessageBox box(Question, title, message, parent, true, okText, cancelText);
    return box.exec() == QDialog::Accepted;
}

bool StyledMessageBox::confirm(QWidget *parent, const QString &title, const QString &message)
{
    return question(parent, title, message, "Confirm", "Cancel");
}
