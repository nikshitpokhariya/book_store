#ifndef LISTINGSWINDOW_H
#define LISTINGSWINDOW_H

#include <QWidget>
#include <QString>
#include <QVBoxLayout>

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
    void handleDeleteListing(int bookId, const QString &bookTitle);

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createStatsBar();
    QWidget* createListingCard(int bookId, const QString &title, const QString &author,
                               const QString &category, const QString &condition,
                               double price, const QString &status, const QString &imagePath);

private:
    QString userName;

    QLabel *totalCountLabel;
    QLabel *availableCountLabel;
    QLabel *soldCountLabel;

    QVBoxLayout *cardsLayout;
};

#endif // LISTINGSWINDOW_H
