#include "test_apiclient.h"
#include <QTest>
#include <QJsonObject>
#include <QJsonDocument>
#include "network/ApiClient.h"

void TestApiClient::testSingletonInstance()
{
    ApiClient &client1 = ApiClient::instance();
    ApiClient &client2 = ApiClient::instance();
    QCOMPARE(&client1, &client2);
}

void TestApiClient::testAuthHeaderInjection()
{
    // Test base URL
    QVERIFY(!ApiClient::instance().baseUrl().isEmpty());
    QCOMPARE(ApiClient::instance().baseUrl(), QStringLiteral("http://localhost:8080"));
}
