#ifndef SPRITEMANAGER_H
#define SPRITEMANAGER_H

#include <QObject>
#include <QVector>
#include "SpriteData.h"

class SpriteManager : public QObject {
    Q_OBJECT

public:
    explicit SpriteManager(QObject *parent = nullptr);

    void loadPresets(const QString &presetDir);
    bool importCustom(const QString &path);
    void addSprite(const SpriteData &data);
    bool switchTo(const QString &name);
    bool switchTo(int index);
    int currentIndex() const;
    const SpriteData *current() const;
    QVector<SpriteData> allPets() const;

signals:
    void petListChanged();

private:
    SpriteData loadFromDirectory(const QString &dirPath);
    SpriteData loadFromGif(const QString &filePath);
    QVector<QPixmap> loadFrameSequence(const QString &folderPath);

    QVector<SpriteData> m_pets;
    int m_currentIndex = -1;
};

#endif // SPRITEMANAGER_H
