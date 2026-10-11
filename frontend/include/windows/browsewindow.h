#ifndef BROWSEWINDOW_H
#define BROWSEWINDOW_H

#include <QWidget>
#include <QString>
#include <QVector>
#include "models/DataModels.h"

class QLineEdit;
class QPushButton;
class QComboBox;
class QLabel;
class QFrame;
class QGridLayout;
class QDoubleSpinBox;

class BrowseWindow : public QWidget
{
    Q_OBJECT

public:
    explicit BrowseWindow(
        const QString &userName,
        const QString &initialCategory = QString(),
        const QString &initialSearch = QString(),
        QWidget *parent = nullptr
    );

    ~BrowseWindow() override;

    void fetchBooks();

signals:
    void backRequested();
    void bookSelected(const QString &bookId);

private slots:
    void handleSearch();
    void handleFilterChanged();
    void handleResetFilters();
    void handlePrevPage();
    void handleNextPage();
    void handleBack();
    void handleSearchTextChanged(const QString &text);
    void fetchSuggestions();
    void handleSuggestionSelected(int row);

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createFilterBar();
    QWidget* createBooksSection();
    QWidget* createPaginationBar();

    QFrame* createBookCard(const BookModel &book);
    void clearBooksGrid();

private:
    QString userName;

    QLineEdit *searchEdit;
    QPushButton *searchButton;
    class QListWidget *suggestionsList;
    class QTimer *debounceTimer;
    QComboBox *categoryCombo;
    QComboBox *conditionCombo;
    QComboBox *sortCombo;
    QDoubleSpinBox *minPriceSpin;
    QDoubleSpinBox *maxPriceSpin;
    QPushButton *resetFilterButton;
    QPushButton *backButton;

    QPushButton *prevPageButton;
    QPushButton *nextPageButton;
    QLabel *pageLabel;

    QGridLayout *booksGrid;
    QWidget *booksContainer;
    QLabel *resultLabel;

    QVector<BookModel> books;
    int currentPage{1};
    int totalPages{1};
};

#endif // BROWSEWINDOW_H