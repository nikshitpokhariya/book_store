#ifndef SELLWINDOW_H
#define SELLWINDOW_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QVector>

class QLineEdit;
class QTextEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QFrame;

class SellWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SellWindow(
        const QString &sellerName,
        QWidget *parent = nullptr
    );

    ~SellWindow() override;

signals:
    void backRequested();
    void bookAdded();
    void bookPublished();

private slots:
    void chooseImageSlot(int index);
    void chooseMultipleImages();
    void removeImage(int index);
    void chooseVideo();
    void removeVideo();
    void sellBook();
    void handleBack();

private:
    void setupUI();
    void updateImageSlotsUI();
    void updateVideoUI();
    QString copyMediaToUploads(const QString &sourcePath);

    QString sellerName;

    // Form fields
    QLineEdit *titleEdit;
    QLineEdit *authorEdit;
    QLineEdit *isbnEdit;
    QComboBox *categoryCombo;
    QComboBox *conditionCombo;
    QLineEdit *priceEdit;
    QTextEdit *descriptionEdit;

    // Multi-Image & Video state
    QStringList selectedImagePaths; // max 6, min 4
    QString selectedVideoPath;      // optional
    int selectedThumbnailIndex{0};

    // Image slot UI elements
    QLabel *photoCountBadge;
    struct ImageSlotUI {
        QFrame *frame{nullptr};
        QLabel *previewLabel{nullptr};
        QLabel *titleLabel{nullptr};
        QPushButton *actionButton{nullptr};
        QPushButton *thumbnailButton{nullptr};
    };
    QVector<ImageSlotUI> imageSlots;


    // Video UI elements
    QFrame *videoCard;
    QLabel *videoStatusLabel;
    QPushButton *chooseVideoBtn;
    QPushButton *removeVideoBtn;

    // Action buttons
    QPushButton *chooseAllImagesBtn;
    QPushButton *sellButton;
    QPushButton *backButton;
};

#endif // SELLWINDOW_H