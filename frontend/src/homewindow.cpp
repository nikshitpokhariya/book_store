#include "homewindow.h"
#include "bookdetailswindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include "models/DataModels.h"
#include <QUrlQuery>
#include <QNetworkReply>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QMessageBox>
#include <QSizePolicy>
#include <QPixmap>
#include <QFileInfo>
#include <QFont>
#include <QStringList>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QLinearGradient>
#include <QVector>
#include <QPair>

// =========================================================
// CONSTRUCTOR
// =========================================================

HomeWindow::HomeWindow(const QString &userName,
                       QWidget *parent)
    : QWidget(parent),
    userName(userName),
    searchEdit(nullptr),
    searchButton(nullptr),
    cartButton(nullptr),
    exchangesButton(nullptr),
    logoutButton(nullptr),
    recentBooksLayout(nullptr)
{
    setWindowTitle("BookBazzar - Home");

    resize(1400, 850);
    setMinimumSize(1000, 650);

    setupUI();
    refreshCartCount();
}


// =========================================================
// DESTRUCTOR
// =========================================================

HomeWindow::~HomeWindow()
{
}


// =========================================================
// ELEVATION HELPER
// =========================================================
// A soft drop shadow is what makes cards/panels read as "raised"
// surfaces instead of flat rectangles - this is one of the biggest
// single changes that makes a UI look like a real product rather
// than a placeholder mockup.

void HomeWindow::applyElevation(QWidget *widget, int blurRadius,
                                int yOffset, int alpha)
{
    QGraphicsDropShadowEffect *shadow =
        new QGraphicsDropShadowEffect(widget);

    shadow->setBlurRadius(blurRadius);
    shadow->setOffset(0, yOffset);
    shadow->setColor(QColor(23, 32, 51, alpha));

    widget->setGraphicsEffect(shadow);
}


// =========================================================
// MAIN UI
// =========================================================

void HomeWindow::setupUI()
{
    setStyleSheet(R"(

        QWidget {
            font-family: "Segoe UI";
        }

        QScrollArea {
            border: none;
            background-color: #F7F8FC;
        }

        QScrollBar:vertical {
            width: 10px;
            background: transparent;
            margin: 4px;
        }

        QScrollBar::handle:vertical {
            background: #C7CBD6;
            border-radius: 5px;
            min-height: 45px;
        }

        QScrollBar::handle:vertical:hover {
            background: #A5A9B4;
        }

        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0px;
        }

        QScrollBar:horizontal {
            height: 0px;
        }

    )");

    QScrollArea *scrollArea = new QScrollArea(this);

    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *page = new QWidget();
    page->setObjectName("homePage");
    page->setStyleSheet(R"(
        QWidget#homePage {
            background-color: #F7F8FC;
        }
    )");

    QVBoxLayout *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    pageLayout->addWidget(createNavigationBar());

    QWidget *content = new QWidget();
    content->setObjectName("contentWrapper");
    content->setStyleSheet(R"(
        QWidget#contentWrapper {
            background-color: #F7F8FC;
        }
    )");

    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 28, 32, 40);
    contentLayout->setSpacing(0);

    contentLayout->addWidget(createHeroSection());
    contentLayout->addSpacing(35);
    contentLayout->addWidget(createCategorySection());
    contentLayout->addSpacing(38);
    contentLayout->addWidget(createRecentlyListedSection());
    contentLayout->addSpacing(42);
    contentLayout->addWidget(createSellSection());
    contentLayout->addSpacing(42);
    contentLayout->addWidget(createWhySection());
    contentLayout->addSpacing(42);
    contentLayout->addWidget(createFooter());

    pageLayout->addWidget(content, 1);

    scrollArea->setWidget(page);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(scrollArea);
}


// =========================================================
// NAVIGATION BAR
// =========================================================

