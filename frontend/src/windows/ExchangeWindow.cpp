#include "windows/ExchangeWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileInfo>
#include <QPainter>
#include <QUrlQuery>
#include <QNetworkReply>

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
    setupUI();
    refreshExchanges();
}

void ExchangeWindow::setupUI()
{
    setWindowTitle(QStringLiteral("BookBazzar - Book Exchanges"));
    resize(1200, 800);
    setMinimumSize(950, 600);
    setStyleSheet(QStringLiteral("background-color: #F8F9FD; font-family: 'Segoe UI', Arial, sans-serif;"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(createTopBar());

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(QStringLiteral("background: transparent; border: none;"));

    auto *content = new QWidget();
    content->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 24, 40, 40);
    contentLayout->setSpacing(20);

    contentLayout->addWidget(createTabBar());

    exchangesLayout = new QVBoxLayout();
    exchangesLayout->setContentsMargins(0, 0, 0, 0);
    exchangesLayout->setSpacing(16);
    contentLayout->addLayout(exchangesLayout);

    emptyStateWidget = new QWidget();
    auto *emptyL = new QVBoxLayout(emptyStateWidget);
    emptyL->setContentsMargins(20, 60, 20, 60);
    emptyL->setAlignment(Qt::AlignCenter);
    auto *emptyIcon = new QLabel(QStringLiteral("⇄"), emptyStateWidget);
    emptyIcon->setStyleSheet(QStringLiteral("font-size: 54px; color: #94A3B8;"));
    emptyIcon->setAlignment(Qt::AlignCenter);
    emptyL->addWidget(emptyIcon);
    auto *emptyLbl = new QLabel(QStringLiteral("No exchange proposals found in this section."), emptyStateWidget);
    emptyLbl->setStyleSheet(QStringLiteral("color: #64748B; font-size: 15px; font-weight: 600;"));
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
    bar->setFixedHeight(74);
    bar->setStyleSheet(QStringLiteral("QFrame { background-color: white; border-bottom: 1px solid #E2E8F0; }"));

    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(36, 12, 36, 12);
    layout->setSpacing(16);

    auto *backButton = new QPushButton(QStringLiteral("← Back"), bar);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setFixedSize(90, 40);
    backButton->setStyleSheet(R"(
        QPushButton {
            background-color: #F1F5F9;
            color: #334155;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 700;
        }
        QPushButton:hover {
            background-color: #E2E8F0;
            color: #0F172A;
        }
    )");
    connect(backButton, &QPushButton::clicked, this, &ExchangeWindow::backRequested);
    layout->addWidget(backButton);

    auto *title = new QLabel(QStringLiteral("Peer-to-Peer Book Exchanges"), bar);
    title->setStyleSheet(QStringLiteral("QLabel { color: #0F172A; font-size: 22px; font-weight: 800; }"));
    layout->addWidget(title);

    layout->addStretch();

    auto *refreshBtn = new QPushButton(QStringLiteral("↻ Refresh"), bar);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: #4F46E5; border: 1px solid #C7D2FE; padding: 6px 14px; border-radius: 8px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background: #EEF2FF; }"
    ));
    connect(refreshBtn, &QPushButton::clicked, this, &ExchangeWindow::refreshExchanges);
    layout->addWidget(refreshBtn);

    return bar;
}

