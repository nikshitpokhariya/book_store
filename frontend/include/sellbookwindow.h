#ifndef SELLBOOKWINDOW_H
#define SELLBOOKWINDOW_H

#include <QWidget>
#include <QString>

class QLineEdit;
class QTextEdit;
class QComboBox;
class QPushButton;
class QLabel;
class QDoubleSpinBox;

class SellBookWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SellBookWindow(
        const QString &sellerEmail,
        QWidget *parent = nullptr
    );

signals:
    void bookPublished();
    void backRequested();

private slots:
    void selectImage();
    void publishBook();
    void goBack();

private:
    void setupUI();

private:
    QString sellerEmail;

    // Image
    QLabel *imagePreview;
    QPushButton *chooseImageButton;
    QString imagePath;

    // Book information
    QLineEdit *titleEdit;
    QLineEdit *authorEdit;
    QLineEdit *isbnEdit;
    QLineEdit *editionEdit;

    QComboBox *categoryCombo;
    QComboBox *conditionCombo;

    QDoubleSpinBox *priceSpinBox;
    QTextEdit *descriptionEdit;

    QPushButton *publishButton;
    QPushButton *backButton;
};

#endif // SELLBOOKWINDOW_H