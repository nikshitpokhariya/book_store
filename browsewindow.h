#ifndef BROWSEWINDOW_H
#define BROWSEWINDOW_H

#include <QWidget>
#include <QString>
#include <QList>

class QLineEdit;
class QPushButton;
class QComboBox;
class QLabel;
class QFrame;
class QGridLayout;
class QWidget;

class BrowseWindow : public QWidget
{
    Q_OBJECT

public:

    explicit BrowseWindow(
        const QString &userName,
        QWidget *parent = nullptr
        );

    ~BrowseWindow();


signals:

    void backRequested();

    void bookSelected(
        int bookId
        );


private slots:

    void handleSearch();

    void handleCategoryChanged(
        const QString &category
        );

    void handleBack();


private:

    // =====================================================
    // LOCAL BOOK STRUCTURE
    // =====================================================

    struct Book
    {
        int id = -1;

        QString title;

        QString author;

        QString condition;

        QString price;

        QString location;

        QString category;

        QString imagePath;
    };


    // =====================================================
    // UI
    // =====================================================

    void setupUI();


    QWidget* createTopBar();

    QWidget* createFilterBar();

    QWidget* createBooksSection();


    QFrame* createBookCard(
        int bookId,
        const QString &title,
        const QString &author,
        const QString &condition,
        const QString &price,
        const QString &location,
        const QString &imagePath
        );


    QPushButton* createPrimaryButton(
        const QString &text
        );


    QPushButton* createSecondaryButton(
        const QString &text
        );


    QLabel* createSectionTitle(
        const QString &text
        );


    // =====================================================
    // BOOK FUNCTIONS
    // =====================================================

    void populateBooks();

    void clearBooks();


    void filterBooks(
        const QString &searchText,
        const QString &category
        );


    // =====================================================
    // USER
    // =====================================================

    QString userName;


    // =====================================================
    // WIDGETS
    // =====================================================

    QLineEdit *searchEdit;

    QPushButton *searchButton;

    QComboBox *categoryCombo;

    QPushButton *backButton;

    QGridLayout *booksGrid;

    QWidget *booksContainer;

    QLabel *resultLabel;


    // =====================================================
    // BOOK DATA
    // =====================================================

    QList<Book> books;
};

#endif // BROWSEWINDOW_H