QWidget* ExchangeWindow::createTabBar()
{
    auto *container = new QFrame();
    container->setStyleSheet(QStringLiteral("background: transparent;"));

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto styleActive = QStringLiteral("QPushButton { background: #4F46E5; color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: 700; padding: 0 20px; }");
    auto styleInactive = QStringLiteral("QPushButton { background: white; color: #475569; border: 1px solid #CBD5E1; border-radius: 8px; font-size: 14px; font-weight: 700; padding: 0 20px; } QPushButton:hover { background: #F1F5F9; color: #0F172A; }");

    receivedTabBtn = new QPushButton(QStringLiteral("Received Proposals"), container);
    receivedTabBtn->setCursor(Qt::PointingHandCursor);
    receivedTabBtn->setFixedHeight(42);
    receivedTabBtn->setStyleSheet(styleActive);
    connect(receivedTabBtn, &QPushButton::clicked, this, [this]() { handleTabChange(0); });
    layout->addWidget(receivedTabBtn);

    sentTabBtn = new QPushButton(QStringLiteral("Sent Proposals"), container);
    sentTabBtn->setCursor(Qt::PointingHandCursor);
    sentTabBtn->setFixedHeight(42);
    sentTabBtn->setStyleSheet(styleInactive);
    connect(sentTabBtn, &QPushButton::clicked, this, [this]() { handleTabChange(1); });
    layout->addWidget(sentTabBtn);

    historyTabBtn = new QPushButton(QStringLiteral("Exchange History"), container);
    historyTabBtn->setCursor(Qt::PointingHandCursor);
    historyTabBtn->setFixedHeight(42);
    historyTabBtn->setStyleSheet(styleInactive);
    connect(historyTabBtn, &QPushButton::clicked, this, [this]() { handleTabChange(2); });
    layout->addWidget(historyTabBtn);

    layout->addStretch();
    return container;
}

QWidget* ExchangeWindow::createPaginationBar()
{
    auto *container = new QFrame();
    container->setStyleSheet(QStringLiteral("background: transparent;"));

    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 10, 0, 10);
    layout->setSpacing(12);

    prevPageBtn = new QPushButton(QStringLiteral("← Previous"), container);
    prevPageBtn->setCursor(Qt::PointingHandCursor);
    prevPageBtn->setFixedSize(100, 36);
    prevPageBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: white; border: 1px solid #CBD5E1; border-radius: 6px; font-weight: 600; color: #334155; }"
        "QPushButton:hover { background: #F1F5F9; }"
        "QPushButton:disabled { color: #94A3B8; border-color: #E2E8F0; }"
    ));
    connect(prevPageBtn, &QPushButton::clicked, this, &ExchangeWindow::handlePrevPage);
    layout->addWidget(prevPageBtn);

    pageIndicatorLabel = new QLabel(QStringLiteral("Page 1 of 1"), container);
    pageIndicatorLabel->setStyleSheet(QStringLiteral("font-weight: 600; color: #475569; font-size: 13px;"));
    layout->addWidget(pageIndicatorLabel);

    nextPageBtn = new QPushButton(QStringLiteral("Next →"), container);
    nextPageBtn->setCursor(Qt::PointingHandCursor);
    nextPageBtn->setFixedSize(100, 36);
    nextPageBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: white; border: 1px solid #CBD5E1; border-radius: 6px; font-weight: 600; color: #334155; }"
        "QPushButton:hover { background: #F1F5F9; }"
        "QPushButton:disabled { color: #94A3B8; border-color: #E2E8F0; }"
    ));
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

    QLinearGradient grad(0, 0, w, h);
    grad.setColorAt(0.0, QColor(0x4F, 0x46, 0xE5));
    grad.setColorAt(1.0, QColor(0x7C, 0x3A, 0xED));
    painter.fillRect(0, 0, w, h, grad);

    painter.setPen(Qt::white);
    QFont f = painter.font();
    f.setBold(true);
    f.setPointSize(9);
    painter.setFont(f);
    QRect titleRect(3, 6, w - 6, h / 2);
    painter.drawText(titleRect, Qt::AlignCenter | Qt::TextWordWrap, title.isEmpty() ? QStringLiteral("Book") : title);
    painter.end();
    return pixmap;
}

