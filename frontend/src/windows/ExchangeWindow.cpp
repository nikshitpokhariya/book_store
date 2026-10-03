#include "windows/ExchangeWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include "AppStyle.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include "StyledMessageBox.h"
#include <QInputDialog>
#include <QFileInfo>
#include <QPainter>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonArray>
#include <QJsonObject>

ExchangeWindow::ExchangeWindow(QWidget *parent)
    : QWidget(parent),
    receivedTabBtn(nullptr),
    sentTabBtn(nullptr),
    historyTabBtn(nullptr),
    exchangesLayout(nullptr),
    emptyStateWidget(nullptr),
    prevPageBtn(nullptr),
    nextPageBtn(nullptr),
    pageIndicatorLabel(nullptr)
{
    setWindowTitle("BookBazzar - Book Exchanges");
    resize(1240, 820);
    setMinimumSize(980, 640);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    setupUI();
    refreshExchanges();
}

void ExchangeWindow::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(createTopBar());

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QString(R"(
        QScrollArea {
            background-color: %1;
            border: none;
        }
        %2
    )").arg(AppStyle::Background, AppStyle::scrollBarStyle()));

    auto *content = new QWidget();
    content->setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 28, 40, 48);
    contentLayout->setSpacing(24);

    contentLayout->addWidget(createTabBar());

    exchangesLayout = new QVBoxLayout();
    exchangesLayout->setContentsMargins(0, 0, 0, 0);
    exchangesLayout->setSpacing(16);
    contentLayout->addLayout(exchangesLayout);

    emptyStateWidget = new QWidget();
    auto *emptyL = new QVBoxLayout(emptyStateWidget);
    emptyL->setContentsMargins(20, 60, 20, 60);
    emptyL->setAlignment(Qt::AlignCenter);
    emptyL->setSpacing(12);

    auto *emptyIcon = new QLabel("⇄", emptyStateWidget);
    emptyIcon->setStyleSheet(QString("font-size: 54px; color: %1;").arg(AppStyle::TextMuted));
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyL->addWidget(emptyIcon);

    auto *emptyLbl = new QLabel("No exchange proposals found in this section.", emptyStateWidget);
    emptyLbl->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: 600;").arg(AppStyle::TextSecondary));
    emptyLbl->setAlignment(Qt::AlignCenter);
    emptyL->addWidget(emptyLbl);

    emptyStateWidget->setVisible(false);
    contentLayout->addWidget(emptyStateWidget);

    contentLayout->addWidget(createPaginationBar());
    contentLayout->addStretch();

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea, 1);
}

QWidget* ExchangeWindow::createTopBar()
{
    auto *bar = new QFrame(this);
    bar->setFixedHeight(72);
    bar->setStyleSheet(R"(
        .QFrame {
            background-color: #FFFFFF;
            border-bottom: 1px solid #E2E8F0;
        }
        QLabel {
            border: none;
            background: transparent;
        }
    )");
    AppStyle::applyElevation(bar, 16, 2, 15);

    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(36, 10, 36, 10);
    layout->setSpacing(16);

    auto *backButton = new QPushButton("← Back", bar);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setFixedSize(88, 38);
    backButton->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(backButton, &QPushButton::clicked, this, &ExchangeWindow::backRequested);
    layout->addWidget(backButton);

    auto *title = new QLabel("Peer-to-Peer Book Exchanges", bar);
    title->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: 800;").arg(AppStyle::TextPrimary));
    layout->addWidget(title);

    layout->addStretch();

    auto *refreshBtn = new QPushButton("↻ Refresh", bar);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setFixedHeight(38);
    refreshBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(refreshBtn, &QPushButton::clicked, this, &ExchangeWindow::refreshExchanges);
    layout->addWidget(refreshBtn);

    return bar;
}

QWidget* ExchangeWindow::createTabBar()
{
    auto *container = new QFrame();
    container->setStyleSheet("background: transparent;");

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    receivedTabBtn = new QPushButton("Received Proposals", container);
    receivedTabBtn->setCursor(Qt::PointingHandCursor);
    receivedTabBtn->setStyleSheet(AppStyle::activeTabStyle());

    sentTabBtn = new QPushButton("Sent Proposals", container);
    sentTabBtn->setCursor(Qt::PointingHandCursor);
    sentTabBtn->setStyleSheet(AppStyle::inactiveTabStyle());

    historyTabBtn = new QPushButton("Exchange History", container);
    historyTabBtn->setCursor(Qt::PointingHandCursor);
    historyTabBtn->setStyleSheet(AppStyle::inactiveTabStyle());

    layout->addWidget(receivedTabBtn);
    layout->addWidget(sentTabBtn);
    layout->addWidget(historyTabBtn);
    layout->addStretch();

    connect(receivedTabBtn, &QPushButton::clicked, this, [this]() { handleTabChange(0); });
    connect(sentTabBtn, &QPushButton::clicked, this, [this]() { handleTabChange(1); });
    connect(historyTabBtn, &QPushButton::clicked, this, [this]() { handleTabChange(2); });

    return container;
}

