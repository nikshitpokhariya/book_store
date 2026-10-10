#include "test_sessionmanager.h"
#include <QTest>
#include "auth/SessionManager.h"

void TestSessionManager::init()
{
    SessionManager::instance().clearSession();
}

void TestSessionManager::cleanup()
{
    SessionManager::instance().clearSession();
}

void TestSessionManager::testInitialLoggedOutState()
{
    QVERIFY(!SessionManager::instance().isLoggedIn());
    QVERIFY(SessionManager::instance().accessToken().isEmpty());
    QVERIFY(SessionManager::instance().userId().isEmpty());
    QVERIFY(SessionManager::instance().username().isEmpty());
    QVERIFY(SessionManager::instance().isTokenExpired());
}

void TestSessionManager::testSetSession()
{
    const QString fakeJwt = QStringLiteral("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.testpayload.signature");
    const QString fakeUserId = QStringLiteral("60d5ec49f1b2c8b1f8e4e1a1");
    const QString fakeUsername = QStringLiteral("testuser");
    const int expiresInSeconds = 900; // 15 mins

    SessionManager::instance().setSession(fakeJwt, expiresInSeconds, fakeUserId, fakeUsername);

    QVERIFY(SessionManager::instance().isLoggedIn());
    QCOMPARE(SessionManager::instance().accessToken(), fakeJwt);
    QCOMPARE(SessionManager::instance().userId(), fakeUserId);
    QCOMPARE(SessionManager::instance().username(), fakeUsername);
    QVERIFY(!SessionManager::instance().isTokenExpired());
}

void TestSessionManager::testClearSession()
{
    SessionManager::instance().setSession(QStringLiteral("tok123"), 900, QStringLiteral("u1"), QStringLiteral("user1"));
    QVERIFY(SessionManager::instance().isLoggedIn());

    SessionManager::instance().clearSession();

    QVERIFY(!SessionManager::instance().isLoggedIn());
    QVERIFY(SessionManager::instance().accessToken().isEmpty());
    QVERIFY(SessionManager::instance().userId().isEmpty());
    QVERIFY(SessionManager::instance().username().isEmpty());
    QVERIFY(SessionManager::instance().isTokenExpired());
}

void TestSessionManager::testTokenExpiration()
{
    // If expires in 10 seconds, with 60s buffer, it should register as expired
    SessionManager::instance().setSession(QStringLiteral("tok123"), 10, QStringLiteral("u1"), QStringLiteral("user1"));
    QVERIFY(SessionManager::instance().isTokenExpired());
}