QWidget* ExchangeWindow::createBookSnapshotWidget(const QString &heading, const ExchangeBookSnapshot &book)
{
    auto *frame = new QFrame();
    frame->setStyleSheet(QStringLiteral("QFrame { background: #F8FAFC; border: 1px solid #E2E8F0; border-radius: 10px; padding: 12px; }"));

    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(8);

    auto *headLbl = new QLabel(heading, frame);
    headLbl->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 800; color: #64748B; text-transform: uppercase;"));
    layout->addWidget(headLbl);

    auto *contentRow = new QHBoxLayout();
    contentRow->setSpacing(12);

    auto *cover = new QLabel(frame);
    cover->setFixedSize(50, 68);
    cover->setPixmap(loadOrGenerateCover(book.coverImage, book.title, book.author, 50, 68));
    cover->setStyleSheet(QStringLiteral("border-radius: 4px;"));
    contentRow->addWidget(cover);

    auto *info = new QVBoxLayout();
    info->setSpacing(2);

    auto *tLbl = new QLabel(book.title.isEmpty() ? QStringLiteral("Untitled") : book.title, frame);
    tLbl->setStyleSheet(QStringLiteral("font-size: 14px; font-weight: 700; color: #0F172A;"));
    info->addWidget(tLbl);

    auto *aLbl = new QLabel(QStringLiteral("by ") + (book.author.isEmpty() ? QStringLiteral("Unknown") : book.author), frame);
    aLbl->setStyleSheet(QStringLiteral("font-size: 12px; color: #64748B;"));
    info->addWidget(aLbl);

    auto *condLbl = new QLabel(QString("Condition: %1 • Owner: %2").arg(book.condition).arg(book.originalOwnerUsername), frame);
    condLbl->setStyleSheet(QStringLiteral("font-size: 11px; color: #475569; font-weight: 600;"));
    info->addWidget(condLbl);

    contentRow->addLayout(info, 1);
    layout->addLayout(contentRow);

    return frame;
}

