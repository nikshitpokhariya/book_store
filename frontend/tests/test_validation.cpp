#include "test_validation.h"
#include <QTest>
#include <QRegularExpression>

void TestValidation::testUsernameValidation()
{
    auto isValidUsername = [](const QString &u) -> bool {
        if (u.length() < 3 || u.length() > 30) return false;
        static const QRegularExpression regex(QStringLiteral("^[a-zA-Z0-9_]+$"));
        return regex.match(u).hasMatch();
    };

    QVERIFY(isValidUsername(QStringLiteral("alice")));
    QVERIFY(isValidUsername(QStringLiteral("bob_123")));
    QVERIFY(isValidUsername(QStringLiteral("user_with_30_characters_here_")));

    // Rejections
    QVERIFY(!isValidUsername(QStringLiteral("ab"))); // Too short (<3)
    QVERIFY(!isValidUsername(QStringLiteral("this_username_is_way_too_long_and_exceeds_thirty_chars"))); // Too long (>30)
    QVERIFY(!isValidUsername(QStringLiteral("user@name"))); // Special chars not allowed
    QVERIFY(!isValidUsername(QStringLiteral("user name"))); // Spaces not allowed
}

void TestValidation::testPasswordValidation()
{
    auto isValidPassword = [](const QString &p) -> bool {
        return p.length() >= 8 && p.length() <= 128;
    };

    QVERIFY(isValidPassword(QStringLiteral("password123")));
    QVERIFY(isValidPassword(QStringLiteral("12345678"))); // Boundary: exactly 8 chars

    // Rejections
    QVERIFY(!isValidPassword(QStringLiteral("short"))); // <8 chars
    QVERIFY(!isValidPassword(QStringLiteral("1234567"))); // 7 chars
}

void TestValidation::testEmailValidation()
{
    auto isValidEmail = [](const QString &e) -> bool {
        static const QRegularExpression regex(QStringLiteral("^[^@\\s]+@[^@\\s]+\\.[^@\\s]+$"));
        return regex.match(e).hasMatch();
    };

    QVERIFY(isValidEmail(QStringLiteral("user@example.com")));
    QVERIFY(isValidEmail(QStringLiteral("john.doe@sub.domain.org")));

    // Rejections
    QVERIFY(!isValidEmail(QStringLiteral("notanemail")));
    QVERIFY(!isValidEmail(QStringLiteral("user@")));
    QVERIFY(!isValidEmail(QStringLiteral("@example.com")));
    QVERIFY(!isValidEmail(QStringLiteral("user@domain")));
}

void TestValidation::testShippingAddressValidation()
{
    auto validateAddress = [](const QString &name, const QString &addr, const QString &city,
                              const QString &state, const QString &pin, const QString &phone) -> bool {
        if (name.trimmed().isEmpty() || name.trimmed().length() > 100) return false;
        if (addr.trimmed().isEmpty() || addr.trimmed().length() > 200) return false;
        if (city.trimmed().isEmpty() || city.trimmed().length() > 50) return false;
        if (state.trimmed().isEmpty() || state.trimmed().length() > 50) return false;
        if (pin.trimmed().isEmpty() || pin.trimmed().length() > 20) return false;
        if (phone.trimmed().isEmpty() || phone.trimmed().length() > 20) return false;
        return true;
    };

    QVERIFY(validateAddress(QStringLiteral("Alice Smith"), QStringLiteral("123 Main St"),
                            QStringLiteral("Delhi"), QStringLiteral("Delhi"),
                            QStringLiteral("110001"), QStringLiteral("9876543210")));

    // Rejections
    QVERIFY(!validateAddress(QString(), QStringLiteral("123 Main St"), QStringLiteral("Delhi"),
                             QStringLiteral("Delhi"), QStringLiteral("110001"), QStringLiteral("9876543210"))); // Missing name
    QVERIFY(!validateAddress(QStringLiteral("Alice"), QString(), QStringLiteral("Delhi"),
                             QStringLiteral("Delhi"), QStringLiteral("110001"), QStringLiteral("9876543210"))); // Missing addr
    QVERIFY(!validateAddress(QStringLiteral("Alice"), QStringLiteral("123 Main St"), QStringLiteral("Delhi"),
                             QStringLiteral("Delhi"), QStringLiteral("110001"), QString())); // Missing phone
}

void TestValidation::testBookPricingAndCondition()
{
    auto isValidCondition = [](const QString &cond) -> bool {
        static const QStringList valid = {
            QStringLiteral("New"), QStringLiteral("Like New"), QStringLiteral("Very Good"),
            QStringLiteral("Good"), QStringLiteral("Acceptable")
        };
        return valid.contains(cond);
    };

    QVERIFY(isValidCondition(QStringLiteral("New")));
    QVERIFY(isValidCondition(QStringLiteral("Like New")));
    QVERIFY(isValidCondition(QStringLiteral("Very Good")));
    QVERIFY(isValidCondition(QStringLiteral("Good")));
    QVERIFY(isValidCondition(QStringLiteral("Acceptable")));
    QVERIFY(!isValidCondition(QStringLiteral("Mint"))); // Invalid
    QVERIFY(!isValidCondition(QStringLiteral("Poor"))); // Invalid

    auto isValidPrice = [](double price) -> bool {
        return price >= 0.0 && price <= 1000000.0;
    };

    QVERIFY(isValidPrice(0.0)); // Free/exchange
    QVERIFY(isValidPrice(299.0));
    QVERIFY(isValidPrice(999999.0));
    QVERIFY(!isValidPrice(-5.0)); // Negative price rejected
    QVERIFY(!isValidPrice(1500000.0)); // Excessive price rejected
}
