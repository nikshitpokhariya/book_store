#ifndef BOOKDETAILSWINDOW_H
#define BOOKDETAILSWINDOW_H

#include <QWidget>
#include <QString>

class QLabel;
class QPushButton;
class QScrollArea;
class QFrame;

class BookDetailsWindow : public QWidget
{
    Q_OBJECT

public:

    explicit BookDetailsWindow(
        int bookId,
        const QString &userName,
        QWidget *parent = nullptr
        );

    ~BookDetailsWindow() override;


signals:

    void backRequested();

    void orderPlaced();


private slots:

    void handleOrder();

    void handleBack();


private:

    // =====================================================
    // UI
    // =====================================================

    void setupUI();

    QWidget* createTopBar();

    QWidget* createDetailsSection();

    QFrame* createInfoCard(
        const QString &label,
        const QString &value
        );


    // =====================================================
    // DATABASE
    // =====================================================

    bool loadBook();


    // =====================================================
    // DATA
    // =====================================================

    int bookId;

    QString userName;


    struct BookData
    {
        int id = -1;

        QString title;
        QString author;
        QString isbn;
        QString category;
        QString condition;

        double price = 0.0;

        QString description;
        QString location;
        QString imagePath;

        QString sellerName;
        QString createdAt;
    };


    BookData book;


    // =====================================================
    // UI POINTERS
    // =====================================================

    QLabel *titleLabel;

    QLabel *authorLabel;

    QLabel *priceLabel;

    QLabel *conditionLabel;

    QLabel *categoryLabel;

    QLabel *isbnLabel;

    QLabel *locationLabel;

    QLabel *sellerLabel;

    QLabel *descriptionLabel;

    QLabel *coverLabel;

    QPushButton *orderButton;

    QPushButton *backButton;
};

#endif // BOOKDETAILSWINDOW_H