void ExchangeWindow::handleTabChange(int tabIndex)
{
    if (currentTab == tabIndex) return;
    currentTab = tabIndex;
    currentPage = 1;

    receivedTabBtn->setStyleSheet(currentTab == 0 ? AppStyle::activeTabStyle() : AppStyle::inactiveTabStyle());
    sentTabBtn->setStyleSheet(currentTab == 1 ? AppStyle::activeTabStyle() : AppStyle::inactiveTabStyle());
    historyTabBtn->setStyleSheet(currentTab == 2 ? AppStyle::activeTabStyle() : AppStyle::inactiveTabStyle());

    refreshExchanges();
}

QWidget* ExchangeWindow::createPaginationBar()
{
    auto *container = new QFrame();
    container->setStyleSheet("background: transparent;");

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 10, 0, 10);
    layout->setSpacing(12);

    prevPageBtn = new QPushButton("← Previous", container);
    prevPageBtn->setCursor(Qt::PointingHandCursor);
    prevPageBtn->setFixedSize(100, 36);
    prevPageBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(prevPageBtn, &QPushButton::clicked, this, &ExchangeWindow::handlePrevPage);
    layout->addWidget(prevPageBtn);

    pageIndicatorLabel = new QLabel("Page 1 of 1", container);
    pageIndicatorLabel->setStyleSheet(QString("font-weight: 600; color: %1; font-size: 13px;").arg(AppStyle::TextSecondary));
    layout->addWidget(pageIndicatorLabel);

    nextPageBtn = new QPushButton("Next →", container);
    nextPageBtn->setCursor(Qt::PointingHandCursor);
    nextPageBtn->setFixedSize(100, 36);
    nextPageBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
    connect(nextPageBtn, &QPushButton::clicked, this, &ExchangeWindow::handleNextPage);
    layout->addWidget(nextPageBtn);

    layout->addStretch();
    return container;
}

