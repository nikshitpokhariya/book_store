#ifndef HOMEWINDOW_H
#define HOMEWINDOW_H

#include <QWidget>
#include <QString>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPixmap>

class QStackedWidget;
class QScrollArea;

class HomeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit HomeWindow(const QString &userName,
                        QWidget *parent = nullptr);

    ~HomeWindow() override;

    void refreshRecentBooks();
    void refreshCartCount();
    void updateCartBadge(int count);

    void pushPage(QWidget *page);
    void popPage();

signals:
    void browseBooksRequested(const QString &searchQuery = QString(), const QString &category = QString());
    void sellBookRequested();
    void listingsRequested();
    void ordersRequested();
    void cartRequested();
    void exchangesRequested();
    void logoutRequested();

public slots:
    void handleBrowseBooks();
    void handleBrowseWithCategory(const QString &category);
    void handleSellBook();
    void handleLogout();
    void showUserProfile();

private slots:
    void handleSearch();
    void handleListings();
    void handleOrders();
    void handleCart();
    void handleExchanges();

private:
    void setupUI();

    QWidget* createNavigationBar();
    QWidget* createHeroSection();
    QWidget* createCategorySection();
    QWidget* createRecentlyListedSection();
    QWidget* createSellSection();
    QWidget* createFooter();

    QFrame* createBookCard(
        const QString &bookId,
        const QString &title,
        const QString &author,
        const QString &condition,
        const QString &price,
        const QString &category,
        const QString &imagePath
    );

    void showBookDetails(const QString &bookId);

    QPixmap loadOrGenerateCover(
        const QString &imagePath,
        const QString &title,
        const QString &author,
        int width,
        int height
    );

    QLabel* createSectionTitle(const QString &text);

private:
    QString userName;

    QLineEdit *searchEdit;
    QPushButton *searchButton;
    QPushButton *cartButton;
    QPushButton *exchangesButton;
    QPushButton *logoutButton;
    QPushButton *profileButton;

    QVBoxLayout *recentBooksLayout;
    QStackedWidget *mainStackedWidget;
    QScrollArea *homeScrollArea;
};

#endif // HOMEWINDOW_H