QFrame* ExchangeWindow::createExchangeCard(const ExchangeModel &exchange)
{
    auto *card = new QFrame();
    card->setObjectName(QStringLiteral("exchangeCard"));
    card->setStyleSheet(QStringLiteral(
        "QFrame#exchangeCard { background-color: white; border: 1px solid #E2E8F0; border-radius: 14px; padding: 18px; }"
    ));

    auto *mainL = new QVBoxLayout(card);
    mainL->setContentsMargins(18, 16, 18, 16);
    mainL->setSpacing(14);

    // Top Header: Exchange Number, Date, Status badge
    auto *topRow = new QHBoxLayout();
    auto *numLbl = new QLabel(QString("Exchange #%1").arg(exchange.exchangeNumber), card);
    numLbl->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 800; color: #0F172A;"));
    topRow->addWidget(numLbl);

    auto *dateLbl = new QLabel(exchange.createdAt.toString(QStringLiteral("dd MMM yyyy, hh:mm AP")), card);
    dateLbl->setStyleSheet(QStringLiteral("font-size: 12px; color: #64748B; margin-left: 8px;"));
    topRow->addWidget(dateLbl);

    topRow->addStretch();

    auto *badge = new QLabel(exchange.status, card);
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedHeight(28);

    if (exchange.status == QStringLiteral("COMPLETED")) {
        badge->setStyleSheet(QStringLiteral("background: #D1FAE5; color: #065F46; font-size: 11px; font-weight: 800; border-radius: 6px; padding: 0 12px;"));
    } else if (exchange.status == QStringLiteral("REJECTED") || exchange.status == QStringLiteral("CANCELLED")) {
        badge->setStyleSheet(QStringLiteral("background: #FEE2E2; color: #991B1B; font-size: 11px; font-weight: 800; border-radius: 6px; padding: 0 12px;"));
    } else {
        badge->setStyleSheet(QStringLiteral("background: #FEF3C7; color: #92400E; font-size: 11px; font-weight: 800; border-radius: 6px; padding: 0 12px;"));
    }
    topRow->addWidget(badge);
    mainL->addLayout(topRow);

    // Swap Visual Row: Requested Book <-> Swap Icon <-> Offered Book
    auto *swapRow = new QHBoxLayout();
    swapRow->setSpacing(12);

    swapRow->addWidget(createBookSnapshotWidget(QStringLiteral("Book Requested"), exchange.requestedBook), 1);

    auto *swapIconBox = new QVBoxLayout();
    swapIconBox->setAlignment(Qt::AlignCenter);
    auto *swapIcon = new QLabel(QStringLiteral("⇄"), card);
    swapIcon->setStyleSheet(QStringLiteral("font-size: 26px; font-weight: 900; color: #4F46E5; background: #EEF2FF; border-radius: 20px; padding: 8px;"));
    swapIcon->setAlignment(Qt::AlignCenter);
    swapIconBox->addWidget(swapIcon);
    swapRow->addLayout(swapIconBox);

    swapRow->addWidget(createBookSnapshotWidget(QStringLiteral("Book Offered in Exchange"), exchange.offeredBook), 1);

    mainL->addLayout(swapRow);

    // Message box
    if (!exchange.message.isEmpty()) {
        auto *msgLbl = new QLabel(QString("💬 Note: \"%1\"").arg(exchange.message), card);
        msgLbl->setStyleSheet(QStringLiteral("font-size: 12px; color: #475569; font-style: italic; background: #F1F5F9; padding: 8px 12px; border-radius: 6px;"));
        mainL->addWidget(msgLbl);
    }

    // Status note
    if (!exchange.statusNote.isEmpty()) {
        auto *noteLbl = new QLabel(QString("ℹ️ %1").arg(exchange.statusNote), card);
        noteLbl->setStyleSheet(QStringLiteral("font-size: 12px; color: #64748B; background: #F8FAFC; padding: 6px 10px; border-radius: 6px;"));
        mainL->addWidget(noteLbl);
    }

    // Action buttons
    auto *actionsRow = new QHBoxLayout();
    actionsRow->addStretch();

    if (currentTab == 0 && exchange.status == QStringLiteral("PENDING")) {
        // Received proposals: Accept or Reject
        auto *acceptBtn = new QPushButton(QStringLiteral("✓ Accept Exchange"), card);
        acceptBtn->setCursor(Qt::PointingHandCursor);
        acceptBtn->setStyleSheet(QStringLiteral("QPushButton { background: #10B981; color: white; border: none; font-weight: 700; font-size: 12px; padding: 8px 16px; border-radius: 6px; } QPushButton:hover { background: #059669; }"));
        connect(acceptBtn, &QPushButton::clicked, this, [this, id = exchange.id]() { handleAccept(id); });
        actionsRow->addWidget(acceptBtn);

        auto *rejectBtn = new QPushButton(QStringLiteral("✕ Decline"), card);
        rejectBtn->setCursor(Qt::PointingHandCursor);
        rejectBtn->setStyleSheet(QStringLiteral("QPushButton { background: #EF4444; color: white; border: none; font-weight: 700; font-size: 12px; padding: 8px 16px; border-radius: 6px; } QPushButton:hover { background: #DC2626; }"));
        connect(rejectBtn, &QPushButton::clicked, this, [this, id = exchange.id]() { handleReject(id); });
        actionsRow->addWidget(rejectBtn);
    } else if (currentTab == 1 && exchange.status == QStringLiteral("PENDING")) {
        // Sent proposals: Cancel
        auto *cancelBtn = new QPushButton(QStringLiteral("Cancel Proposal"), card);
        cancelBtn->setCursor(Qt::PointingHandCursor);
        cancelBtn->setStyleSheet(QStringLiteral("QPushButton { background: #F1F5F9; color: #EF4444; border: 1px solid #FCA5A5; font-weight: 700; font-size: 12px; padding: 8px 16px; border-radius: 6px; } QPushButton:hover { background: #FEE2E2; }"));
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
    query.addQueryItem(QStringLiteral("page"), QString::number(currentPage));
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("10"));

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
            pageIndicatorLabel->setText(QString("Page %1 of %2 (Total %3)")
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

void ExchangeWindow::handleTabChange(int tabIndex)
{
    if (currentTab == tabIndex) return;
    currentTab = tabIndex;
    currentPage = 1;

    auto styleActive = QStringLiteral("QPushButton { background: #4F46E5; color: white; border: none; border-radius: 8px; font-size: 14px; font-weight: 700; padding: 0 20px; }");
    auto styleInactive = QStringLiteral("QPushButton { background: white; color: #475569; border: 1px solid #CBD5E1; border-radius: 8px; font-size: 14px; font-weight: 700; padding: 0 20px; } QPushButton:hover { background: #F1F5F9; color: #0F172A; }");

    receivedTabBtn->setStyleSheet(currentTab == 0 ? styleActive : styleInactive);
    sentTabBtn->setStyleSheet(currentTab == 1 ? styleActive : styleInactive);
    historyTabBtn->setStyleSheet(currentTab == 2 ? styleActive : styleInactive);

    refreshExchanges();
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

void ExchangeWindow::handleAccept(const QString &exchangeId)
{
    auto res = QMessageBox::question(this, QStringLiteral("Accept Exchange"),
                                     QStringLiteral("Are you sure you want to accept this exchange? Both books will be atomically marked as exchanged."),
                                     QMessageBox::Yes | QMessageBox::No);
    if (res != QMessageBox::Yes) return;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/exchanges/") + exchangeId + QStringLiteral("/accept"), QJsonObject(), true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            QMessageBox::information(this, QStringLiteral("Exchange Accepted!"),
                                     QStringLiteral("Congratulations! The book exchange has been completed successfully."));
            refreshExchanges();
        } else {
            QMessageBox::warning(this, QStringLiteral("Error"), errorMsg.isEmpty() ? QStringLiteral("Unable to accept exchange.") : errorMsg);
        }
    });
}