QWidget* HomeWindow::createNavigationBar()
{
    QFrame *nav = new QFrame();
    nav->setObjectName("navigationBar");
    nav->setFixedHeight(76);
    nav->setStyleSheet(R"(
        QFrame#navigationBar {
            background-color: white;
            border-bottom: 1px solid #E6E8EF;
        }
    )");

    // A faint shadow under the nav bar separates it from scrolled
    // content beneath it, like a real app's sticky header.
    applyElevation(nav, 20, 3, 18);

    QHBoxLayout *layout = new QHBoxLayout(nav);
    layout->setContentsMargins(32, 10, 32, 10);
    layout->setSpacing(6);

    QLabel *logo = new QLabel("BookBazzar");
    logo->setStyleSheet(R"(
        QLabel {
            color: #172033;
            font-size: 25px;
            font-weight: 800;
        }
    )");
    layout->addWidget(logo);

    QLabel *dot = new QLabel(".");
    dot->setStyleSheet(R"(
        QLabel {
            color: #5B5FEF;
            font-size: 30px;
            font-weight: 800;
        }
    )");
    layout->addWidget(dot);
    layout->addSpacing(30);

    auto createNavButton = [](const QString &text)
    {
        QPushButton *button = new QPushButton(text);
        button->setMinimumHeight(40);
        button->setStyleSheet(R"(
            QPushButton {
                background: transparent;
                border: none;
                color: #4B5563;
                padding: 0 12px;
                font-size: 14px;
                font-weight: 600;
                border-radius: 8px;
            }
            QPushButton:hover {
                color: #4F46E5;
                background-color: #F3F4FF;
            }
            QPushButton:pressed {
                background-color: #E9E7FF;
            }
        )");
        return button;
    };

    QPushButton *browseButton = createNavButton("Browse Books");
    QPushButton *sellButton = createNavButton("Sell Your Book");
    QPushButton *listingButton = createNavButton("My Listings");
    QPushButton *ordersButton = createNavButton("My Orders");
    exchangesButton = createNavButton("Exchanges");
    cartButton = createNavButton("Cart (0)");

    layout->addWidget(browseButton);
    layout->addWidget(sellButton);
    layout->addWidget(listingButton);
    layout->addWidget(ordersButton);
    layout->addWidget(exchangesButton);
    layout->addWidget(cartButton);
    layout->addStretch();

    const QString displayName = SessionManager::instance().username().isEmpty() ? userName : SessionManager::instance().username();
    QLabel *userLabel = new QLabel("Hello, " + displayName);
    userLabel->setStyleSheet(R"(
        QLabel {
            color: #374151;
            font-size: 14px;
            font-weight: 600;
            padding: 0 12px;
        }
    )");
    layout->addWidget(userLabel);

    logoutButton = new QPushButton("Logout");
    logoutButton->setMinimumSize(90, 40);
    logoutButton->setStyleSheet(R"(
        QPushButton {
            background-color: #F8F9FC;
            color: #374151;
            border: 1px solid #E1E4EA;
            border-radius: 9px;
            font-size: 13px;
            font-weight: 700;
        }
        QPushButton:hover {
            background-color: #FEECEC;
            color: #DC2626;
            border-color: #FECACA;
        }
        QPushButton:pressed {
            background-color: #FDE2E2;
        }
    )");
    layout->addWidget(logoutButton);

    connect(browseButton, &QPushButton::clicked, this, &HomeWindow::handleBrowseBooks);
    connect(sellButton, &QPushButton::clicked, this, &HomeWindow::handleSellBook);
    connect(listingButton, &QPushButton::clicked, this, &HomeWindow::handleListings);
    connect(ordersButton, &QPushButton::clicked, this, &HomeWindow::handleOrders);
    connect(exchangesButton, &QPushButton::clicked, this, &HomeWindow::handleExchanges);
    connect(cartButton, &QPushButton::clicked, this, &HomeWindow::handleCart);
    connect(logoutButton, &QPushButton::clicked, this, &HomeWindow::handleLogout);

    return nav;
}


// =========================================================
// HERO SECTION
// =========================================================

