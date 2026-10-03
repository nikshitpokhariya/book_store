#include "homewindow.h"
#include "bookdetailswindow.h"
#include "browsewindow.h"
#include "sellwindow.h"
#include "listingswindow.h"
#include "orderswindow.h"
#include "windows/CartWindow.h"
#include "windows/CheckoutWindow.h"
#include "windows/OrderDetailWindow.h"
#include "windows/ExchangeWindow.h"
#include "ImageLoader.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include "models/DataModels.h"
#include "AppStyle.h"

#include <QUrlQuery>
#include <QNetworkReply>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QStackedWidget>
#include "StyledMessageBox.h"
#include <QSizePolicy>
#include <QPixmap>
#include <QFileInfo>
#include <QFont>
#include <QStringList>
#include <QDialog>
#include <QPainter>
#include <QLinearGradient>
#include <QVector>
#include <QPair>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>

HomeWindow::HomeWindow(const QString &userName, QWidget *parent)
    : QWidget(parent),
    userName(userName),
    searchEdit(nullptr),
    searchButton(nullptr),
    cartButton(nullptr),
    exchangesButton(nullptr),
    logoutButton(nullptr),
    profileButton(nullptr),
    recentBooksLayout(nullptr),
    mainStackedWidget(nullptr),
    homeScrollArea(nullptr)
{
    setWindowTitle("BookBazzar - Home");
    resize(1360, 850);
    setMinimumSize(1020, 680);
    setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

    setupUI();
    refreshCartCount();
}

HomeWindow::~HomeWindow()
{
}

void HomeWindow::setupUI()
{
    mainStackedWidget = new QStackedWidget(this);

    homeScrollArea = new QScrollArea(mainStackedWidget);
    homeScrollArea->setWidgetResizable(true);
    homeScrollArea->setFrameShape(QFrame::NoFrame);
    homeScrollArea->setStyleSheet(QString(R"(
        QScrollArea {
            background-color: %1;
            border: none;
        }
        %2
    )").arg(AppStyle::Background, AppStyle::scrollBarStyle()));

    QWidget *page = new QWidget();
    page->setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));
    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    pageLayout->addWidget(createNavigationBar());

    QWidget *content = new QWidget();
    content->setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(40, 32, 40, 48);
    contentLayout->setSpacing(36);

    contentLayout->addWidget(createHeroSection());
    contentLayout->addWidget(createCategorySection());
    contentLayout->addWidget(createRecentlyListedSection());
    contentLayout->addWidget(createSellSection());
    contentLayout->addWidget(createFooter());

    pageLayout->addWidget(content, 1);
    homeScrollArea->setWidget(page);

    mainStackedWidget->addWidget(homeScrollArea);
    mainStackedWidget->setCurrentWidget(homeScrollArea);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(mainStackedWidget);
}

void HomeWindow::pushPage(QWidget *page)
{
    if (!page || !mainStackedWidget) return;
    mainStackedWidget->addWidget(page);
    mainStackedWidget->setCurrentWidget(page);
}

void HomeWindow::popPage()
{
    if (!mainStackedWidget || mainStackedWidget->count() <= 1) return;
    QWidget *current = mainStackedWidget->currentWidget();
    if (current && current != homeScrollArea) {
        mainStackedWidget->removeWidget(current);
        current->deleteLater();
    }
    QWidget *now = mainStackedWidget->currentWidget();
    if (now == homeScrollArea) {
        refreshRecentBooks();
        refreshCartCount();
    }
}

