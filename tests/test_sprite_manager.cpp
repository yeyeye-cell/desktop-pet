#include <QtTest>
#include <QTemporaryDir>
#include <QImage>
#include <QDir>
#include <QFile>
#include "SpriteManager.h"
#include "SpriteData.h"

class TestSpriteManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testEmptyOnStartup();
    void testLoadPresets();
    void testSwitchPet();

private:
    QTemporaryDir m_tempDir;
};

void TestSpriteManager::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

void TestSpriteManager::testEmptyOnStartup()
{
    SpriteManager mgr;
    QVERIFY(mgr.current() == nullptr);
    QVERIFY(mgr.allPets().isEmpty());
}

void TestSpriteManager::testLoadPresets()
{
    // Create a fake preset structure
    QString presetDir = m_tempDir.path() + "/presets";
    QString catDir = presetDir + "/cat";
    QDir().mkpath(catDir + "/idle");
    QDir().mkpath(catDir + "/walk");

    // Create pet.json
    QFile jsonFile(catDir + "/pet.json");
    bool opened = jsonFile.open(QIODevice::WriteOnly);
    QVERIFY(opened);
    jsonFile.write(R"({"name":"TestCat","frameRate":120})");
    jsonFile.close();

    // Create sample frame images (2px red squares)
    QImage redSquare(32, 32, QImage::Format_ARGB32);
    redSquare.fill(Qt::red);
    redSquare.save(catDir + "/idle/idle_001.png");
    redSquare.save(catDir + "/idle/idle_002.png");
    redSquare.save(catDir + "/walk/walk_001.png");

    QString extraDir = presetDir + "/test";
    QDir().mkpath(extraDir + "/idle");
    redSquare.save(extraDir + "/idle/idle_001.png");

    SpriteManager mgr;
    mgr.loadPresets(presetDir);

    QCOMPARE(mgr.allPets().size(), 1);
    QVERIFY(mgr.current() != nullptr);
    QCOMPARE(mgr.current()->name, QString("TestCat"));
    QCOMPARE(mgr.current()->frameRate, 120);
    QVERIFY(mgr.current()->frames.contains(PetState::Idle));
    QCOMPARE(mgr.current()->frames[PetState::Idle].size(), 2);
    QVERIFY(mgr.current()->frames.contains(PetState::Walking));
    QCOMPARE(mgr.current()->frames[PetState::Walking].size(), 1);
}

void TestSpriteManager::testSwitchPet()
{
    const QString presetDir = m_tempDir.path() + "/switch-presets";
    const QStringList names = {"cat", "dog", "bird"};
    QImage frame(16, 16, QImage::Format_ARGB32);
    frame.fill(Qt::green);
    for (const QString &name : names) {
        const QString idleDir = presetDir + "/" + name + "/idle";
        QDir().mkpath(idleDir);
        QVERIFY(frame.save(idleDir + "/idle_001.png"));
    }

    SpriteManager mgr;
    mgr.loadPresets(presetDir);
    QCOMPARE(mgr.allPets().size(), 3);
    QCOMPARE(mgr.current()->name, QString("bird"));
    QCOMPARE(mgr.currentIndex(), 0);

    QVERIFY(mgr.switchTo("dog"));
    QCOMPARE(mgr.current()->name, QString("dog"));
    QCOMPARE(mgr.currentIndex(), 2);

    QVERIFY(mgr.switchTo(0));
    QCOMPARE(mgr.current()->name, QString("bird"));

    QVERIFY(!mgr.switchTo("nonexistent"));
    QVERIFY(!mgr.switchTo(99));
}

QTEST_MAIN(TestSpriteManager)
#include "test_sprite_manager.moc"
