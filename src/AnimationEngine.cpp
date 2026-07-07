#include "AnimationEngine.h"

AnimationEngine::AnimationEngine(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, &AnimationEngine::onTick);
}

void AnimationEngine::setSpriteData(const SpriteData *data)
{
    m_spriteData = data;
    m_frameIndex = 0;
    if (m_spriteData && !m_spriteData->isEmpty()) {
        m_timer->start(m_spriteData->frameRate);
    } else {
        m_timer->stop();
    }
}

void AnimationEngine::setState(PetState state)
{
    m_currentState = state;
    m_frameIndex = 0;
    // Emit first frame immediately
    onTick();
}

QPixmap AnimationEngine::currentFrame() const
{
    if (!m_spriteData) return {};
    auto it = m_spriteData->frames.find(m_currentState);
    if (it == m_spriteData->frames.end() || it->isEmpty()) {
        // Fallback to idle
        auto idleIt = m_spriteData->frames.find(PetState::Idle);
        if (idleIt != m_spriteData->frames.end() && !idleIt->isEmpty()) {
            return idleIt->first();
        }
        return {};
    }
    return it->at(m_frameIndex % it->size());
}

void AnimationEngine::onTick()
{
    emit frameChanged(currentFrame());
    if (!m_spriteData) return;
    auto it = m_spriteData->frames.find(m_currentState);
    if (it != m_spriteData->frames.end() && !it->isEmpty()) {
        m_frameIndex = (m_frameIndex + 1) % it->size();
    }
}