QWidget* HomeWindow::createHeroSection()
{
    QFrame *hero = new QFrame();
    hero->setObjectName("heroSection");
    hero->setMinimumHeight(380);
    hero->setStyleSheet(R"(
        QFrame#heroSection {
            background-color: #171D2D;
            border-radius: 24px;
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(hero);
    layout->setContentsMargins(55, 45, 45, 45);
    layout->setSpacing(40);

    QVBoxLayout *left = new QVBoxLayout();
    left->setSpacing(0);

    QLabel *smallTitle = new QLabel("THE SMARTER WAY TO BUY & SELL BOOKS");
    smallTitle->setStyleSheet(R"(
        QLabel {
            color: #A5B4FC;
            font-size: 11px;
            font-weight: 800;
        }
    )");
    left->addWidget(smallTitle);
    left->addSpacing(14);

    QLabel *heading = new QLabel(
        "Find your next book.\n"
        "Give your old ones a new life."
        );
    heading->setWordWrap(true);
    heading->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 38px;
            font-weight: 800;
        }
    )");
    left->addWidget(heading);
    left->addSpacing(15);

    QLabel *description = new QLabel(
        "Discover affordable books from students, "
        "readers and collectors around you — "
        "or sell books you no longer need."
        );
    description->setWordWrap(true);
    description->setMaximumWidth(650);
    description->setStyleSheet(R"(
        QLabel {
            color: #C9D1E0;
            font-size: 14px;
        }
    )");
    left->addWidget(description);
    left->addSpacing(25);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->setSpacing(8);

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search by title, author or ISBN...");
    searchEdit->setMinimumHeight(52);
    searchEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: white;
            color: #111827;
            border: 2px solid transparent;
            border-radius: 11px;
            padding: 0 18px;
            font-size: 13px;
        }
        QLineEdit:focus {
            border: 2px solid #818CF8;
        }
    )");

    searchButton = new QPushButton("Search");
    searchButton->setMinimumSize(105, 52);
    searchButton->setStyleSheet(R"(
        QPushButton {
            background-color: #6366F1;
            color: white;
            border: none;
            border-radius: 11px;
            font-size: 13px;
            font-weight: 800;
        }
        QPushButton:hover {
            background-color: #818CF8;
        }
        QPushButton:pressed {
            background-color: #4F46E5;
        }
    )");

    searchLayout->addWidget(searchEdit, 1);
    searchLayout->addWidget(searchButton);
    left->addLayout(searchLayout);
    left->addSpacing(20);

    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->setSpacing(10);

    QPushButton *browse = createPrimaryButton("Browse Books");
    QPushButton *sell = createSecondaryButton("+ Sell Your Book");

    buttons->addWidget(browse);
    buttons->addWidget(sell);
    buttons->addStretch();

    left->addLayout(buttons);

    layout->addLayout(left, 3);

    QFrame *visual = new QFrame();
    visual->setMinimumWidth(300);
    visual->setMaximumWidth(430);
    visual->setStyleSheet(R"(
        QFrame {
            background-color: #202940;
            border: 1px solid #303A56;
            border-radius: 20px;
        }
    )");

    QVBoxLayout *visualLayout = new QVBoxLayout(visual);
    visualLayout->setContentsMargins(25, 22, 25, 22);

    QLabel *visualTitle = new QLabel("DISCOVER • READ • REPEAT");
    visualTitle->setAlignment(Qt::AlignCenter);
    visualTitle->setStyleSheet(R"(
        QLabel {
            color: #9DA8C1;
            font-size: 10px;
            font-weight: 800;
        }
    )");
    visualLayout->addWidget(visualTitle);
    visualLayout->addStretch();

    // Hero "books" now use the same generated-cover look as the real
    // book cards, instead of flat solid rectangles, so this panel
    // reads as book covers at a glance rather than colored blocks.
    QHBoxLayout *books = new QHBoxLayout();
    books->setSpacing(12);
    books->setAlignment(Qt::AlignCenter);

    auto addHeroBook = [this, books](const QString &title,
                                     const QString &author,
                                     int width,
                                     int height)
    {
        QLabel *cover = new QLabel();
        cover->setFixedSize(width, height);
        cover->setPixmap(loadOrGenerateCover("", title, author, width, height));

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(cover);
        shadow->setBlurRadius(18);
        shadow->setOffset(0, 5);
        shadow->setColor(QColor(0, 0, 0, 90));
        cover->setGraphicsEffect(shadow);

        books->addWidget(cover);
    };

    addHeroBook("Atomic Habits", "James Clear", 85, 135);
    addHeroBook("C++ Programming", "Bjarne Stroustrup", 95, 155);
    addHeroBook("The Alchemist", "Paulo Coelho", 82, 128);

    visualLayout->addLayout(books);
    visualLayout->addStretch();

    QLabel *visualText = new QLabel(
        "Thousands of stories.\n"
        "One place to find them."
        );
    visualText->setAlignment(Qt::AlignCenter);
    visualText->setStyleSheet(R"(
        QLabel {
            color: #D5DAE7;
            font-size: 13px;
            font-weight: 600;
        }
    )");
    visualLayout->addWidget(visualText);

    layout->addWidget(visual, 1);

    connect(searchButton, &QPushButton::clicked, this, &HomeWindow::handleSearch);
    connect(searchEdit, &QLineEdit::returnPressed, this, &HomeWindow::handleSearch);
    connect(browse, &QPushButton::clicked, this, &HomeWindow::handleBrowseBooks);
    connect(sell, &QPushButton::clicked, this, &HomeWindow::handleSellBook);

    return hero;
}


