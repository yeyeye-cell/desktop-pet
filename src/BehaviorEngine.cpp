#include "BehaviorEngine.h"
#include <QCursor>
#include <QScreen>
#include <QGuiApplication>
#include <cmath>
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
            emit moveRequested(randomWalkTarget());
            QTimer::singleShot(randomInt(1000, 2000), this, [this]() {
                if (m_currentState == PetState::Walking) {
                    setState(PetState::Idle);
                }
            });
        }
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

void BehaviorEngine::setPetPosition(QPoint screenPos)
{
    m_petPosition = screenPos;
}

void BehaviorEngine::onClicked()
{
    if (m_currentState == PetState::Sleeping) {
        setState(PetState::Idle);
    } else {
        setState(PetState::Clicked);
        QTimer::singleShot(800, this, [this]() {
            if (m_currentState == PetState::Clicked) {
                setState(PetState::Idle);
            }
        });
    }
}

void BehaviorEngine::onDragged(QPoint)
{
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

    QPoint mousePos = QCursor::pos();
    int distToMouse = (mousePos - m_petPosition).manhattanLength();

    // === HAPPY: mouse near → follow ===
    if (distToMouse < NEAR_DISTANCE
        && m_currentState != PetState::Clicked
        && m_currentState != PetState::Dragged
        && m_currentState != PetState::Sleeping)
    {
        if (m_currentState != PetState::Happy) {
            setState(PetState::Happy);
        }
        // Move pet toward mouse (smooth step)
        int dx = mousePos.x() - m_petPosition.x();
        int dy = mousePos.y() - m_petPosition.y();
        double len = std::sqrt(double(dx*dx + dy*dy));
        if (len > 1.0) {
            int stepX = int(dx / len * HAPPY_FOLLOW_SPEED);
            int stepY = int(dy / len * HAPPY_FOLLOW_SPEED);
            // Clamp to not overshoot
            if (std::abs(stepX) > std::abs(dx)) stepX = dx;
            if (std::abs(stepY) > std::abs(dy)) stepY = dy;
            emit moveRequested(QPoint(stepX, stepY));
        }
        m_idleSeconds = 0;
        return;
    }

    // === Leave HAPPY when mouse moves away ===
    if (m_currentState == PetState::Happy && distToMouse > LEAVE_DISTANCE) {
        setState(PetState::Idle);
        return;
    }

    // === SLEEPING: wake up ===
    if (m_currentState == PetState::Sleeping) {
        if (distToMouse < NEAR_DISTANCE) {
            setState(PetState::Idle);
            m_idleSeconds = 0;
        }
        return;
    }

    // === CLICKED returning to idle ===
    if (m_currentState == PetState::Clicked) {
        return; // timer handles transition back to idle
    }

    // === WALKING — timer handles transitions ===

    // === Idle timeout → sleep ===
    m_idleSeconds++;
    if (m_idleSeconds >= IDLE_SLEEP_SECS * (1000 / POLL_INTERVAL)) {
        if (m_currentState == PetState::Idle || m_currentState == PetState::Walking) {
            setState(PetState::Sleeping);
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
    return QPoint(randomInt(-100, 100), randomInt(-50, 50));
}
