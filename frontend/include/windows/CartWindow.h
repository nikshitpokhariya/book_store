#ifndef CARTWINDOW_H
#define CARTWINDOW_H

#include <QWidget>
#include <QString>
#include <QVector>
#include "models/DataModels.h"

class QVBoxLayout;
class QLabel;
class QPushButton;
class QScrollArea;
class QFrame;

class CartWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CartWindow(QWidget *parent = nullptr);
    ~CartWindow() override = default;

    void refreshCart();

signals:
    void checkoutRequested();
    void backRequested();
    void browseRequested();
    void cartCountChanged(int count);

private slots:
    void handleRemoveItem(const QString &bookId);
    void handleClearCart();
    void handleCheckout();

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createSummaryCard();
    QFrame* createItemCard(const CartItemModel &item);
    QPixmap loadOrGenerateCover(const QString &imagePath, const QString &title, const QString &author, int w, int h);
    void updateEmptyState();

private:
    CartModel cart;

    QVBoxLayout *itemsLayout;
    QWidget *itemsContainer;
    QWidget *emptyStateWidget;
    QScrollArea *scrollArea;

    // Summary Card elements
    QLabel *itemCountLabel;
    QLabel *subtotalLabel;
    QLabel *shippingLabel;
    QLabel *totalLabel;
    QLabel *unavailableWarningLabel;
    QPushButton *checkoutButton;
    QPushButton *clearCartButton;
};

#endif // CARTWINDOW_H