// =========================================================
// CATEGORY SECTION
// =========================================================

QWidget* HomeWindow::createCategorySection()
{
    QWidget *section = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *header = new QHBoxLayout();
    header->addWidget(createSectionTitle("Explore Categories"));
    header->addStretch();

    QLabel *sub = new QLabel("Find books that match your interests");
    sub->setStyleSheet(R"(
        QLabel {
            color: #9CA3AF;
            font-size: 12px;
        }
    )");
    header->addWidget(sub);

    layout->addLayout(header);
    layout->addSpacing(16);

    QHBoxLayout *categories = new QHBoxLayout();
    categories->setSpacing(12);

    QStringList categoryNames = {
        "Academic", "Programming", "Engineering", "Fiction",
        "Competitive Exams", "School", "Novels"
    };

    QStringList categoryIcons = {
        "A", "{ }", "E", "F", "C", "S", "N"
    };

    for (int i = 0; i < categoryNames.size(); ++i)
    {
        QFrame *card = new QFrame();
        card->setMinimumHeight(72);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        card->setStyleSheet(R"(
            QFrame {
                background-color: white;
                border: 1px solid #E5E7EB;
                border-radius: 13px;
            }
        )");

        applyElevation(card, 16, 4, 12);

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 10, 10, 8);
        cardLayout->setSpacing(5);

        QLabel *icon = new QLabel(categoryIcons[i]);
        icon->setAlignment(Qt::AlignCenter);
        icon->setStyleSheet(R"(
            QLabel {
                color: #4F46E5;
                font-size: 15px;
                font-weight: 900;
            }
        )");

        QLabel *name = new QLabel(categoryNames[i]);
        name->setAlignment(Qt::AlignCenter);
        name->setStyleSheet(R"(
            QLabel {
                color: #374151;
                font-size: 11px;
                font-weight: 700;
            }
        )");

        cardLayout->addWidget(icon);
        cardLayout->addWidget(name);

        categories->addWidget(card);
    }

    layout->addLayout(categories);

    return section;
}


// =========================================================
// RECENTLY LISTED
// =========================================================

QWidget* HomeWindow::createRecentlyListedSection()
{
    QWidget *section = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *header = new QHBoxLayout();
    header->addWidget(createSectionTitle("Recently Listed Books"));
    header->addStretch();

    QPushButton *viewAll = new QPushButton("View all  →");
    viewAll->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: none;
            color: #4F46E5;
            font-size: 13px;
            font-weight: 800;
            padding: 8px;
        }
        QPushButton:hover {
            color: #3730A3;
        }
    )");

    header->addWidget(viewAll);
    layout->addLayout(header);
    layout->addSpacing(17);

    recentBooksLayout = new QVBoxLayout();
    recentBooksLayout->setContentsMargins(0, 0, 0, 0);
    recentBooksLayout->setSpacing(16);

    refreshRecentBooks();

    layout->addLayout(recentBooksLayout);

    connect(viewAll, &QPushButton::clicked, this, &HomeWindow::handleBrowseBooks);

    return section;
}


// =========================================================
// REFRESH RECENT BOOKS
// =========================================================

