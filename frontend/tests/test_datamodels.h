#ifndef TEST_DATAMODELS_H
#define TEST_DATAMODELS_H

#include <QObject>

class TestDataModels : public QObject
{
    Q_OBJECT

private slots:
    void testBookModelFromJson();
    void testBookModelDefaults();
    void testCartItemModelFromJson();
    void testCartModelCalculation();
    void testOrderModelFromJson();
    void testShippingAddressModelRoundtrip();
    void testExchangeModelFromJson();
    void testReviewModelFromJson();
    void testPaginationMetaFromJson();
};

#endif // TEST_DATAMODELS_H
