#include "SpriteManager.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QImageReader>
#include <QMovie>
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

    const auto entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &entry : entries) {
        SpriteData data = loadFromDirectory(entry.absoluteFilePath());
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

SpriteData SpriteManager::loadFromGif(const QString &filePath)
{
    SpriteData data;
    QFileInfo fi(filePath);

    // Try QImageReader first (synchronous, works for most GIFs)
    QImageReader reader(filePath, "gif");
    if (!reader.canRead()) {
        qWarning() << "Cannot read GIF:" << filePath;
        return data;
    }

    data.name = fi.baseName();
    data.frameRate = reader.nextImageDelay(); // ms delay of first frame, default 100

    // Extract all frames
    int frameCount = reader.imageCount();
    if (frameCount == 0) {
        // Some GIFs report 0; read until no more images
        while (true) {
            QImage img = reader.read();
            if (img.isNull()) break;
            data.frames[PetState::Idle].append(QPixmap::fromImage(img));
            if (!reader.jumpToNextImage()) break;
        }
    } else {
        for (int i = 0; i < frameCount; ++i) {
            reader.jumpToImage(i);
            QImage img = reader.read();
            if (!img.isNull()) {
                data.frames[PetState::Idle].append(QPixmap::fromImage(img));
            }
        }
    }

    if (data.frames[PetState::Idle].isEmpty()) {
        qWarning() << "No frames extracted from GIF:" << filePath;
        return SpriteData();
    }

    qDebug() << "Loaded" << data.frames[PetState::Idle].size()
             << "frames from GIF:" << filePath;
    return data;
}

bool SpriteManager::importCustom(const QString &path)
{
    QFileInfo fi(path);
    SpriteData data;

    if (fi.isDir()) {
        data = loadFromDirectory(path);
    } else if (fi.suffix().toLower() == "gif") {
        data = loadFromGif(path);
    } else {
        // Treat single file as idle frame
        QPixmap pix(path);
        if (pix.isNull()) return false;
        data.name = fi.baseName();
        data.frames[PetState::Idle] = {pix};
    }

    if (data.isEmpty()) return false;

    // Copy fallback frames: any unset state uses idle
    auto idleIt = data.frames.find(PetState::Idle);
    if (idleIt != data.frames.end()) {
        for (int s = 0; s <= static_cast<int>(PetState::Sleeping); ++s) {
            PetState st = static_cast<PetState>(s);
            if (!data.frames.contains(st) || data.frames[st].isEmpty()) {
                data.frames[st] = idleIt.value();
            }
        }
    }

    m_pets.append(data);
    if (m_currentIndex < 0) m_currentIndex = 0;
    emit petListChanged();
    return true;
}

void SpriteManager::addSprite(const SpriteData &data)
{
    // Replace existing custom pet, or append
    for (int i = 0; i < m_pets.size(); ++i) {
        if (m_pets[i].name == data.name) {
            m_pets[i] = data;
            m_currentIndex = i;
            emit petListChanged();
            return;
        }
    }
    m_pets.append(data);
    m_currentIndex = m_pets.size() - 1;
    emit petListChanged();
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