void HomeWindow::refreshRecentBooks()
{
    if (!recentBooksLayout)
    {
        return;
    }

    auto *reply = ApiClient::instance().getPublic(QStringLiteral("/api/books"), QUrlQuery("limit=8&sort=newest"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!recentBooksLayout) return;

        QLayoutItem *item;
        while ((item = recentBooksLayout->takeAt(0)) != nullptr)
        {
            if (item->widget())
            {
                delete item->widget();
                delete item;
            }
            else if (item->layout())
            {
                QLayout *subLayout = item->layout();
                QLayoutItem *subItem;
                while ((subItem = subLayout->takeAt(0)) != nullptr)
                {
                    if (subItem->widget())
                    {
                        delete subItem->widget();
                    }
                    delete subItem;
                }
                delete subLayout;
            }
            else
            {
                delete item;
            }
        }

        const QJsonArray booksArr = json.value(QStringLiteral("books")).toArray();

        if (!ok || booksArr.isEmpty())
        {
            QLabel *empty = new QLabel(
                "No books have been listed yet.\n\n"
                "Be the first person to sell a book!"
                );
            empty->setAlignment(Qt::AlignCenter);
            empty->setMinimumHeight(180);
            empty->setStyleSheet(R"(
                QLabel {
                    background-color: white;
                    border: 1px solid #E5E7EB;
                    border-radius: 15px;
                    color: #6B7280;
                    font-size: 14px;
                    font-weight: 600;
                }
            )");

            recentBooksLayout->addWidget(empty);
            return;
        }

        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(16);

        int count = 0;

        for (const auto &v : booksArr)
        {
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

            if (count == 4)
            {
                recentBooksLayout->addLayout(row);
                row = new QHBoxLayout();
                row->setSpacing(16);
                count = 0;
            }
        }

        if (count > 0)
        {
            row->addStretch();
            recentBooksLayout->addLayout(row);
        }
        else
        {
            delete row;
        }
    });
}


// =========================================================
// COVER ART
// =========================================================
// Tries to load a real cover image first. If none exists yet,
// generates a clean, book-jacket-style placeholder instead of a
// flat color box with initials - gradient background (color picked
// consistently per title so the same book always looks the same),
// a spine accent, a faint watermark, and the actual title/author
// text laid out like a real cover. No copyrighted art involved -
// this is drawn entirely in code.

QPixmap HomeWindow::loadOrGenerateCover(
    const QString &imagePath,
    const QString &title,
    const QString &author,
    int width,
    int height
    )
{
    if (!imagePath.isEmpty())
    {
        QFileInfo info(imagePath);

        if (info.exists())
        {
            QPixmap real(imagePath);

            if (!real.isNull())
            {
                return real.scaled(
                    width, height,
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                    );
            }
        }
    }

    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    static const QVector<QPair<QColor, QColor>> palette = {
                                                            { QColor("#6366F1"), QColor("#3730A3") },
                                                            { QColor("#F59E0B"), QColor("#B45309") },
                                                            { QColor("#10B981"), QColor("#047857") },
                                                            { QColor("#EC4899"), QColor("#9D174D") },
                                                            { QColor("#3B82F6"), QColor("#1E40AF") },
                                                            { QColor("#8B5CF6"), QColor("#5B21B6") },
                                                            { QColor("#EF4444"), QColor("#991B1B") },
                                                            { QColor("#14B8A6"), QColor("#115E59") },
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

    // Spine accent along the left edge.
    painter.setBrush(QColor(255, 255, 255, 45));
    painter.drawRect(0, 0, qMax(3, width / 22), height);

    // Faint open-book watermark, purely decorative.
    QFont iconFont = painter.font();
    iconFont.setPointSize(qMax(10, height / 5));
    painter.setFont(iconFont);
    painter.setPen(QColor(255, 255, 255, 30));
    painter.drawText(fullRect, Qt::AlignCenter, QString::fromUtf8("\U0001F4D6"));

    // Title, word-wrapped and centered in the upper portion.
    QFont titleFont("Segoe UI", qMax(8, width / 13), QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    QRectF titleRect(width * 0.08, height * 0.14, width * 0.84, height * 0.52);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, title);

    // Author, smaller, near the bottom.
    QFont authorFont("Segoe UI", qMax(6, width / 16));
    painter.setFont(authorFont);
    painter.setPen(QColor(255, 255, 255, 215));
    QRectF authorRect(width * 0.08, height * 0.80, width * 0.84, height * 0.16);
    painter.drawText(authorRect, Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, author);

    painter.end();
    return pixmap;
}


// =========================================================
// BOOK CARD
// =========================================================

QFrame* HomeWindow::createBookCard(
    const QString &bookId,
    const QString &title,
    const QString &author,
    const QString &condition,
    const QString &price,
    const QString &location,
    const QString &imagePath
    )
{
    QFrame *card = new QFrame();

    card->setMinimumHeight(355);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    card->setStyleSheet(R"(
        QFrame {
            background-color: white;
            border: 1px solid #E5E7EB;
            border-radius: 16px;
        }
        QFrame:hover {
            border-color: #C7D2FE;
            background-color: #FEFEFF;
        }
    )");

    applyElevation(card, 24, 6, 22);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(7);

    // Real cover if available, otherwise a generated book-jacket-style
    // placeholder instead of a flat letter box.
    QLabel *image = new QLabel();
    image->setFixedHeight(160);
    image->setAlignment(Qt::AlignCenter);
    image->setPixmap(loadOrGenerateCover(imagePath, title, author, 125, 155));

    layout->addWidget(image);

    QLabel *titleLabel = new QLabel(title);
    titleLabel->setWordWrap(true);
    titleLabel->setMaximumHeight(42);
    titleLabel->setStyleSheet(R"(
        QLabel {
            color: #172033;
            font-size: 14px;
            font-weight: 800;
        }
    )");
    layout->addWidget(titleLabel);

    QLabel *authorLabel = new QLabel("by " + author);
    authorLabel->setStyleSheet(R"(
        QLabel {
            color: #6B7280;
            font-size: 11px;
        }
    )");
    layout->addWidget(authorLabel);

    QLabel *conditionLabel = new QLabel(condition);
    conditionLabel->setAlignment(Qt::AlignCenter);
    conditionLabel->setMaximumWidth(90);
    conditionLabel->setStyleSheet(R"(
        QLabel {
            background-color: #ECFDF5;
            color: #047857;
            border-radius: 6px;
            padding: 4px 8px;
            font-size: 10px;
            font-weight: 800;
        }
    )");
    layout->addWidget(conditionLabel, 0, Qt::AlignLeft);

    QLabel *locationLabel = new QLabel("Location  •  " + location);
    locationLabel->setStyleSheet(R"(
        QLabel {
            color: #9CA3AF;
            font-size: 10px;
        }
    )");
    layout->addWidget(locationLabel);

    layout->addStretch();

    QHBoxLayout *bottom = new QHBoxLayout();

    QLabel *priceLabel = new QLabel(price);
    priceLabel->setStyleSheet(R"(
        QLabel {
            color: #111827;
            font-size: 18px;
            font-weight: 900;
        }
    )");
    bottom->addWidget(priceLabel);
    bottom->addStretch();

    QPushButton *viewButton = new QPushButton("View Book");
    viewButton->setMinimumHeight(34);
    viewButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4F46E5;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 0 13px;
            font-size: 11px;
            font-weight: 800;
        }
        QPushButton:hover {
            background-color: #4338CA;
        }
        QPushButton:pressed {
            background-color: #3730A3;
        }
    )");

    bottom->addWidget(viewButton);
    layout->addLayout(bottom);

    connect(viewButton, &QPushButton::clicked, this, [this, bookId]()
            {
                showBookDetails(bookId);
            });

    return card;
}


