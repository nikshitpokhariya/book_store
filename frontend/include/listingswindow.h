#ifndef LISTINGSWINDOW_H
#define LISTINGSWINDOW_H

#include <QWidget>
#include <QString>
#include <QVBoxLayout>
#include "models/DataModels.h"

class QLabel;
class QPushButton;
class QScrollArea;

class ListingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ListingsWindow(const QString &userName, QWidget *parent = nullptr);
    ~ListingsWindow() override;

    void refreshListings();

signals:
    void backRequested();
    void addNewListingRequested();

private slots:
    void handleBack();
    void handleAddNew();
    void handleDeleteListing(const QString &bookId, const QString &bookTitle);
    void handleEditListing(const BookModel &book);

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createStatsBar();
    QWidget* createListingCard(const BookModel &book);

private:
    QString userName;

    QLabel *totalCountLabel;
    QLabel *availableCountLabel;
    QLabel *soldCountLabel;

    QVBoxLayout *cardsLayout;
};

#endif // LISTINGSWINDOW_H
