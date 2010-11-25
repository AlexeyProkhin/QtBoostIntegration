#include <QtCore/QString>
#include <QtTest/QtTest>

#include "boost/lambda/lambda.hpp"
#include "boost/lambda/bind.hpp"

#include "qtlambda.h"

using namespace boost::lambda;

class QtLambdaTest : public QObject
{
    Q_OBJECT

public:
    QtLambdaTest();
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

QtLambdaTest::QtLambdaTest()
    : m_wasCalled(false), m_ival(0)
{
}

void QtLambdaTest::testNullary()
{
    QVERIFY(qtlambda::connect<void (void)>(this, SIGNAL(signal1()),
                this, bind(&QtLambdaTest::called, this)));
    m_wasCalled = false;
    emit signal1();
    QVERIFY(m_wasCalled);
    QVERIFY(qtlambda::disconnect(this, SIGNAL(signal1()), this));

    QVERIFY(qtlambda::connect<void (void)>(this, SIGNAL(signal1()),
                this, var(m_wasCalled) = true));
    m_wasCalled = false;
    emit signal1();
    QVERIFY(m_wasCalled);
    QVERIFY(qtlambda::disconnect(this, SIGNAL(signal1()), this));
}

void QtLambdaTest::called()
{
    m_wasCalled = true;
}

void QtLambdaTest::testUnary()
{
    QVERIFY(qtlambda::connect<void (int)>(this, SIGNAL(signal2(int)),
                this, var(m_ival) = _1));
    m_ival = -1;
    emit signal2(2);
    QVERIFY(m_ival == 2);

    QVERIFY(qtlambda::connect<void (QByteArray)>(this, SIGNAL(signal3(QByteArray)),
                0, bind(&QtLambdaTest::signal2, this, bind(&QByteArray::toInt, _1, (bool*)0, 10))));
    emit signal3("55");
    QVERIFY(m_ival == 55);

    QVERIFY(qtlambda::disconnect(this, SIGNAL(signal2(int)), this));
    QVERIFY(qtlambda::disconnect(this, SIGNAL(signal3(QByteArray)), 0));

    QVERIFY(qtlambda::connect<void (void)>(this, SIGNAL(signal2(int)), this, var(m_wasCalled) = true));
    m_wasCalled = false;
    emit signal2(1000);
    QVERIFY(m_wasCalled);
    QVERIFY(qtlambda::disconnect(this, SIGNAL(signal2(int)), this));

    QVERIFY(qtlambda::connect<void (int)>(this, SIGNAL(signal2(int)), this, var(m_wasCalled) = (_1 == 1001)));
    m_wasCalled = false;
    emit signal2(1000);
    QVERIFY(!m_wasCalled);
    emit signal2(1001);
    QVERIFY(m_wasCalled);
    QVERIFY(qtlambda::disconnect(this, SIGNAL(signal2(int)), this));
}

void QtLambdaTest::testBinary()
{
    QVERIFY(qtlambda::connect<void (uint,QString)>(this, SIGNAL(signal4(uint,QString)),
        this, var(m_wasCalled) = (_1 == bind(&QString::toUInt, _2, (bool*)0, 10))));
    m_wasCalled = false;
    emit signal4(345, QLatin1String("678"));
    QVERIFY(!m_wasCalled);
    emit signal4(123, QLatin1String("123"));
    QVERIFY(m_wasCalled);
    QVERIFY(qtlambda::connect<void (void)>(this, SIGNAL(signal4(uint,QString)),
        this, var(m_wasCalled) = false));
    emit signal4(1001, QLatin1String("1001"));
    QVERIFY(!m_wasCalled);
    QVERIFY(qtlambda::disconnect(this, SIGNAL(signal4(uint,QString)), this));
}

void QtLambdaTest::testAutoUnbind()
{
    QtLambdaTest *testObj = new QtLambdaTest;

    QVERIFY(qtlambda::connect<void (void)>(this, SIGNAL(signal1()), testObj, var(m_wasCalled) = true));
    m_wasCalled = false;
    emit signal1();
    QVERIFY(m_wasCalled);

    delete testObj;
    m_wasCalled = false;
    emit signal1();
    QVERIFY(!m_wasCalled);
}

QTEST_APPLESS_MAIN(QtLambdaTest);

#include "tst_qtlambdatest.moc"