// =========================================================
// SHOW BOOK DETAILS
// =========================================================

void HomeWindow::showBookDetails(const QString &bookId)
{
    BookDetailsWindow *details = new BookDetailsWindow(bookId, userName);
    details->setAttribute(Qt::WA_DeleteOnClose);
    connect(details, &BookDetailsWindow::backRequested, details, &QWidget::close);
    connect(details, &BookDetailsWindow::addToCartRequested, this, [this](const QString &) {
        refreshCartCount();
    });
    details->show();
    details->raise();
    details->activateWindow();
}


// =========================================================
// SELL SECTION
// =========================================================

QWidget* HomeWindow::createSellSection()
{
    QFrame *section = new QFrame();
    section->setMinimumHeight(190);
    section->setStyleSheet(R"(
        QFrame {
            background-color: #EEF2FF;
            border: 1px solid #DDE3FF;
            border-radius: 20px;
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(section);
    layout->setContentsMargins(38, 30, 38, 30);

    QVBoxLayout *left = new QVBoxLayout();

    QLabel *small = new QLabel("SELL YOUR BOOKS");
    small->setStyleSheet(R"(
        QLabel {
            color: #4F46E5;
            font-size: 11px;
            font-weight: 900;
        }
    )");
    left->addWidget(small);

    QLabel *title = new QLabel("Have books you're done with?");
    title->setStyleSheet(R"(
        QLabel {
            color: #172033;
            font-size: 24px;
            font-weight: 800;
        }
    )");
    left->addWidget(title);

    QLabel *description = new QLabel(
        "List your books, choose your price and "
        "connect with another reader."
        );
    description->setWordWrap(true);
    description->setStyleSheet(R"(
        QLabel {
            color: #6B7280;
            font-size: 13px;
        }
    )");
    left->addWidget(description);

    layout->addLayout(left, 2);
    layout->addStretch();

    QPushButton *sellButton = createPrimaryButton("+ List Your Book");
    sellButton->setMinimumSize(175, 52);

    layout->addWidget(sellButton, 0, Qt::AlignCenter);

    connect(sellButton, &QPushButton::clicked, this, &HomeWindow::handleSellBook);

    return section;
}


// =========================================================
// WHY SECTION
// =========================================================

QWidget* HomeWindow::createWhySection()
{
    QWidget *section = new QWidget();

    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel *heading = createSectionTitle("Why BookBazzar?");
    heading->setAlignment(Qt::AlignCenter);
    layout->addWidget(heading);

    QLabel *subtitle = new QLabel("A simple marketplace designed for book lovers.");
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(R"(
        QLabel {
            color: #9CA3AF;
            font-size: 12px;
        }
    )");
    layout->addWidget(subtitle);
    layout->addSpacing(22);

    QHBoxLayout *features = new QHBoxLayout();
    features->setSpacing(18);

    QStringList numbers = { "01", "02", "03" };

    QStringList titles = {
        "Real Book Listings",
        "Your Price, Your Choice",
        "Simple Ordering"
    };

    QStringList descriptions = {
        "Discover books listed by real readers "
        "with their condition and price.",

        "Sellers decide the price of their books "
        "and control their listings.",

        "Find a book you want and place an order "
        "in just a few simple steps."
    };

    for (int i = 0; i < 3; ++i)
    {
        QFrame *feature = new QFrame();
        feature->setMinimumHeight(150);
        feature->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        feature->setStyleSheet(R"(
            QFrame {
                background-color: white;
                border: 1px solid #E5E7EB;
                border-radius: 15px;
            }
        )");

        applyElevation(feature, 18, 5, 14);

        QVBoxLayout *featureLayout = new QVBoxLayout(feature);
        featureLayout->setContentsMargins(24, 22, 24, 22);

        QLabel *number = new QLabel(numbers[i]);
        number->setStyleSheet(R"(
            QLabel {
                color: #4F46E5;
                font-size: 12px;
                font-weight: 900;
            }
        )");
        featureLayout->addWidget(number);

        QLabel *featureTitle = new QLabel(titles[i]);
        featureTitle->setStyleSheet(R"(
            QLabel {
                color: #172033;
                font-size: 16px;
                font-weight: 800;
            }
        )");
        featureLayout->addWidget(featureTitle);

        QLabel *text = new QLabel(descriptions[i]);
        text->setWordWrap(true);
        text->setStyleSheet(R"(
            QLabel {
                color: #6B7280;
                font-size: 12px;
            }
        )");
        featureLayout->addWidget(text);

        features->addWidget(feature);
    }

    layout->addLayout(features);

    return section;
}


// =========================================================
// FOOTER
// =========================================================

QWidget* HomeWindow::createFooter()
{
    QFrame *footer = new QFrame();
    footer->setMinimumHeight(90);
    footer->setStyleSheet(R"(
        QFrame {
            background-color: #171D2D;
            border-radius: 15px;
        }
    )");

    QHBoxLayout *layout = new QHBoxLayout(footer);
    layout->setContentsMargins(28, 20, 28, 20);

    QLabel *logo = new QLabel("BookBazzar");
    logo->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 18px;
            font-weight: 800;
        }
    )");
    layout->addWidget(logo);

    QLabel *text = new QLabel("  •  Buy books. Sell books. Give them a new life.");
    text->setStyleSheet(R"(
        QLabel {
            color: #9CA8BD;
            font-size: 12px;
        }
    )");
    layout->addWidget(text);
    layout->addStretch();

    QLabel *copyright = new QLabel("BookBazzar Marketplace");
    copyright->setStyleSheet(R"(
        QLabel {
            color: #6F7B91;
            font-size: 11px;
        }
    )");
    layout->addWidget(copyright);

    return footer;
}


