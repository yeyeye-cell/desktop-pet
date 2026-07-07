#include <QtTest>
#include "AnimationEngine.h"
#include "SpriteData.h"

class TestAnimationEngine : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testStateChange();
    void testEmptySpriteData();
};

void TestAnimationEngine::testInitialState()
{
    AnimationEngine engine;
    QVERIFY(engine.currentFrame().isNull());
}

void TestAnimationEngine::testStateChange()
{
    AnimationEngine engine;

    SpriteData data;
    data.name = "test";
    data.frameRate = 50;
    QPixmap px(32, 32);
    px.fill(Qt::red);
    data.frames[PetState::Idle] = {px};
    data.frames[PetState::Walking] = {px};

    engine.setSpriteData(&data);
    engine.setState(PetState::Idle);
    QVERIFY(!engine.currentFrame().isNull());
}

void TestAnimationEngine::testEmptySpriteData()
{
    AnimationEngine engine;
    engine.setState(PetState::Walking);
    // Should not crash with null sprite data
    QVERIFY(engine.currentFrame().isNull());
}

QTEST_MAIN(TestAnimationEngine)
#include "test_animation_engine.moc"
