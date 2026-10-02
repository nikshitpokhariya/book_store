#ifndef SELLWINDOW_H
#define SELLWINDOW_H

#include <QWidget>
#include <QString>

class QLineEdit;
class QTextEdit;
class QComboBox;
class QPushButton;
class QLabel;

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

private slots:
    void chooseImage();
    void sellBook();
    void handleBack();

private:
    void setupUI();
    QString copyImageToAppFolder(const QString &sourcePath);

    QString sellerName;

    // Form fields
    QLineEdit *titleEdit;
    QLineEdit *authorEdit;
    QLineEdit *isbnEdit;
    QComboBox *categoryCombo;
    QComboBox *conditionCombo;
    QLineEdit *priceEdit;
    QTextEdit *descriptionEdit;
    QLineEdit *locationEdit;

    // Image
    QLabel *imagePreview;
    QLabel *imagePathLabel;

    // Buttons
    QPushButton *chooseImageButton;
    QPushButton *sellButton;
    QPushButton *backButton;

    QString selectedImagePath;
};

#endif // SELLWINDOW_H