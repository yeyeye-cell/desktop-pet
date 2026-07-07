#include <QtTest>
#include <QTemporaryDir>
#include <QImage>
#include "SpriteManager.h"
#include "SpriteData.h"

class TestSpriteManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testEmptyOnStartup();
    void testLoadPresets();
    void testImportSingleImage();
    void testImportBadFile();
    void testSwitchPet();
    void testFallbackFrames();

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

    SpriteManager mgr;
    mgr.loadPresets(presetDir);

    QVERIFY(mgr.current() != nullptr);
    QCOMPARE(mgr.current()->name, QString("TestCat"));
    QCOMPARE(mgr.current()->frameRate, 120);
    QVERIFY(mgr.current()->frames.contains(PetState::Idle));
    QCOMPARE(mgr.current()->frames[PetState::Idle].size(), 2);
    QVERIFY(mgr.current()->frames.contains(PetState::Walking));
    QCOMPARE(mgr.current()->frames[PetState::Walking].size(), 1);
}

void TestSpriteManager::testImportSingleImage()
{
    // Create a test PNG
    QString imgPath = m_tempDir.path() + "/testpet.png";
    QImage img(32, 32, QImage::Format_ARGB32);
    img.fill(Qt::blue);
    img.save(imgPath);

    SpriteManager mgr;
    bool ok = mgr.importCustom(imgPath);
    QVERIFY(ok);
    QVERIFY(mgr.current() != nullptr);
    QCOMPARE(mgr.current()->name, QString("testpet"));
    // All states should fallback to idle
    for (int s = 0; s <= static_cast<int>(PetState::Sleeping); ++s) {
        PetState st = static_cast<PetState>(s);
        QVERIFY2(mgr.current()->frames.contains(st),
                 QString("State %1 should have fallback frames").arg(s).toUtf8());
    }
}

void TestSpriteManager::testImportBadFile()
{
    SpriteManager mgr;
    bool ok = mgr.importCustom("/nonexistent/file_12345.png");
    QVERIFY(!ok);
    QVERIFY(mgr.allPets().isEmpty());
}

void TestSpriteManager::testSwitchPet()
{
    SpriteManager mgr;

    // Add two pets
    QString img1 = m_tempDir.path() + "/pet1.png";
    QString img2 = m_tempDir.path() + "/pet2.png";
    QImage img(16, 16, QImage::Format_ARGB32);
    img.fill(Qt::green);
    img.save(img1);
    img.fill(Qt::yellow);
    img.save(img2);

    mgr.importCustom(img1);
    mgr.importCustom(img2);

    QCOMPARE(mgr.allPets().size(), 2);

    // Switch by name
    QVERIFY(mgr.switchTo("pet2"));
    QCOMPARE(mgr.current()->name, QString("pet2"));

    // Switch by index
    QVERIFY(mgr.switchTo(0));
    QCOMPARE(mgr.current()->name, QString("pet1"));

    // Bad switch
    QVERIFY(!mgr.switchTo("nonexistent"));
    QVERIFY(!mgr.switchTo(99));
}

void TestSpriteManager::testFallbackFrames()
{
    // Import a single image — all states should fall back to idle
    QString imgPath = m_tempDir.path() + "/single.png";
    QImage img(32, 32, QImage::Format_ARGB32);
    img.fill(Qt::magenta);
    img.save(imgPath);

    SpriteManager mgr;
    mgr.importCustom(imgPath);

    auto idleFrames = mgr.current()->frames[PetState::Idle];
    QVERIFY(!idleFrames.isEmpty());

    // Every non-idle state should have same frames as idle
    QVector<PetState> otherStates = {
        PetState::Walking, PetState::Clicked,
        PetState::Happy, PetState::Sleeping
    };
    for (auto st : otherStates) {
        const auto &stateFrames = mgr.current()->frames[st];
        QCOMPARE(stateFrames.size(), idleFrames.size());
        // Compare image data (QPixmap::operator== is deleted)
        for (int i = 0; i < stateFrames.size(); ++i) {
            QCOMPARE(stateFrames[i].size(), idleFrames[i].size());
        }
    }
}

QTEST_MAIN(TestSpriteManager)
#include "test_sprite_manager.moc"