// =========================================================
// PRIMARY BUTTON
// =========================================================

QPushButton* HomeWindow::createPrimaryButton(const QString &text)
{
    QPushButton *button = new QPushButton(text);
    button->setMinimumHeight(42);
    button->setStyleSheet(R"(
        QPushButton {
            background-color: #5B5FEF;
            color: white;
            border: none;
            border-radius: 9px;
            padding: 0 20px;
            font-size: 13px;
            font-weight: 800;
        }
        QPushButton:hover {
            background-color: #4F46E5;
        }
        QPushButton:pressed {
            background-color: #4338CA;
        }
    )");

    return button;
}


// =========================================================
// SECONDARY BUTTON
// =========================================================

QPushButton* HomeWindow::createSecondaryButton(const QString &text)
{
    QPushButton *button = new QPushButton(text);
    button->setMinimumHeight(42);
    button->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(255,255,255,0.07);
            color: white;
            border: 1px solid #4B556B;
            border-radius: 9px;
            padding: 0 20px;
            font-size: 13px;
            font-weight: 800;
        }
        QPushButton:hover {
            background-color: rgba(255,255,255,0.14);
            border-color: #68758D;
        }
        QPushButton:pressed {
            background-color: rgba(255,255,255,0.20);
        }
    )");

    return button;
}