QPixmap ExchangeWindow::loadOrGenerateCover(const QString &imagePath, const QString &title, const QString &author, int w, int h)
{
    if (!imagePath.trimmed().isEmpty()) {
        QFileInfo fi(imagePath);
        if (fi.exists() && fi.isFile()) {
            QPixmap pix(imagePath);
            if (!pix.isNull()) {
                return pix.scaled(w, h, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            }
        }
    }

    QPixmap pixmap(w, h);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(0, 0, w, h, QColor("#1E293B"));
    painter.setPen(Qt::white);
    QFont f(AppStyle::appFont(), 8, QFont::Bold);
    painter.setFont(f);
    QRect titleRect(3, 6, w - 6, h / 2);
    painter.drawText(titleRect, Qt::AlignCenter | Qt::TextWordWrap, title.isEmpty() ? "Book" : title);
    painter.end();
    return pixmap;
}

QWidget* ExchangeWindow::createBookSnapshotWidget(const QString &heading, const ExchangeBookSnapshot &book)
{
    auto *frame = new QFrame();
    frame->setStyleSheet(QString(R"(
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

    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(8);

    auto *headLbl = new QLabel(heading.toUpper(), frame);
    headLbl->setStyleSheet(QString("font-size: 10px; font-weight: 800; color: %1; letter-spacing: 0.5px;").arg(AppStyle::TextMuted));
    layout->addWidget(headLbl);

    auto *contentRow = new QHBoxLayout();
    contentRow->setSpacing(12);

    auto *cover = new QLabel(frame);
    cover->setFixedSize(50, 68);
    cover->setPixmap(loadOrGenerateCover(book.coverImage, book.title, book.author, 50, 68));
    cover->setStyleSheet("border-radius: 4px;");
    contentRow->addWidget(cover);

    auto *info = new QVBoxLayout();
    info->setSpacing(2);

    auto *tLbl = new QLabel(book.title.isEmpty() ? "Untitled Book" : book.title, frame);
    tLbl->setStyleSheet(QString("font-size: 14px; font-weight: 700; color: %1;").arg(AppStyle::TextPrimary));
    info->addWidget(tLbl);

    auto *aLbl = new QLabel("by " + (book.author.isEmpty() ? "Unknown" : book.author), frame);
    aLbl->setStyleSheet(QString("font-size: 12px; color: %1;").arg(AppStyle::TextSecondary));
    info->addWidget(aLbl);

    auto *condLbl = new QLabel(QString("Condition: %1 • Owner: %2").arg(book.condition).arg(book.originalOwnerUsername), frame);
    condLbl->setStyleSheet(QString("font-size: 11px; color: %1; font-weight: 600;").arg(AppStyle::TextMuted));
    info->addWidget(condLbl);

    contentRow->addLayout(info, 1);
    layout->addLayout(contentRow);

    return frame;
}

QFrame* ExchangeWindow::createExchangeCard(const ExchangeModel &exchange)
{
    auto *card = new QFrame();
    card->setObjectName("exchangeCard");
    card->setStyleSheet(AppStyle::cardStyle());
    AppStyle::applyElevation(card, 16, 4, 12);

    auto *mainL = new QVBoxLayout(card);
    mainL->setContentsMargins(20, 18, 20, 18);
    mainL->setSpacing(14);

    auto *topRow = new QHBoxLayout();
    auto *numLbl = new QLabel(QString("Exchange #%1").arg(exchange.exchangeNumber), card);
    numLbl->setStyleSheet(QString("font-size: 15px; font-weight: 800; color: %1;").arg(AppStyle::TextPrimary));
    topRow->addWidget(numLbl);

    auto *dateLbl = new QLabel(exchange.createdAt.toString("dd MMM yyyy, hh:mm AP"), card);
    dateLbl->setStyleSheet(QString("font-size: 12px; color: %1; margin-left: 8px;").arg(AppStyle::TextMuted));
    topRow->addWidget(dateLbl);

    topRow->addStretch();

    auto *badge = new QLabel(exchange.status, card);
    badge->setAlignment(Qt::AlignCenter);
    badge->setStyleSheet(AppStyle::statusBadgeStyle(exchange.status));
    topRow->addWidget(badge);
    mainL->addLayout(topRow);

    // Swap Visual Row: Requested Book <-> Swap Icon <-> Offered Book
    auto *swapRow = new QHBoxLayout();
    swapRow->setSpacing(14);

    swapRow->addWidget(createBookSnapshotWidget("Book Requested", exchange.requestedBook), 1);

    auto *swapIconBox = new QVBoxLayout();
    swapIconBox->setAlignment(Qt::AlignCenter);
    auto *swapIcon = new QLabel("⇄", card);
    swapIcon->setStyleSheet(QString("font-size: 24px; font-weight: 900; color: %1; background-color: %2; border-radius: 20px; padding: 6px 12px;").arg(AppStyle::Primary, AppStyle::PrimaryLight));
    swapIcon->setAlignment(Qt::AlignCenter);
    swapIconBox->addWidget(swapIcon);
    swapRow->addLayout(swapIconBox);

    swapRow->addWidget(createBookSnapshotWidget("Book Offered in Exchange", exchange.offeredBook), 1);

    mainL->addLayout(swapRow);

    if (!exchange.message.isEmpty()) {
        auto *msgLbl = new QLabel(QString("💬 Proposal Note: \"%1\"").arg(exchange.message), card);
        msgLbl->setStyleSheet(QString("font-size: 12px; color: %1; font-style: italic; background-color: %2; padding: 8px 12px; border-radius: 6px;").arg(AppStyle::TextSecondary, AppStyle::SurfaceSubtle));
        mainL->addWidget(msgLbl);
    }

    if (!exchange.statusNote.isEmpty()) {
        auto *noteLbl = new QLabel(QString("ℹ️ %1").arg(exchange.statusNote), card);
        noteLbl->setStyleSheet(QString("font-size: 12px; color: %1; background-color: %2; padding: 6px 10px; border-radius: 6px;").arg(AppStyle::TextMuted, AppStyle::SurfaceSubtle));
        mainL->addWidget(noteLbl);
    }

    auto *actionsRow = new QHBoxLayout();
    actionsRow->addStretch();

    if (currentTab == 0 && exchange.status == "PENDING") {
        // Received proposals: Accept or Decline
        auto *acceptBtn = new QPushButton("✓ Accept Exchange", card);
        acceptBtn->setCursor(Qt::PointingHandCursor);
        acceptBtn->setStyleSheet(AppStyle::primaryButtonStyle());
        connect(acceptBtn, &QPushButton::clicked, this, [this, id = exchange.id]() { handleAccept(id); });
        actionsRow->addWidget(acceptBtn);

        auto *rejectBtn = new QPushButton("✕ Decline", card);
        rejectBtn->setCursor(Qt::PointingHandCursor);
        rejectBtn->setStyleSheet(AppStyle::dangerButtonStyle());
        connect(rejectBtn, &QPushButton::clicked, this, [this, id = exchange.id]() { handleReject(id); });
        actionsRow->addWidget(rejectBtn);
    } else if (currentTab == 1 && exchange.status == "PENDING") {
        // Sent proposals: Cancel
        auto *cancelBtn = new QPushButton("Cancel Proposal", card);
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setStyleSheet(AppStyle::secondaryButtonStyle());
        connect(cancelBtn, &QPushButton::clicked, this, [this, id = exchange.id]() { handleCancel(id); });
        actionsRow->addWidget(cancelBtn);
    }

    mainL->addLayout(actionsRow);
    return card;
}

void ExchangeWindow::refreshExchanges()
{
    if (!exchangesLayout) return;

    QUrlQuery query;
    query.addQueryItem("page", QString::number(currentPage));
    query.addQueryItem("limit", "10");

    QString endpoint;
    if (currentTab == 0) endpoint = QStringLiteral("/api/exchanges/received");
    else if (currentTab == 1) endpoint = QStringLiteral("/api/exchanges/sent");
    else endpoint = QStringLiteral("/api/exchanges/history");

    auto *reply = ApiClient::instance().get(endpoint, query);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        QLayoutItem *item;
        while ((item = exchangesLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        if (!ok || !json.contains(QStringLiteral("exchanges"))) {
            emptyStateWidget->setVisible(true);
            return;
        }

        QJsonArray arr = json[QStringLiteral("exchanges")].toArray();
        QVector<ExchangeModel> exchanges;
        for (const auto &val : arr) {
            exchanges.append(ExchangeModel::fromJson(val.toObject()));
        }

        if (json.contains(QStringLiteral("pagination"))) {
            pagination = PaginationMeta::fromJson(json[QStringLiteral("pagination")].toObject());
            pageIndicatorLabel->setText(QString("Page %1 of %2 (%3 proposals)")
                                            .arg(pagination.page)
                                            .arg(qMax(1, pagination.totalPages))
                                            .arg(pagination.total));
            prevPageBtn->setEnabled(pagination.hasPrevPage);
            nextPageBtn->setEnabled(pagination.hasNextPage);
        }

        if (exchanges.isEmpty()) {
            emptyStateWidget->setVisible(true);
        } else {
            emptyStateWidget->setVisible(false);
            for (const auto &ex : exchanges) {
                exchangesLayout->addWidget(createExchangeCard(ex));
            }
        }
    });
}

void ExchangeWindow::handleAccept(const QString &exchangeId)
{
    bool confirmed = StyledMessageBox::question(
        this, "Accept Exchange",
        "Accepting this exchange will mark your book as EXCHANGED and transfer ownership. Proceed?",
        "Yes, Accept", "Cancel"
    );
    if (!confirmed) return;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/exchanges/") + exchangeId + QStringLiteral("/accept"), {}, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            StyledMessageBox::success(this, "Exchange Accepted!", "Exchange completed successfully! Both book records have been updated.");
            refreshExchanges();
        } else {
            StyledMessageBox::warning(this, "Accept Failed", errorMsg.isEmpty() ? "Unable to accept exchange." : errorMsg);
        }
    });
}

