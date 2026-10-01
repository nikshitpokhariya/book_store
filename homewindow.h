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

// Forward declaration
class Database;

class HomeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit HomeWindow(const QString &userName,
                        QWidget *parent = nullptr);

    ~HomeWindow();

    // Dynamically reloads the recent books section from the database
    void refreshRecentBooks();

signals:
    void browseBooksRequested();
    void sellBookRequested();
    void listingsRequested();
    void ordersRequested();
    void logoutRequested();

private slots:
    void handleSearch();
    void handleBrowseBooks();
    void handleSellBook();
    void handleListings();
    void handleOrders();
    void handleLogout();

private:
    // Main UI
    void setupUI();

    // Sections
    QWidget* createNavigationBar();
    QWidget* createHeroSection();
    QWidget* createCategorySection();
    QWidget* createRecentlyListedSection();
    QWidget* createSellSection();
    QWidget* createWhySection();
    QWidget* createFooter();

    // Book Cards & Details
    QFrame* createBookCard(
        int bookId,
        const QString &title,
        const QString &author,
        const QString &condition,
        const QString &price,
        const QString &location,
        const QString &imagePath
        );

    void showBookDetails(int bookId);

    // Cover art - either loads a real image, or generates a
    // professional-looking placeholder cover when none exists yet.
    QPixmap loadOrGenerateCover(
        const QString &imagePath,
        const QString &title,
        const QString &author,
        int width,
        int height
        );

    // Helpers
    QPushButton* createPrimaryButton(const QString &text);
    QPushButton* createSecondaryButton(const QString &text);
    QLabel* createSectionTitle(const QString &text);
    void applyElevation(QWidget *widget, int blurRadius = 30,
                        int yOffset = 8, int alpha = 35);

private:
    QString userName;

    QLineEdit *searchEdit;
    QPushButton *searchButton;
    QPushButton *logoutButton;

    QVBoxLayout *recentBooksLayout;
};

#endif // HOMEWINDOW_H