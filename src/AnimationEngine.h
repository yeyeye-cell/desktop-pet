#ifndef ANIMATIONENGINE_H
#define ANIMATIONENGINE_H

#include <QObject>
#include <QTimer>
#include <QPixmap>
#include "SpriteData.h"

class AnimationEngine : public QObject {
    Q_OBJECT

public:
    explicit AnimationEngine(QObject *parent = nullptr);

    void setSpriteData(const SpriteData *data);
    void setState(PetState state);
    QPixmap currentFrame() const;

signals:
    void frameChanged(const QPixmap &frame);

private slots:
    void onTick();

private:
    const SpriteData *m_spriteData = nullptr;
    QTimer *m_timer;
    PetState m_currentState = PetState::Idle;
    int m_frameIndex = 0;
};

#endif // ANIMATIONENGINE_H
