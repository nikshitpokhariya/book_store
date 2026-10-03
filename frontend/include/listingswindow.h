#ifndef LISTINGSWINDOW_H
#define LISTINGSWINDOW_H

#include <QWidget>
#include <QString>
#include <QVBoxLayout>
#include <QVector>
#include "models/DataModels.h"

class QLabel;
class QPushButton;
class QScrollArea;
class QStackedWidget;

class ListingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ListingsWindow(const QString &userName, QWidget *parent = nullptr);
    ~ListingsWindow() override;

    void refreshListings();
    void refreshMyReviews();

public slots:
    void handleTabChange(int tabIndex);

signals:
    void backRequested();
    void addNewListingRequested();

private slots:
    void handleBack();
    void handleAddNew();
    void handleDeleteListing(const QString &bookId, const QString &bookTitle);
    void handleEditListing(const BookModel &book);
    void handleEditReview(const ReviewModel &rev);
    void handleDeleteReview(const QString &reviewId);

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createTabsBar();
    QWidget* createStatsBar();
    QWidget* createListingCard(const BookModel &book);
    QWidget* createReviewCard(const ReviewModel &rev);

private:
    QString userName;

    QPushButton *tabListingsBtn;
    QPushButton *tabReviewsBtn;
    QPushButton *addBtn;

    QLabel *totalCountLabel;
    QLabel *availableCountLabel;
    QLabel *soldCountLabel;

    QStackedWidget *tabStack;
    QWidget *statsBarWidget;

    QVBoxLayout *cardsLayout;
    QVBoxLayout *reviewsLayout;

    int currentTab{0};
};

#endif // LISTINGSWINDOW_H
