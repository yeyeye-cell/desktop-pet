#include "BehaviorEngine.h"
#include <QCursor>
#include <QScreen>
#include <QGuiApplication>
#include <random>

static int randomInt(int min, int max)
{
    static std::mt19937 rng(std::random_device{}());
    return std::uniform_int_distribution<int>(min, max)(rng);
}

BehaviorEngine::BehaviorEngine(QObject *parent)
    : QObject(parent)
    , m_pollTimer(new QTimer(this))
    , m_walkTimer(new QTimer(this))
    , m_idleTimer(new QTimer(this))
{
    m_pollTimer->setInterval(POLL_INTERVAL);
    connect(m_pollTimer, &QTimer::timeout, this, &BehaviorEngine::onPoll);

    m_walkTimer->setSingleShot(true);
    connect(m_walkTimer, &QTimer::timeout, this, [this]() {
        if (m_currentState == PetState::Idle) {
            setState(PetState::Walking);
            QPoint target = randomWalkTarget();
            emit moveRequested(target);
            // Walk ends after 1-2 seconds
            QTimer::singleShot(randomInt(1000, 2000), this, [this]() {
                if (m_currentState == PetState::Walking) {
                    setState(PetState::Idle);
                }
            });
        }
        // Schedule next walk
        m_walkTimer->start(randomInt(WALK_INTERVAL_MIN, WALK_INTERVAL_MAX));
    });
}

void BehaviorEngine::start()
{
    m_pollTimer->start();
    m_walkTimer->start(randomInt(WALK_INTERVAL_MIN, WALK_INTERVAL_MAX));
    setState(PetState::Idle);
}

void BehaviorEngine::stop()
{
    m_pollTimer->stop();
    m_walkTimer->stop();
}

void BehaviorEngine::onClicked()
{
    if (m_currentState == PetState::Sleeping) {
        setState(PetState::Idle);
    } else {
        setState(PetState::Clicked);
        // Return to idle after 800ms
        QTimer::singleShot(800, this, [this]() {
            if (m_currentState == PetState::Clicked) {
                setState(PetState::Idle);
            }
        });
    }
}

void BehaviorEngine::onDragged(QPoint delta)
{
    Q_UNUSED(delta);
    m_isDragging = true;
    setState(PetState::Dragged);
}

void BehaviorEngine::onDragReleased()
{
    m_isDragging = false;
    setState(PetState::Idle);
}

void BehaviorEngine::onPoll()
{
    if (m_isDragging) return;

    QPoint mousePos = getMouseScreenPos();
    bool wasNear = (m_lastMousePos - QCursor::pos()).manhattanLength() < NEAR_DISTANCE;

    // Check mouse proximity
    if (!wasNear) {
        m_lastMousePos = mousePos;
    }
    m_idleSeconds++;

    // Priority: Idle → Walking → Sleeping → Happy
    if (m_currentState == PetState::Sleeping) {
        // Wake up on mouse near
        QPoint delta = mousePos - QCursor::pos();
        if (delta.manhattanLength() < NEAR_DISTANCE) {
            setState(PetState::Idle);
            m_idleSeconds = 0;
        }
        return;
    }

    // Idle timeout → sleep
    if (m_idleSeconds >= IDLE_SLEEP_SECS * (1000 / POLL_INTERVAL)) {
        if (m_currentState == PetState::Idle || m_currentState == PetState::Walking) {
            setState(PetState::Sleeping);
            return;
        }
    }
}

void BehaviorEngine::setState(PetState state)
{
    if (m_currentState != state) {
        m_currentState = state;
        m_idleSeconds = 0;
        emit stateChanged(state);
    }
}

QPoint BehaviorEngine::randomWalkTarget() const
{
    int dx = randomInt(-100, 100);
    int dy = randomInt(-50, 50);
    return QPoint(dx, dy);
}

QPoint BehaviorEngine::getMouseScreenPos() const
{
    return QCursor::pos();
}
