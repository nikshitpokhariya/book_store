#ifndef TEST_SESSIONMANAGER_H
#define TEST_SESSIONMANAGER_H

#include <QObject>

class TestSessionManager : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void testInitialLoggedOutState();
    void testSetSession();
    void testClearSession();
    void testTokenExpiration();
};

#endif // TEST_SESSIONMANAGER_H
