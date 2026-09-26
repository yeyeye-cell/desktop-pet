#include "SpriteManager.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDebug>

SpriteManager::SpriteManager(QObject *parent)
    : QObject(parent)
{
}

void SpriteManager::loadPresets(const QString &presetDir)
{
    QDir dir(presetDir);
    if (!dir.exists()) {
        qWarning() << "Preset directory not found:" << presetDir;
        return;
    }

    const QStringList builtInPets = {"bird", "cat", "dog"};
    for (const QString &petName : builtInPets) {
        const QString petPath = dir.filePath(petName);
        if (!QFileInfo(petPath).isDir()) continue;

        SpriteData data = loadFromDirectory(petPath);
        if (!data.isEmpty()) {
            m_pets.append(data);
        }
    }

    if (!m_pets.isEmpty()) {
        m_currentIndex = 0;
    }
    emit petListChanged();
}

SpriteData SpriteManager::loadFromDirectory(const QString &dirPath)
{
    SpriteData data;

    // Read pet.json for metadata
    QFile jsonFile(dirPath + "/pet.json");
    if (jsonFile.open(QIODevice::ReadOnly)) {
        QJsonObject json = QJsonDocument::fromJson(jsonFile.readAll()).object();
        data.name = json.value("name").toString();
        data.frameRate = json.value("frameRate").toInt(100);
    } else {
        data.name = QFileInfo(dirPath).fileName(); // fallback: folder name
    }

    // Map state names to folder names
    struct StateFolder { PetState state; QString folder; };
    const QVector<StateFolder> mappings = {
        {PetState::Idle,     "idle"},
        {PetState::Walking,  "walk"},
        {PetState::Clicked,  "click"},
        {PetState::Happy,    "happy"},
        {PetState::Sleeping, "sleep"},
    };

    for (const auto &m : mappings) {
        QString folderPath = dirPath + "/" + m.folder;
        QVector<QPixmap> frames = loadFrameSequence(folderPath);
        if (!frames.isEmpty()) {
            data.frames[m.state] = frames;
        }
    }

    return data;
}

QVector<QPixmap> SpriteManager::loadFrameSequence(const QString &folderPath)
{
    QVector<QPixmap> frames;
    QDir dir(folderPath);
    if (!dir.exists()) return frames;

    // Filter image files, sorted by name
    QStringList nameFilters = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
    QStringList files = dir.entryList(nameFilters, QDir::Files, QDir::Name);

    for (const auto &file : files) {
        QPixmap pix(dir.absoluteFilePath(file));
        if (!pix.isNull()) {
            frames.append(pix);
        }
    }
    return frames;
}

bool SpriteManager::switchTo(const QString &name)
{
    for (int i = 0; i < m_pets.size(); ++i) {
        if (m_pets[i].name == name) {
            m_currentIndex = i;
            return true;
        }
    }
    return false;
}

bool SpriteManager::switchTo(int index)
{
    if (index < 0 || index >= m_pets.size()) return false;
    m_currentIndex = index;
    return true;
}

int SpriteManager::currentIndex() const
{
    return m_currentIndex;
}

const SpriteData *SpriteManager::current() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_pets.size()) return nullptr;
    return &m_pets.at(m_currentIndex);
}

QVector<SpriteData> SpriteManager::allPets() const
{
    return m_pets;
}
