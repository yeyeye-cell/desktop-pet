#include <QtTest>
#include "BehaviorEngine.h"

class TestBehaviorEngine : public QObject {
    Q_OBJECT

private slots:
    void testConstruct();
    void testStartStop();
    void testClickDoesNotCrash();
};

void TestBehaviorEngine::testConstruct()
{
    BehaviorEngine engine;
    QVERIFY(true);
}

void TestBehaviorEngine::testStartStop()
{
    BehaviorEngine engine;
    engine.start();
    engine.stop();
    QVERIFY(true);
}

void TestBehaviorEngine::testClickDoesNotCrash()
{
    BehaviorEngine engine;
    engine.start();
    engine.onClicked();
    engine.stop();
    QVERIFY(true);
}

QTEST_MAIN(TestBehaviorEngine)
#include "test_behavior_engine.moc"
