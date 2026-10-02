#ifndef BOOKDETAILSWINDOW_H
#define BOOKDETAILSWINDOW_H

#include <QWidget>
#include <QString>
#include <QVector>
#include "models/DataModels.h"

class QLabel;
class QPushButton;
class QScrollArea;
class QFrame;
class QVBoxLayout;

class BookDetailsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit BookDetailsWindow(
        const QString &bookId,
        const QString &userName = QString(),
        QWidget *parent = nullptr
    );

    ~BookDetailsWindow() override;

    void loadBook();
    void loadReviews();

signals:
    void backRequested();
    void orderPlaced();
    void addToCartRequested(const QString &bookId);

public slots:
    void handleProposeExchange();

private slots:
    void handleOrder();
    void handleBack();

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createDetailsSection();
    QWidget* createReviewsSection();
    QFrame* createInfoCard(const QString &label, const QString &value);

private:
    QString bookId;
    QString userName;
    BookModel book;
    QVector<ReviewModel> reviews;

    QLabel *titleLabel;
    QLabel *authorLabel;
    QLabel *priceLabel;
    QLabel *conditionLabel;
    QLabel *categoryLabel;
    QLabel *isbnLabel;
    QLabel *sellerLabel;
    QLabel *ratingLabel;
    QLabel *descriptionLabel;
    QLabel *coverLabel;

    QVBoxLayout *reviewsLayout;

    QPushButton *orderButton;
    QPushButton *exchangeButton;
    QPushButton *backButton;
};

#endif // BOOKDETAILSWINDOW_H