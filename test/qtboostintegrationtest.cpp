#include <QtCore/QString>
#include <QtTest/QtTest>

#include "boost/lambda/lambda.hpp"
#include "boost/lambda/bind.hpp"

#include "QtBoostIntegration"

using namespace boost::lambda;

class QtBoostIntegrationTest : public QObject
{
    Q_OBJECT

public:
    QtBoostIntegrationTest();
    void called();

signals:
    void signal1();
    void signal2(int);
    void signal3(const QByteArray&);
    void signal4(unsigned int, const QString&);

private Q_SLOTS:
    void testNullary();
    void testUnary();
    void testBinary();
    void testAutoUnbind();

private:
    bool m_wasCalled;
    int m_ival;
    QByteArray m_bval;
};

QtBoostIntegrationTest::QtBoostIntegrationTest()
    : m_wasCalled(false), m_ival(0)
{
}

void QtBoostIntegrationTest::testNullary()
{
    QVERIFY(qtBoostConnect<void (void)>(this, SIGNAL(signal1()),
                this, bind(&QtBoostIntegrationTest::called, this)));
    m_wasCalled = false;
    emit signal1();
    QVERIFY(m_wasCalled);
    QVERIFY(qtBoostDisconnect(this, SIGNAL(signal1()), this));

    QVERIFY(qtBoostConnect<void (void)>(this, SIGNAL(signal1()),
                this, var(m_wasCalled) = true));
    m_wasCalled = false;
    emit signal1();
    QVERIFY(m_wasCalled);
    QVERIFY(qtBoostDisconnect(this, SIGNAL(signal1()), this));
}

void QtBoostIntegrationTest::called()
{
    m_wasCalled = true;
}

void QtBoostIntegrationTest::testUnary()
{
    QVERIFY(qtBoostConnect<void (int)>(this, SIGNAL(signal2(int)),
                this, var(m_ival) = _1));
    m_ival = -1;
    emit signal2(2);
    QVERIFY(m_ival == 2);

    QVERIFY(qtBoostConnect<void (QByteArray)>(this, SIGNAL(signal3(QByteArray)),
                0, bind(&QtBoostIntegrationTest::signal2, this, bind(&QByteArray::toInt, _1, (bool*)0, 10))));
    emit signal3("55");
    QVERIFY(m_ival == 55);

    QVERIFY(qtBoostDisconnect(this, SIGNAL(signal2(int)), this));
    QVERIFY(qtBoostDisconnect(this, SIGNAL(signal3(QByteArray)), 0));

    QVERIFY(qtBoostConnect<void (void)>(this, SIGNAL(signal2(int)), this, var(m_wasCalled) = true));
    m_wasCalled = false;
    emit signal2(1000);
    QVERIFY(m_wasCalled);
    QVERIFY(qtBoostDisconnect(this, SIGNAL(signal2(int)), this));

    QVERIFY(qtBoostConnect<void (int)>(this, SIGNAL(signal2(int)), this, var(m_wasCalled) = (_1 == 1001)));
    m_wasCalled = false;
    emit signal2(1000);
    QVERIFY(!m_wasCalled);
    emit signal2(1001);
    QVERIFY(m_wasCalled);
    QVERIFY(qtBoostDisconnect(this, SIGNAL(signal2(int)), this));
}

void QtBoostIntegrationTest::testBinary()
{
    QVERIFY(qtBoostConnect<void (uint,QString)>(this, SIGNAL(signal4(uint,QString)),
        this, var(m_wasCalled) = (_1 == bind(&QString::toUInt, _2, (bool*)0, 10))));
    m_wasCalled = false;
    emit signal4(345, QLatin1String("678"));
    QVERIFY(!m_wasCalled);
    emit signal4(123, QLatin1String("123"));
    QVERIFY(m_wasCalled);
    QVERIFY(qtBoostConnect<void (void)>(this, SIGNAL(signal4(uint,QString)),
        this, var(m_wasCalled) = false));
    emit signal4(1001, QLatin1String("1001"));
    QVERIFY(!m_wasCalled);
    QVERIFY(qtBoostDisconnect(this, SIGNAL(signal4(uint,QString)), this));
}

void QtBoostIntegrationTest::testAutoUnbind()
{
    QtBoostIntegrationTest *testObj = new QtBoostIntegrationTest;

    QVERIFY(qtBoostConnect<void (void)>(this, SIGNAL(signal1()), testObj, var(m_wasCalled) = true));
    m_wasCalled = false;
    emit signal1();
    QVERIFY(m_wasCalled);

    delete testObj;
    m_wasCalled = false;
    emit signal1();
    QVERIFY(!m_wasCalled);
}

QTEST_APPLESS_MAIN(QtBoostIntegrationTest);

#include "qtboostintegrationtest.moc"
