#ifndef TEST_APICLIENT_H
#define TEST_APICLIENT_H

#include <QObject>

class TestApiClient : public QObject
{
    Q_OBJECT

private slots:
    void testSingletonInstance();
    void testAuthHeaderInjection();
};

#endif // TEST_APICLIENT_H
