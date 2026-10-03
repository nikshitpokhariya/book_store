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
class QHBoxLayout;
class QTextEdit;

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

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;


signals:
    void backRequested();
    void orderPlaced();
    void addToCartRequested(const QString &bookId);

public slots:
    void handleProposeExchange();

private slots:
    void handleOrder();
    void handleBack();
    void selectGalleryMedia(int index);
    void handlePlayVideo();
    void handleSetRating(int stars);
    void handleSubmitReview();

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createDetailsSection();
    QWidget* createReviewsSection();
    QFrame* createInfoCard(const QString &label, const QString &value);
    void updateGalleryUI();
    void checkBuyerStatus();
    void renderReviewsList(const QVector<ReviewModel> &sellerReviews, double sellerAvgRating, int sellerReviewCount);

private:
    QString bookId;
    QString userName;
    BookModel book;
    QVector<ReviewModel> reviews;

    // Gallery state
    int currentMediaIndex_{0};
    QString currentVideoUrl_;
    QLabel *coverLabel;
    QPushButton *playVideoBtn;
    QHBoxLayout *thumbnailsLayout;
    QVector<QPushButton*> thumbnailBtns;

    // Book Info
    QLabel *titleLabel;
    QLabel *authorLabel;
    QLabel *priceLabel;
    QLabel *conditionLabel;
    QLabel *categoryLabel;
    QLabel *isbnLabel;
    QLabel *sellerLabel;
    QLabel *ratingLabel;
    QLabel *descriptionLabel;

    // Reviews & Buyer check
    bool hasPurchased_{false};
    QString buyerOrderId_;
    int selectedRating_{5};

    QFrame *reviewFormCard_{nullptr};
    QLabel *buyerNoticeLabel_{nullptr};
    QVector<QPushButton*> starButtons_;
    QTextEdit *reviewCommentEdit_{nullptr};
    QPushButton *submitReviewBtn_{nullptr};
    QVBoxLayout *reviewsLayout;

    // Action buttons
    QPushButton *orderButton;
    QPushButton *exchangeButton;
    QPushButton *backButton;
};

#endif // BOOKDETAILSWINDOW_H