void ExchangeWindow::handleReject(const QString &exchangeId)
{
    bool ok = false;
    QString note = QInputDialog::getText(this, QStringLiteral("Decline Proposal"),
                                        QStringLiteral("Optional reason for declining:"),
                                        QLineEdit::Normal, QString(), &ok);
    if (!ok) return;

    QJsonObject payload;
    if (!note.trimmed().isEmpty()) {
        payload[QStringLiteral("note")] = note.trimmed();
    }

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/exchanges/") + exchangeId + QStringLiteral("/reject"), payload, true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            QMessageBox::information(this, QStringLiteral("Proposal Declined"), QStringLiteral("The exchange proposal has been declined."));
            refreshExchanges();
        } else {
            QMessageBox::warning(this, QStringLiteral("Error"), errorMsg.isEmpty() ? QStringLiteral("Unable to decline exchange.") : errorMsg);
        }
    });
}

void ExchangeWindow::handleCancel(const QString &exchangeId)
{
    auto res = QMessageBox::question(this, QStringLiteral("Cancel Proposal"),
                                     QStringLiteral("Are you sure you want to cancel this exchange proposal?"),
                                     QMessageBox::Yes | QMessageBox::No);
    if (res != QMessageBox::Yes) return;

    auto *reply = ApiClient::instance().post(QStringLiteral("/api/exchanges/") + exchangeId + QStringLiteral("/cancel"), QJsonObject(), true);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (ok) {
            QMessageBox::information(this, QStringLiteral("Proposal Cancelled"), QStringLiteral("Your exchange proposal has been cancelled."));
            refreshExchanges();
        } else {
            QMessageBox::warning(this, QStringLiteral("Error"), errorMsg.isEmpty() ? QStringLiteral("Unable to cancel exchange proposal.") : errorMsg);
        }
    });
}