QWidget* HomeWindow::createNavigationBar()
{
    QFrame *nav = new QFrame();
    nav->setObjectName("navigationBar");
    nav->setFixedHeight(72);
    nav->setStyleSheet(R"(
        QFrame#navigationBar {
            background-color: #FFFFFF;
            border-bottom: 1px solid #E2E8F0;
        }
    )");
    AppStyle::applyElevation(nav, 16, 2, 15);

    QHBoxLayout *layout = new QHBoxLayout(nav);
    layout->setContentsMargins(36, 10, 36, 10);
    layout->setSpacing(8);

    QLabel *logo = new QLabel("BookBazzar");
    logo->setStyleSheet(QString("color: %1; font-size: 22px; font-weight: 800;").arg(AppStyle::TextPrimary));
    layout->addWidget(logo);

    QLabel *dot = new QLabel("•");
    dot->setStyleSheet(QString("color: %1; font-size: 24px; font-weight: 900; margin-right: 16px;").arg(AppStyle::Primary));
    layout->addWidget(dot);

    auto createNavBtn = [](const QString &text) {
        QPushButton *btn = new QPushButton(text);
        btn->setMinimumHeight(38);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(AppStyle::navButtonStyle());
        return btn;
    };

    QPushButton *browseBtn = createNavBtn("Browse Books");
    QPushButton *sellBtn = createNavBtn("Sell Your Book");
    QPushButton *listingBtn = createNavBtn("My Listings");
    QPushButton *ordersBtn = createNavBtn("My Orders");
    exchangesButton = createNavBtn("Exchanges");
    cartButton = createNavBtn("Cart (0)");

    layout->addWidget(browseBtn);
    layout->addWidget(sellBtn);
    layout->addWidget(listingBtn);
    layout->addWidget(ordersBtn);
    layout->addWidget(exchangesButton);
    layout->addWidget(cartButton);
    layout->addStretch();

    const QString displayName = SessionManager::instance().username().isEmpty() ? userName : SessionManager::instance().username();
    profileButton = new QPushButton(QString("👤  %1").arg(displayName));
    profileButton->setCursor(Qt::PointingHandCursor);
    profileButton->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 20px;
            padding: 6px 16px;
            font-size: 13px;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: #E0E7FF;
            color: %4;
        }
    )").arg(AppStyle::PrimaryLight, AppStyle::Primary, AppStyle::PrimaryBorder, AppStyle::PrimaryActive));
    layout->addWidget(profileButton);

    logoutButton = new QPushButton("Logout");
    logoutButton->setCursor(Qt::PointingHandCursor);
    logoutButton->setMinimumSize(82, 36);
    logoutButton->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 8px;
            font-size: 12px;
            font-weight: 600;
            padding: 4px 12px;
        }
        QPushButton:hover {
            background-color: %4;
            color: %5;
            border-color: %6;
        }
    )").arg(AppStyle::SurfaceSubtle, AppStyle::TextSecondary, AppStyle::BorderSubtle,
           AppStyle::DangerLight, AppStyle::DangerText, AppStyle::DangerBorder));
    layout->addWidget(logoutButton);

    connect(browseBtn, &QPushButton::clicked, this, &HomeWindow::handleBrowseBooks);
    connect(sellBtn, &QPushButton::clicked, this, &HomeWindow::handleSellBook);
    connect(listingBtn, &QPushButton::clicked, this, &HomeWindow::handleListings);
    connect(ordersBtn, &QPushButton::clicked, this, &HomeWindow::handleOrders);
    connect(exchangesButton, &QPushButton::clicked, this, &HomeWindow::handleExchanges);
    connect(cartButton, &QPushButton::clicked, this, &HomeWindow::handleCart);
    connect(profileButton, &QPushButton::clicked, this, &HomeWindow::showUserProfile);
    connect(logoutButton, &QPushButton::clicked, this, &HomeWindow::handleLogout);

    return nav;
}

