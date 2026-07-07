#ifndef SPRITEDATA_H
#define SPRITEDATA_H

#include <QString>
#include <QMap>
#include <QVector>
#include <QPixmap>

enum class PetState {
    Idle,
    Walking,
    Clicked,
    Happy,
    Sleeping,
    Dragged   // 拖拽时复用 Happy 动画帧
};

struct SpriteData {
    QString name;
    QMap<PetState, QVector<QPixmap>> frames;
    int frameRate = 100;   // ms per frame
    qreal scale = 1.0;

    bool isEmpty() const { return name.isEmpty() || frames.isEmpty(); }
};

#endif // SPRITEDATA_H
