#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QDialog>
#include <QScrollArea>
#include <QScrollBar>
#include <iostream>

#include "signupwindow.h"
#include "loginwindow.h"
#include "homewindow.h"
#include "browsewindow.h"
#include "bookdetailswindow.h"
#include "listingswindow.h"
#include "orderswindow.h"
#include "sellbookwindow.h"
#include "windows/CartWindow.h"
#include "windows/CheckoutWindow.h"
#include "windows/OrderDetailWindow.h"
#include "windows/ExchangeWindow.h"
#include "network/ApiClient.h"
#include "auth/SessionManager.h"
#include "models/DataModels.h"
#undef private
#undef protected

static void waitMs(int ms) {
    qint64 start = QDateTime::currentMSecsSinceEpoch();
    while (QDateTime::currentMSecsSinceEpoch() - start < ms) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(20);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QString outDir = "C:/Users/Acer/.gemini/antigravity-ide/brain/992f1cdc-d189-4f69-84f4-0aa64d16e134/qt_screens";
    QDir().mkpath(outDir);

    std::cout << "Starting Qt Screen Capture Utility..." << std::endl;

    // 1. Capture Login
    {
        LoginWindow win;
        win.resize(1024, 680);
        win.show();
        waitMs(300);
        win.grab().save(outDir + "/01_login.png");
        std::cout << "Saved 01_login.png" << std::endl;
        win.close();
    }

    // 2. Capture Signup
    {
        SignupWindow win;
        win.resize(960, 680);
        win.show();
        waitMs(300);
        win.grab().save(outDir + "/02_signup.png");
        std::cout << "Saved 02_signup.png" << std::endl;
        win.close();
    }

    // Create / Log in user to view active screens
    qint64 ts = QDateTime::currentMSecsSinceEpoch();
    QString uName = QString("vis_%1").arg(ts % 100000);
    QString uEmail = QString("vis_%1@test.com").arg(ts % 100000);
    QString uPass = "Secret123!";

    // Register User
    {
        QJsonObject reg;
        reg["username"] = uName;
        reg["email"] = uEmail;
        reg["password"] = uPass;
        auto *rep = ApiClient::instance().post("/api/auth/signup", reg, false);
        while (!rep->isFinished()) { QCoreApplication::processEvents(); QThread::msleep(10); }
        rep->deleteLater();
    }

    // Login User
    QString token, userId;
    {
        QJsonObject log;
        log["email"] = uEmail;
        log["password"] = uPass;
        auto *rep = ApiClient::instance().post("/api/auth/login", log, false);
        while (!rep->isFinished()) { QCoreApplication::processEvents(); QThread::msleep(10); }
        auto [ok, json, err] = ApiClient::parseReply(rep);
        rep->deleteLater();
        token = json["accessToken"].toString();
        userId = json["user"].toObject()["id"].toString();
        SessionManager::instance().setSession(token, 900, userId, uName);
    }

    // Seed 1 listing from this user
    QString myBookId;
    {
        QJsonObject bk;
        bk["title"] = "Discrete Mathematics & Applications";
        bk["author"] = "Kenneth H. Rosen";
        bk["price"] = 520.0;
        bk["condition"] = "Like New";
        bk["category"] = "Academic";
        bk["isbn"] = "9780073383095";
        bk["description"] = "Comprehensive discrete math textbook for computer science students.";
        auto *rep = ApiClient::instance().post("/api/books", bk, true);
        while (!rep->isFinished()) { QCoreApplication::processEvents(); QThread::msleep(10); }
        auto [ok, json, err] = ApiClient::parseReply(rep);
        rep->deleteLater();
        myBookId = json["book"].toObject()["id"].toString();
    }

    // 3. Capture Home
    {
        HomeWindow win(uName);
        win.resize(1200, 850);
        win.show();
        waitMs(900);
        if (auto *sa = win.findChild<QScrollArea*>()) {
            sa->verticalScrollBar()->setValue(0);
        }
        waitMs(150);
        win.grab().save(outDir + "/03_home.png");
        std::cout << "Saved 03_home.png" << std::endl;
        win.close();
    }

    // 4. Capture Browse
    {
        BrowseWindow win(uName);
        win.resize(1200, 850);
        win.show();
        waitMs(900);
        if (auto *sa = win.findChild<QScrollArea*>()) {
            sa->verticalScrollBar()->setValue(0);
        }
        waitMs(150);
        win.grab().save(outDir + "/04_browse.png");
        std::cout << "Saved 04_browse.png" << std::endl;
        win.close();
    }

    // 5. Capture Book Details
    {
        BookDetailsWindow win(myBookId, uName);
        win.resize(1060, 780);
        win.show();
        waitMs(800);
        win.grab().save(outDir + "/05_book_details.png");
        std::cout << "Saved 05_book_details.png" << std::endl;
        win.close();
    }

    // 6. Capture Sell Book
    {
        SellBookWindow win(uName);
        win.resize(1060, 780);
        win.show();
        waitMs(300);
        win.grab().save(outDir + "/06_sell_book.png");
        std::cout << "Saved 06_sell_book.png" << std::endl;
        win.close();
    }

    // 7. Capture My Listings
    {
        ListingsWindow win(uName);
        win.resize(1120, 780);
        win.show();
        waitMs(800);
        win.grab().save(outDir + "/07_my_listings.png");
        std::cout << "Saved 07_my_listings.png" << std::endl;
        win.close();
    }

    // Seed book in cart
    // Create another user to sell a book to add to cart
    QString otherBookId;
    {
        QJsonObject u2;
        u2["username"] = uName + "_seller";
        u2["email"] = uName + "_seller@test.com";
        u2["password"] = uPass;
        auto *rep = ApiClient::instance().post("/api/auth/signup", u2, false);
        while (!rep->isFinished()) { QCoreApplication::processEvents(); QThread::msleep(10); }
        rep->deleteLater();

        auto *repL = ApiClient::instance().post("/api/auth/login", u2, false);
        while (!repL->isFinished()) { QCoreApplication::processEvents(); QThread::msleep(10); }
        auto [okL, jsonL, errL] = ApiClient::parseReply(repL);
        repL->deleteLater();
        QString t2 = jsonL["accessToken"].toString();
        QString id2 = jsonL["user"].toObject()["id"].toString();
        SessionManager::instance().setSession(t2, 900, id2, uName + "_seller");

        QJsonObject b2;
        b2["title"] = "Artificial Intelligence: Modern Approach";
        b2["author"] = "Stuart Russell & Peter Norvig";
        b2["price"] = 650.0;
        b2["condition"] = "Very Good";
        b2["category"] = "Technology";
        b2["isbn"] = "9780136042594";
        b2["description"] = "Classic AI reference textbook.";
        auto *repB = ApiClient::instance().post("/api/books", b2, true);
        while (!repB->isFinished()) { QCoreApplication::processEvents(); QThread::msleep(10); }
        auto [okB, jsonB, errB] = ApiClient::parseReply(repB);
        repB->deleteLater();
        otherBookId = jsonB["book"].toObject()["id"].toString();

        // Switch session back to original user
        SessionManager::instance().setSession(token, 900, userId, uName);

        // Add otherBook to cart
        QJsonObject cItem;
        cItem["bookId"] = otherBookId;
        auto *repC = ApiClient::instance().post("/api/cart/items", cItem, true);
        while (!repC->isFinished()) { QCoreApplication::processEvents(); QThread::msleep(10); }
        repC->deleteLater();
    }

    // 8. Capture Cart
    {
        CartWindow win;
        win.resize(1120, 780);
        win.show();
        waitMs(800);
        win.grab().save(outDir + "/08_cart.png");
        std::cout << "Saved 08_cart.png" << std::endl;
        win.close();
    }

    // 9. Capture Checkout
    {
        CheckoutWindow win;
        win.resize(1120, 800);
        win.show();
        waitMs(800);
        win.grab().save(outDir + "/09_checkout.png");
        std::cout << "Saved 09_checkout.png" << std::endl;
        win.close();
    }

    // Place an order to test Orders & Order Detail
    QString orderId;
    {
        QJsonObject ordReq;
        QJsonObject addr;
        addr["fullName"] = "Priya Sharma";
        addr["addressLine"] = "404 Academic Towers, Tech City";
        addr["city"] = "Bengaluru";
        addr["state"] = "Karnataka";
        addr["postalCode"] = "560001";
        addr["phone"] = "9876543210";
        ordReq["shippingAddress"] = addr;
        ordReq["paymentMethod"] = "UPI";

        auto *repO = ApiClient::instance().post("/api/checkout", ordReq, true);
        while (!repO->isFinished()) { QCoreApplication::processEvents(); QThread::msleep(10); }
        auto [okO, jsonO, errO] = ApiClient::parseReply(repO);
        repO->deleteLater();
        orderId = jsonO["order"].toObject()["id"].toString();
    }

    // 10. Capture Orders History
    {
        OrdersWindow win(uName);
        win.resize(1120, 780);
        win.show();
        waitMs(800);
        win.grab().save(outDir + "/10_orders.png");
        std::cout << "Saved 10_orders.png" << std::endl;
        win.close();
    }

    // 11. Capture Order Details
    {
        OrderDetailWindow win(orderId);
        win.resize(1080, 800);
        win.show();
        waitMs(800);
        win.grab().save(outDir + "/11_order_details.png");
        std::cout << "Saved 11_order_details.png" << std::endl;
        win.close();
    }

    // 12. Capture Exchanges
    {
        ExchangeWindow win;
        win.resize(1120, 780);
        win.show();
        waitMs(800);
        win.grab().save(outDir + "/12_exchanges.png");
        std::cout << "Saved 12_exchanges.png" << std::endl;
        win.close();
    }

    // 13. Capture My Reviews
    {
        ListingsWindow win(uName);
        win.handleTabChange(1); // My Reviews tab
        win.resize(1120, 780);
        win.show();
        waitMs(800);
        win.grab().save(outDir + "/13_my_reviews.png");
        std::cout << "Saved 13_my_reviews.png" << std::endl;
        win.close();
    }

    // 14. Capture Profile Popover
    {
        HomeWindow win(uName);
        win.resize(1200, 800);
        win.show();
        waitMs(400);

        QTimer::singleShot(400, [&]() {
            for (QWidget *w : QApplication::topLevelWidgets()) {
                if (auto *dlg = qobject_cast<QDialog*>(w)) {
                    dlg->grab().save(outDir + "/14_profile.png");
                    std::cout << "Saved 14_profile.png" << std::endl;
                    dlg->accept();
                    return;
                }
            }
        });
        win.showUserProfile();
        waitMs(800);
        win.close();
    }

    std::cout << "All 14 Qt screens captured successfully to: " << outDir.toStdString() << std::endl;
    return 0;
}