QWidget* HomeWindow::createHeroSection()
{
    QFrame *hero = new QFrame();
    hero->setObjectName("heroSection");
    hero->setStyleSheet(R"(
        QFrame#heroSection {
            background-color: #0F172A;
            border-radius: 20px;
        }
        QLabel {
            background: transparent;
            border: none;
        }
    )");
    AppStyle::applyElevation(hero, 28, 8, 25);

    QVBoxLayout *layout = new QVBoxLayout(hero);
    layout->setContentsMargins(56, 44, 56, 44);
    layout->setSpacing(20);

    QLabel *badge = new QLabel("THE SMARTER WAY TO BUY & SELL BOOKS");
    badge->setStyleSheet("background: transparent; border: none; color: #818CF8; font-size: 11px; font-weight: 800; letter-spacing: 1px;");

    QLabel *heading = new QLabel("Find your next book.\nGive your old ones a new life.");
    heading->setWordWrap(true);
    heading->setStyleSheet("background: transparent; border: none; color: #FFFFFF; font-size: 34px; font-weight: 800; line-height: 1.2;");

    QLabel *desc = new QLabel("Discover affordable textbooks, novels, and classics from readers near you — or list books you no longer need in seconds.");
    desc->setWordWrap(true);
    desc->setMaximumWidth(750);
    desc->setStyleSheet("background: transparent; border: none; color: #94A3B8; font-size: 14px; line-height: 1.5;");

    QHBoxLayout *searchRow = new QHBoxLayout();
    searchRow->setSpacing(8);

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search by book title, author, or ISBN...");
    searchEdit->setMinimumHeight(48);
    searchEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background-color: #FFFFFF;
            color: #0F172A;
            border: 2px solid transparent;
            border-radius: 10px;
            padding: 0 16px;
            font-size: 14px;
        }
        QLineEdit:focus {
            border: 2px solid %1;
        }
    )").arg(AppStyle::Primary));

    searchButton = new QPushButton("Search Books");
    searchButton->setMinimumSize(130, 48);
    searchButton->setCursor(Qt::PointingHandCursor);
    searchButton->setStyleSheet(AppStyle::primaryButtonStyle());

    searchRow->addWidget(searchEdit, 1);
    searchRow->addWidget(searchButton);

    QHBoxLayout *actionsRow = new QHBoxLayout();
    actionsRow->setSpacing(12);

    QPushButton *browseBtn = new QPushButton("Browse All Books");
    browseBtn->setMinimumHeight(42);
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: #FFFFFF;
            border: none;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 20px;
        }
        QPushButton:hover {
            background-color: %2;
        }
    )").arg(AppStyle::Primary, AppStyle::PrimaryHover));

    QPushButton *sellBtn = new QPushButton("+ Sell Your Book");
    sellBtn->setMinimumHeight(42);
    sellBtn->setCursor(Qt::PointingHandCursor);
    sellBtn->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(255, 255, 255, 0.1);
            color: #FFFFFF;
            border: 1px solid rgba(255, 255, 255, 0.2);
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            padding: 8px 20px;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 0.2);
            border-color: rgba(255, 255, 255, 0.4);
        }
    )");

    actionsRow->addWidget(browseBtn);
    actionsRow->addWidget(sellBtn);
    actionsRow->addStretch();

    layout->addWidget(badge);
    layout->addWidget(heading);
    layout->addWidget(desc);
    layout->addSpacing(4);
    layout->addLayout(searchRow);
    layout->addSpacing(6);
    layout->addLayout(actionsRow);

    connect(searchButton, &QPushButton::clicked, this, &HomeWindow::handleSearch);
    connect(searchEdit, &QLineEdit::returnPressed, this, &HomeWindow::handleSearch);
    connect(browseBtn, &QPushButton::clicked, this, &HomeWindow::handleBrowseBooks);
    connect(sellBtn, &QPushButton::clicked, this, &HomeWindow::handleSellBook);

    return hero;
}

