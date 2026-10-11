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
class QStackedWidget;
class VideoPlayerWidget;

struct GalleryMediaItem {
    bool isVideo{false};
    QString url;
};

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
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

signals:
    void backRequested();
    void orderPlaced();
    void addToCartRequested(const QString &bookId);
    void exchangeRequested(const QString &bookId);

public slots:
    void handleProposeExchange();
    void showPreviousMedia();
    void showNextMedia();

private slots:
    void handleOrder();
    void handleBack();
    void selectGalleryMedia(int index);
    void handlePlayVideo();
    void handleSetRating(int stars);
    void handleSubmitReview();
    void openImageInspection();

private:
    void setupUI();
    void setupShortcuts();
    QWidget* createTopBar();
    QWidget* createDetailsSection();
    QWidget* createReviewsSection();
    QFrame* createInfoCard(const QString &label, const QString &value);
    void updateGalleryUI();
    void updatePreviewOverlayPositions();
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
    QVector<GalleryMediaItem> mediaItems_;

    QFrame *previewBox_{nullptr};
    QStackedWidget *mediaStack_{nullptr};
    QLabel *coverLabel{nullptr};
    VideoPlayerWidget *videoPlayer_{nullptr};
    QPushButton *prevMediaBtn_{nullptr};
    QPushButton *nextMediaBtn_{nullptr};
    QPushButton *inspectBtn_{nullptr};
    QLabel *mediaCounterBadge_{nullptr};

    QPushButton *playVideoBtn{nullptr};
    QScrollArea *thumbScroll_{nullptr};
    QHBoxLayout *thumbnailsLayout{nullptr};
    QVector<QPushButton*> thumbnailBtns;

    // Book Info
    QLabel *titleLabel{nullptr};
    QLabel *authorLabel{nullptr};
    QLabel *priceLabel{nullptr};
    QLabel *conditionLabel{nullptr};
    QLabel *categoryLabel{nullptr};
    QLabel *isbnLabel{nullptr};
    QLabel *sellerLabel{nullptr};
    QLabel *ratingLabel{nullptr};
    QLabel *descriptionLabel{nullptr};

    // Reviews & Buyer check
    bool hasPurchased_{false};
    QString buyerOrderId_;
    int selectedRating_{5};

    QFrame *reviewFormCard_{nullptr};
    QLabel *buyerNoticeLabel_{nullptr};
    QVector<QPushButton*> starButtons_;
    QTextEdit *reviewCommentEdit_{nullptr};
    QPushButton *submitReviewBtn_{nullptr};
    QVBoxLayout *reviewsLayout{nullptr};

    // Action buttons
    QPushButton *orderButton{nullptr};
    QPushButton *exchangeButton{nullptr};
    QPushButton *backButton{nullptr};
};

#endif // BOOKDETAILSWINDOW_H