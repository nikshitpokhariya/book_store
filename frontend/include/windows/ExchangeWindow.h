#ifndef EXCHANGEWINDOW_H
#define EXCHANGEWINDOW_H

#include <QWidget>
#include <QString>
#include <QVector>
#include "models/DataModels.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class QScrollArea;
class QFrame;

class ExchangeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ExchangeWindow(QWidget *parent = nullptr);
    ~ExchangeWindow() override = default;

    void refreshExchanges();

signals:
    void backRequested();

public slots:
    void handleTabChange(int tabIndex);

private slots:
    void handleNextPage();
    void handlePrevPage();
    void handleAccept(const QString &exchangeId);
    void handleReject(const QString &exchangeId);
    void handleCancel(const QString &exchangeId);

private:
    void setupUI();
    QWidget* createTopBar();
    QWidget* createTabBar();
    QWidget* createPaginationBar();
    QFrame* createExchangeCard(const ExchangeModel &exchange);
    QWidget* createBookSnapshotWidget(const QString &heading, const ExchangeBookSnapshot &book);
    QPixmap loadOrGenerateCover(const QString &imagePath, const QString &title, const QString &author, int w, int h);

private:
    int currentTab{0}; // 0 = Received, 1 = Sent, 2 = History
    int currentPage{1};
    PaginationMeta pagination;

    QPushButton *receivedTabBtn;
    QPushButton *sentTabBtn;
    QPushButton *historyTabBtn;

    QVBoxLayout *exchangesLayout;
    QWidget *emptyStateWidget;

    QPushButton *prevPageBtn;
    QPushButton *nextPageBtn;
    QLabel *pageIndicatorLabel;
};

#endif // EXCHANGEWINDOW_H
