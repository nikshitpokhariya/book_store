#include <QTest>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <iostream>

#include "test_datamodels.h"
#include "test_sessionmanager.h"
#include "test_validation.h"
#include "test_apiclient.h"

static int runSuite(QObject *suite, const QString &name, const QString &logFile)
{
    QStringList args;
    args << "test" << "-o" << QString("%1,txt").arg(logFile);
    
    int status = QTest::qExec(suite, args);

    QFile file(logFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        std::cout << in.readAll().toStdString() << std::endl;
        file.close();
        file.remove();
    }
    return status;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    int status = 0;

    std::cout << "========================================================" << std::endl;
    std::cout << "   RUNNING BOOKBAZZAR QT6 FRONTEND TEST SUITE (PHASE 2) " << std::endl;
    std::cout << "========================================================" << std::endl;

    {
        TestDataModels tc;
        status |= runSuite(&tc, "TestDataModels", "test_models.log");
    }
    {
        TestSessionManager tc;
        status |= runSuite(&tc, "TestSessionManager", "test_session.log");
    }
    {
        TestValidation tc;
        status |= runSuite(&tc, "TestValidation", "test_validation.log");
    }
    {
        TestApiClient tc;
        status |= runSuite(&tc, "TestApiClient", "test_apiclient.log");
    }

    std::cout << "========================================================" << std::endl;
    std::cout << "   ALL QT6 SUITES COMPLETE. FINAL EXIT CODE: " << status << std::endl;
    std::cout << "========================================================" << std::endl;

    return status;
}