void ExchangeWindow::handleReject(const QString &exchangeId)
{
    bool ok = false;
    QString reason = QInputDialog::getText(
        this, "Decline Exchange",
        "Reason for declining (optional):",
        QLineEdit::Normal, QString(), &ok
    );
    if (!ok) return;

    QJsonObject payload;
    if (!reason.trimmed().isEmpty()) {
        payload[QStringLiteral("reason")] = reason.trimmed();
    }

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/exchanges/") + exchangeId + QStringLiteral("/reject"), payload, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            StyledMessageBox::information(this, "Exchange Declined", "You have declined this exchange proposal.");
            refreshExchanges();
        } else {
            StyledMessageBox::warning(this, "Action Failed", errorMsg.isEmpty() ? "Unable to decline exchange." : errorMsg);
        }
    });
}

void ExchangeWindow::handleCancel(const QString &exchangeId)
{
    bool confirmed = StyledMessageBox::question(
        this, "Cancel Proposal",
        "Are you sure you want to cancel your exchange proposal?",
        "Yes, Cancel", "Keep Proposal"
    );
    if (!confirmed) return;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/exchanges/") + exchangeId + QStringLiteral("/cancel"), {}, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            StyledMessageBox::information(this, "Proposal Cancelled", "Your exchange proposal has been cancelled.");
            refreshExchanges();
        } else {
            StyledMessageBox::warning(this, "Cancel Failed", errorMsg.isEmpty() ? "Unable to cancel exchange proposal." : errorMsg);
        }
    });
}

void ExchangeWindow::handleNextPage()
{
    if (pagination.hasNextPage) {
        currentPage++;
        refreshExchanges();
    }
}

void ExchangeWindow::handlePrevPage()
{
    if (pagination.hasPrevPage && currentPage > 1) {
        currentPage--;
        refreshExchanges();
    }
}
