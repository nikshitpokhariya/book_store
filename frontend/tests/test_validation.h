#ifndef TEST_VALIDATION_H
#define TEST_VALIDATION_H

#include <QObject>

class TestValidation : public QObject
{
    Q_OBJECT

private slots:
    void testUsernameValidation();
    void testPasswordValidation();
    void testEmailValidation();
    void testShippingAddressValidation();
    void testBookPricingAndCondition();
};

#endif // TEST_VALIDATION_H