QWidget* HomeWindow::createCategorySection()
{
    QWidget *section = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    QHBoxLayout *header = new QHBoxLayout();
    header->addWidget(createSectionTitle("Explore Categories"));
    header->addStretch();
    QLabel *sub = new QLabel("Click any category to filter available listings");
    sub->setStyleSheet(QString("color: %1; font-size: 13px;").arg(AppStyle::TextMuted));
    header->addWidget(sub);
    layout->addLayout(header);

    QHBoxLayout *categoriesRow = new QHBoxLayout();
    categoriesRow->setSpacing(12);

    struct CatInfo {
        QString name;
        QString icon;
    };

    QVector<CatInfo> cats = {
        {"Academic", "📚"},
        {"Programming", "💻"},
        {"Engineering", "⚙️"},
        {"Fiction", "✨"},
        {"Competitive Exams", "🎯"},
        {"School", "🏫"},
        {"Novels", "📖"}
    };

    for (const auto &cat : cats) {
        QPushButton *chip = new QPushButton(QString("%1  %2").arg(cat.icon, cat.name));
        chip->setMinimumHeight(46);
        chip->setCursor(Qt::PointingHandCursor);
        chip->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        chip->setStyleSheet(QString(R"(
            QPushButton {
                background-color: #FFFFFF;
                color: %1;
                border: 1px solid %2;
                border-radius: 12px;
                font-size: 12px;
                font-weight: 600;
                padding: 6px 12px;
            }
            QPushButton:hover {
                border-color: %3;
                background-color: %4;
                color: %5;
            }
            QPushButton:pressed {
                background-color: #E0E7FF;
            }
        )").arg(AppStyle::TextPrimary, AppStyle::BorderSubtle, AppStyle::Primary, AppStyle::PrimaryLight, AppStyle::Primary));

        AppStyle::applyElevation(chip, 12, 3, 10);

        const QString categoryName = cat.name;
        connect(chip, &QPushButton::clicked, this, [this, categoryName]() {
            handleBrowseWithCategory(categoryName);
        });

        categoriesRow->addWidget(chip);
    }

    layout->addLayout(categoriesRow);
    return section;
}

QWidget* HomeWindow::createRecentlyListedSection()
{
    QWidget *section = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    QHBoxLayout *header = new QHBoxLayout();
    header->addWidget(createSectionTitle("Recently Listed Books"));
    header->addStretch();

    QPushButton *viewAll = new QPushButton("View all  →");
    viewAll->setCursor(Qt::PointingHandCursor);
    viewAll->setStyleSheet(QString(R"(
        QPushButton {
            background: transparent;
            border: none;
            color: %1;
            font-size: 13px;
            font-weight: 700;
            padding: 6px 12px;
        }
        QPushButton:hover {
            color: %2;
        }
    )").arg(AppStyle::Primary, AppStyle::PrimaryHover));
    header->addWidget(viewAll);
    layout->addLayout(header);

    recentBooksLayout = new QVBoxLayout();
    recentBooksLayout->setContentsMargins(0, 0, 0, 0);
    recentBooksLayout->setSpacing(16);

    refreshRecentBooks();
    layout->addLayout(recentBooksLayout);

    connect(viewAll, &QPushButton::clicked, this, &HomeWindow::handleBrowseBooks);
    return section;
}

void HomeWindow::refreshRecentBooks()
{
    if (!recentBooksLayout) return;

    auto *reply = ApiClient::instance().getPublic(QStringLiteral("/api/books"), QUrlQuery("limit=8&sort=newest"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!recentBooksLayout) return;

        QLayoutItem *item;
        while ((item = recentBooksLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                delete item->widget();
                delete item;
            } else if (item->layout()) {
                QLayout *subLayout = item->layout();
                QLayoutItem *subItem;
                while ((subItem = subLayout->takeAt(0)) != nullptr) {
                    if (subItem->widget()) delete subItem->widget();
                    delete subItem;
                }
                delete subLayout;
            } else {
                delete item;
            }
        }

        const QJsonArray booksArr = json.value(QStringLiteral("books")).toArray();
        if (!ok || booksArr.isEmpty()) {
            QLabel *empty = new QLabel("No books have been listed yet.\nBe the first person to sell a book!");
            empty->setAlignment(Qt::AlignCenter);
            empty->setMinimumHeight(160);
            empty->setStyleSheet(QString(R"(
                QLabel {
                    background-color: #FFFFFF;
                    border: 1px solid %1;
                    border-radius: 12px;
                    color: %2;
                    font-size: 14px;
                    font-weight: 600;
                }
            )").arg(AppStyle::BorderSubtle, AppStyle::TextSecondary));
            recentBooksLayout->addWidget(empty);
            return;
        }

        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(16);
        int count = 0;

        for (const auto &v : booksArr) {
            BookModel b = BookModel::fromJson(v.toObject());
            QFrame *card = createBookCard(
                b.id,
                b.title,
                b.author,
                b.condition,
                QString("₹%1").arg(QString::number(b.price, 'f', 0)),
                b.category,
                b.coverImage
            );

            row->addWidget(card);
            ++count;

            if (count == 4) {
                recentBooksLayout->addLayout(row);
                row = new QHBoxLayout();
                row->setSpacing(16);
                count = 0;
            }
        }

        if (count > 0) {
            row->addStretch();
            recentBooksLayout->addLayout(row);
        } else {
            delete row;
        }
    });
}

QPixmap HomeWindow::loadOrGenerateCover(
    const QString &imagePath,
    const QString &title,
    const QString &author,
    int width,
    int height
)
{
    if (!imagePath.isEmpty()) {
        QFileInfo info(imagePath);
        if (info.exists()) {
            QPixmap real(imagePath);
            if (!real.isNull()) {
                return real.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
    }

    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    static const QVector<QPair<QColor, QColor>> palette = {
        { QColor("#4F46E5"), QColor("#312E81") },
        { QColor("#0D9488"), QColor("#115E59") },
        { QColor("#2563EB"), QColor("#1E3A8A") },
        { QColor("#7C3AED"), QColor("#4C1D95") },
        { QColor("#D97706"), QColor("#78350F") },
        { QColor("#DC2626"), QColor("#7F1D1D") }
    };

    const uint hash = qHash(title + author);
    const auto colors = palette[hash % static_cast<uint>(palette.size())];

    QLinearGradient gradient(0, 0, width, height);
    gradient.setColorAt(0, colors.first);
    gradient.setColorAt(1, colors.second);

    QRectF fullRect(0, 0, width, height);
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawRoundedRect(fullRect, 8, 8);

    // Spine
    painter.setBrush(QColor(255, 255, 255, 40));
    painter.drawRect(0, 0, qMax(4, width / 20), height);

    // Title
    QFont titleFont(AppStyle::appFont(), qMax(8, width / 12), QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    QRectF titleRect(width * 0.1, height * 0.16, width * 0.8, height * 0.50);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, title);

    // Author
    QFont authorFont(AppStyle::appFont(), qMax(7, width / 16));
    painter.setFont(authorFont);
    painter.setPen(QColor(255, 255, 255, 210));
    QRectF authorRect(width * 0.1, height * 0.78, width * 0.8, height * 0.16);
    painter.drawText(authorRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, author);

    painter.end();
    return pixmap;
}

QFrame* HomeWindow::createBookCard(
    const QString &bookId,
    const QString &title,
    const QString &author,
    const QString &condition,
    const QString &price,
    const QString &category,
    const QString &imagePath
)
{
    QFrame *card = new QFrame();
    card->setFixedSize(280, 410);
    card->setStyleSheet(QString(R"(
        .QFrame {
            background-color: #FFFFFF;
            border: 1px solid %1;
            border-radius: 14px;
        }
        .QFrame:hover {
            border-color: %2;
        }
        QLabel {
            border: none;
            background: transparent;
        }
    )").arg(AppStyle::BorderSubtle, AppStyle::PrimaryBorder));
    AppStyle::applyElevation(card, 16, 4, 15);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(8);

    // Image container box
    QFrame *imgBox = new QFrame(card);
    imgBox->setFixedHeight(220);
    imgBox->setStyleSheet(QString(R"(
        QFrame {
            background-color: #F8FAFC;
            border: 1px solid %1;
            border-radius: 10px;
        }
    )").arg(AppStyle::BorderSubtle));
    QVBoxLayout *imgBoxLayout = new QVBoxLayout(imgBox);
    imgBoxLayout->setContentsMargins(4, 4, 4, 4);
    imgBoxLayout->setAlignment(Qt::AlignCenter);

    QLabel *image = new QLabel(imgBox);
    image->setFixedSize(160, 205);
    image->setAlignment(Qt::AlignCenter);
    image->setScaledContents(false);

    QPixmap initialPix = loadOrGenerateCover(imagePath, title, author, 150, 200);
    image->setPixmap(initialPix);
    if (!imagePath.isEmpty()) {
        ImageLoader::instance().load(imagePath, image, QSize(150, 200));
    }
    imgBoxLayout->addWidget(image);
    layout->addWidget(imgBox);

    QLabel *titleLabel = new QLabel(title, card);
    titleLabel->setWordWrap(true);
    titleLabel->setFixedHeight(38);
    titleLabel->setStyleSheet(QString("color: %1; font-size: 13.5px; font-weight: 700; line-height: 1.2;").arg(AppStyle::TextPrimary));
    layout->addWidget(titleLabel);

    QLabel *authorLabel = new QLabel("by " + author, card);
    authorLabel->setStyleSheet(QString("color: %1; font-size: 11.5px;").arg(AppStyle::TextSecondary));
    layout->addWidget(authorLabel);

    QHBoxLayout *metaRow = new QHBoxLayout();
    QLabel *condBadge = new QLabel(condition.isEmpty() ? "Good" : condition, card);
    condBadge->setStyleSheet(AppStyle::statusBadgeStyle(condition.isEmpty() ? "AVAILABLE" : condition));
    metaRow->addWidget(condBadge);

    QLabel *catLabel = new QLabel("•  " + (category.isEmpty() ? "General" : category), card);
    catLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(AppStyle::TextMuted));
    metaRow->addWidget(catLabel);
    metaRow->addStretch();
    layout->addLayout(metaRow);

    QHBoxLayout *bottom = new QHBoxLayout();
    QLabel *priceLabel = new QLabel(price, card);
    priceLabel->setStyleSheet(QString("color: %1; font-size: 17px; font-weight: 800;").arg(AppStyle::TextPrimary));
    bottom->addWidget(priceLabel);
    bottom->addStretch();

    QPushButton *viewBtn = new QPushButton("View Details", card);
    viewBtn->setMinimumHeight(34);
    viewBtn->setCursor(Qt::PointingHandCursor);
    viewBtn->setStyleSheet(AppStyle::primaryButtonStyle());
    bottom->addWidget(viewBtn);

    layout->addLayout(bottom);

    connect(viewBtn, &QPushButton::clicked, this, [this, bookId]() {
        showBookDetails(bookId);
    });

    return card;
}

void HomeWindow::showBookDetails(const QString &bookId)
{
    BookDetailsWindow *details = new BookDetailsWindow(bookId, userName, this);
    connect(details, &BookDetailsWindow::backRequested, this, &HomeWindow::popPage);
    connect(details, &BookDetailsWindow::addToCartRequested, this, [this](const QString &) {
        refreshCartCount();
    });
    pushPage(details);
}

QWidget* HomeWindow::createSellSection()
{
    QFrame *section = new QFrame();
    section->setStyleSheet(QString(R"(
        .QFrame {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 16px;
        }
        QLabel {
            border: none;
            background: transparent;
        }
    )").arg(AppStyle::PrimaryLight, AppStyle::PrimaryBorder));
    AppStyle::applyElevation(section, 16, 4, 15);

    QHBoxLayout *layout = new QHBoxLayout(section);
    layout->setContentsMargins(40, 32, 40, 32);

    QVBoxLayout *left = new QVBoxLayout();
    QLabel *small = new QLabel("EARN FROM YOUR BOOKSHELF");
    small->setStyleSheet(QString("background: transparent; border: none; color: %1; font-size: 11px; font-weight: 800; letter-spacing: 1px;").arg(AppStyle::Primary));
    left->addWidget(small);

    QLabel *title = new QLabel("Have books you're done with?");
    title->setStyleSheet(QString("background: transparent; border: none; color: %1; font-size: 22px; font-weight: 800;").arg(AppStyle::TextPrimary));
    left->addWidget(title);

    QLabel *desc = new QLabel("List your books in under a minute, set your own price, and exchange or sell directly to fellow readers.");
    desc->setStyleSheet(QString("background: transparent; border: none; color: %1; font-size: 13px;").arg(AppStyle::TextSecondary));
    left->addWidget(desc);

    layout->addLayout(left, 2);
    layout->addStretch();

    QPushButton *sellBtn = new QPushButton("+ List Your Book Now");
    sellBtn->setMinimumSize(180, 46);
    sellBtn->setCursor(Qt::PointingHandCursor);
    sellBtn->setStyleSheet(AppStyle::primaryButtonStyle());
    layout->addWidget(sellBtn, 0, Qt::AlignCenter);

    connect(sellBtn, &QPushButton::clicked, this, &HomeWindow::handleSellBook);
    return section;
}

QWidget* HomeWindow::createFooter()
{
    QFrame *footer = new QFrame();
    footer->setStyleSheet(R"(
        .QFrame {
            background-color: #0F172A;
            border-radius: 14px;
        }
        QLabel {
            border: none;
            background: transparent;
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(footer);
    layout->setContentsMargins(32, 20, 32, 20);

    QLabel *logo = new QLabel("BookBazzar");
    logo->setStyleSheet("color: #FFFFFF; font-size: 16px; font-weight: 800;");
    layout->addWidget(logo);

    QLabel *text = new QLabel("  •  Community Book Marketplace & Exchange Platform");
    text->setStyleSheet("color: #94A3B8; font-size: 12px;");
    layout->addWidget(text);
    layout->addStretch();

    QLabel *copyright = new QLabel("API-Driven Architecture");
    copyright->setStyleSheet("color: #64748B; font-size: 11px; font-weight: 600;");
    layout->addWidget(copyright);

    return footer;
}

QLabel* HomeWindow::createSectionTitle(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet(QString("color: %1; font-size: 20px; font-weight: 800;").arg(AppStyle::TextPrimary));
    return label;
}

void HomeWindow::handleSearch()
{
    QString searchText = searchEdit->text().trimmed();
    emit browseBooksRequested(searchText, QString());
    BrowseWindow *browseWin = new BrowseWindow(userName, QString(), searchText, this);
    connect(browseWin, &BrowseWindow::backRequested, this, &HomeWindow::popPage);
    connect(browseWin, &BrowseWindow::bookSelected, this, [this](const QString &bookId) {
        showBookDetails(bookId);
    });
    pushPage(browseWin);
}

void HomeWindow::handleBrowseBooks()
{
    emit browseBooksRequested(QString(), QString());
    BrowseWindow *browseWin = new BrowseWindow(userName, QString(), QString(), this);
    connect(browseWin, &BrowseWindow::backRequested, this, &HomeWindow::popPage);
    connect(browseWin, &BrowseWindow::bookSelected, this, [this](const QString &bookId) {
        showBookDetails(bookId);
    });
    pushPage(browseWin);
}

void HomeWindow::handleBrowseWithCategory(const QString &category)
{
    emit browseBooksRequested(QString(), category);
    BrowseWindow *browseWin = new BrowseWindow(userName, category, QString(), this);
    connect(browseWin, &BrowseWindow::backRequested, this, &HomeWindow::popPage);
    connect(browseWin, &BrowseWindow::bookSelected, this, [this](const QString &bookId) {
        showBookDetails(bookId);
    });
    pushPage(browseWin);
}

void HomeWindow::handleSellBook()
{
    emit sellBookRequested();
    SellWindow *sellWin = new SellWindow(userName, this);
    connect(sellWin, &SellWindow::backRequested, this, &HomeWindow::popPage);
    connect(sellWin, &SellWindow::bookPublished, this, [this]() {
        refreshRecentBooks();
        popPage();
    });
    pushPage(sellWin);
}

void HomeWindow::handleListings()
{
    emit listingsRequested();
    ListingsWindow *listings = new ListingsWindow(userName, this);
    connect(listings, &ListingsWindow::backRequested, this, &HomeWindow::popPage);
    connect(listings, &ListingsWindow::addNewListingRequested, this, [this]() {
        handleSellBook();
    });
    pushPage(listings);
}

void HomeWindow::handleOrders()
{
    emit ordersRequested();
    OrdersWindow *orders = new OrdersWindow(userName, this);
    connect(orders, &OrdersWindow::backRequested, this, &HomeWindow::popPage);
    connect(orders, &OrdersWindow::browseRequested, this, [this]() {
        popPage();
        handleBrowseBooks();
    });
    connect(orders, &OrdersWindow::orderSelected, this, [this](const QString &orderId) {
        OrderDetailWindow *detail = new OrderDetailWindow(orderId, this);
        connect(detail, &OrderDetailWindow::backRequested, this, &HomeWindow::popPage);
        pushPage(detail);
    });
    pushPage(orders);
}

void HomeWindow::handleCart()
{
    emit cartRequested();
    CartWindow *cartWin = new CartWindow(this);
    connect(cartWin, &CartWindow::backRequested, this, &HomeWindow::popPage);
    connect(cartWin, &CartWindow::browseRequested, this, [this]() {
        popPage();
        handleBrowseBooks();
    });
    connect(cartWin, &CartWindow::cartCountChanged, this, &HomeWindow::updateCartBadge);
    connect(cartWin, &CartWindow::checkoutRequested, this, [this, cartWin]() {
        CheckoutWindow *checkout = new CheckoutWindow(this);
        connect(checkout, &CheckoutWindow::backRequested, this, &HomeWindow::popPage);
        connect(checkout, &CheckoutWindow::orderPlaced, this, [this](const QString &orderId) {
            popPage(); // pop checkout
            popPage(); // pop cart
            refreshCartCount();

            OrderDetailWindow *detail = new OrderDetailWindow(orderId, this);
            connect(detail, &OrderDetailWindow::backRequested, this, &HomeWindow::popPage);
            pushPage(detail);
        });
        pushPage(checkout);
    });
    pushPage(cartWin);
}

void HomeWindow::handleExchanges()
{
    emit exchangesRequested();
    ExchangeWindow *exWin = new ExchangeWindow(this);
    connect(exWin, &ExchangeWindow::backRequested, this, &HomeWindow::popPage);
    pushPage(exWin);
}

void HomeWindow::showUserProfile()
{
    auto *reply = ApiClient::instance().get(QStringLiteral("/api/auth/me"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        QDialog dlg(this);
        dlg.setWindowTitle("BookBazzar - User Profile");
        dlg.resize(420, 340);
        dlg.setStyleSheet(QString("background-color: %1;").arg(AppStyle::Background));

        QVBoxLayout *layout = new QVBoxLayout(&dlg);
        layout->setContentsMargins(28, 28, 28, 28);
        layout->setSpacing(16);

        QFrame *card = new QFrame(&dlg);
        card->setStyleSheet(AppStyle::cardStyle());
        AppStyle::applyElevation(card, 16, 4, 15);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(24, 24, 24, 24);
        cardLayout->setSpacing(12);

        QLabel *heading = new QLabel("Account Profile");
        heading->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: 800;").arg(AppStyle::TextPrimary));
        cardLayout->addWidget(heading);

        QJsonObject userObj = json.value(QStringLiteral("user")).toObject();
        QString uname = userObj.value(QStringLiteral("username")).toString(SessionManager::instance().username());
        QString email = userObj.value(QStringLiteral("email")).toString("—");
        QString uid = userObj.value(QStringLiteral("id")).toString(SessionManager::instance().userId());
        QString role = userObj.value(QStringLiteral("role")).toString("user");
        QString createdAt = userObj.value(QStringLiteral("createdAt")).toString();
        if (!createdAt.isEmpty()) {
            QDateTime dt = QDateTime::fromString(createdAt, Qt::ISODate);
            if (dt.isValid()) {
                createdAt = dt.toString("MMM d, yyyy");
            }
        } else {
            createdAt = "Active Member";
        }

        auto addField = [&cardLayout](const QString &label, const QString &val) {
            QHBoxLayout *row = new QHBoxLayout();
            QLabel *l = new QLabel(label);
            l->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600; min-width: 90px;").arg(AppStyle::TextSecondary));
            QLabel *v = new QLabel(val);
            v->setStyleSheet(QString("color: %1; font-size: 13px;").arg(AppStyle::TextPrimary));
            v->setTextInteractionFlags(Qt::TextSelectableByMouse);
            row->addWidget(l);
            row->addWidget(v, 1);
            cardLayout->addLayout(row);
        };

        addField("Username:", uname);
        addField("Email:", email);
        addField("User ID:", uid);
        addField("Role:", role);
        addField("Joined:", createdAt);

        layout->addWidget(card);

        QPushButton *closeBtn = new QPushButton("Close");
        closeBtn->setMinimumHeight(40);
        closeBtn->setStyleSheet(AppStyle::primaryButtonStyle());
        connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        layout->addWidget(closeBtn);

        dlg.exec();
    });
}

void HomeWindow::refreshCartCount()
{
    auto *reply = ApiClient::instance().get(QStringLiteral("/api/cart"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();
        if (ok && json.contains(QStringLiteral("cart"))) {
            auto cartObj = json[QStringLiteral("cart")].toObject();
            auto itemsArr = cartObj[QStringLiteral("items")].toArray();
            updateCartBadge(itemsArr.size());
        }
    });
}

void HomeWindow::updateCartBadge(int count)
{
    if (cartButton) {
        cartButton->setText(QString("Cart (%1)").arg(count));
    }
}

void HomeWindow::handleLogout()
{
    bool confirmed = StyledMessageBox::question(
        this, "Logout", "Are you sure you want to log out of BookBazzar?",
        "Log Out", "Cancel"
    );

    if (confirmed) {
        auto *logoutReply = ApiClient::instance().post(QStringLiteral("/api/auth/logout"), {}, false);
        connect(logoutReply, &QNetworkReply::finished, logoutReply, &QNetworkReply::deleteLater);
        SessionManager::instance().clearSession();
        emit logoutRequested();
    }
}