// =========================================================
// SECTION TITLE
// =========================================================

QLabel* HomeWindow::createSectionTitle(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet(R"(
        QLabel {
            color: #172033;
            font-size: 22px;
            font-weight: 800;
        }
    )");

    return label;
}


// =========================================================
// SEARCH
// =========================================================

void HomeWindow::handleSearch()
{
    QString searchText = searchEdit->text().trimmed();

    if (searchText.isEmpty())
    {
        QMessageBox::warning(this, "Search", "Please enter a book title, author or ISBN.");
        searchEdit->setFocus();
        return;
    }

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("search"), searchText);
    auto *reply = ApiClient::instance().getPublic(QStringLiteral("/api/books"), query);
    connect(reply, &QNetworkReply::finished, this, [this, reply, searchText]() {
        auto [ok, json, errorMsg] = ApiClient::parseReply(reply);
        reply->deleteLater();

        if (!ok || !json.contains(QStringLiteral("books"))) {
            QMessageBox::warning(this, "Search Error", errorMsg.isEmpty() ? "Search failed." : errorMsg);
            return;
        }

        auto arr = json[QStringLiteral("books")].toArray();
        if (arr.isEmpty()) {
            QMessageBox::information(this, "No Results", "No books found for:\n\n" + searchText);
            return;
        }

        QString message = QString("Found %1 book%2 for \"%3\":\n\n")
                              .arg(arr.size())
                              .arg(arr.size() == 1 ? "" : "s")
                              .arg(searchText);
        int count = 0;
        for (const auto &val : arr) {
            BookModel b = BookModel::fromJson(val.toObject());
            message += QString("%1. %2 - ₹%3\n   by %4 • %5\n\n")
                           .arg(++count)
                           .arg(b.title)
                           .arg(b.price, 0, 'f', 0)
                           .arg(b.author)
                           .arg(b.condition);
            if (count >= 10) break;
        }
        QMessageBox::information(this, "Search Results", message);
    });
}


// =========================================================
// BROWSE BOOKS
// =========================================================

void HomeWindow::handleBrowseBooks()
{
    emit browseBooksRequested();
}


// =========================================================
// SELL BOOK
// =========================================================

void HomeWindow::handleSellBook()
{
    emit sellBookRequested();
}


// =========================================================
// LISTINGS
// =========================================================

void HomeWindow::handleListings()
{
    emit listingsRequested();
}


// =========================================================
// ORDERS
// =========================================================

void HomeWindow::handleOrders()
{
    emit ordersRequested();
}

void HomeWindow::handleCart()
{
    emit cartRequested();
}

void HomeWindow::handleExchanges()
{
    emit exchangesRequested();
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


// =========================================================
// LOGOUT
// =========================================================

void HomeWindow::handleLogout()
{
    QMessageBox::StandardButton result = QMessageBox::question(
        this, "Logout", "Are you sure you want to logout?",
        QMessageBox::Yes | QMessageBox::No
        );

    if (result == QMessageBox::Yes)
    {
        auto *logoutReply = ApiClient::instance().post(QStringLiteral("/api/auth/logout"), {}, false);
        connect(logoutReply, &QNetworkReply::finished, logoutReply, &QNetworkReply::deleteLater);
        SessionManager::instance().clearSession();
        emit logoutRequested();
    }
}