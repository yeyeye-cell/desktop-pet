#ifndef BEHAVIORENGINE_H
#define BEHAVIORENGINE_H

#include <QObject>
#include <QTimer>
#include <QPoint>
#include "SpriteData.h"

class BehaviorEngine : public QObject {
    Q_OBJECT

public:
    explicit BehaviorEngine(QObject *parent = nullptr);

    void start();
    void stop();

    // Inputs from PetWindow / PetController
    void onClicked();
    void onDragged(QPoint delta);
    void onDragReleased();
    void setPetPosition(QPoint screenPos);

signals:
    void stateChanged(PetState state);
    void moveRequested(QPoint delta);

private slots:
    void onPoll();

private:
    void setState(PetState state);
    QPoint randomWalkTarget() const;
    QPoint getMouseScreenPos() const;

    QTimer *m_pollTimer;
    QTimer *m_walkTimer;
    QTimer *m_idleTimer;

    PetState m_currentState = PetState::Idle;
    PetState m_pendingState = PetState::Idle;

    bool m_isDragging = false;
    QPoint m_petPosition;        // pet window screen position
    int m_idleSeconds = 0;

    static constexpr int POLL_INTERVAL = 50;       // ms (faster for smooth follow)
    static constexpr int NEAR_DISTANCE = 200;      // px — enter HAPPY
    static constexpr int LEAVE_DISTANCE = 250;     // px — leave HAPPY (hysteresis)
    static constexpr int IDLE_SLEEP_SECS = 30;
    static constexpr int WALK_INTERVAL_MIN = 5000;
    static constexpr int WALK_INTERVAL_MAX = 15000;
    static constexpr int HAPPY_FOLLOW_SPEED = 6;   // px per poll toward mouse
};

#endif // BEHAVIORENGINE_H
