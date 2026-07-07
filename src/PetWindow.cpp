#include "PetWindow.h"
#include <QPainter>
#include <QMouseEvent>

PetWindow::PetWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint
                   | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setFixedSize(128, 128);
}

void PetWindow::setPetPixmap(const QPixmap &pixmap)
{
    m_currentFrame = pixmap;
    update();
}

void PetWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (!m_currentFrame.isNull()) {
        painter.drawPixmap(0, 0, m_currentFrame);
    } else {
        // Placeholder: draw a circle so we know it works
        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(rect().adjusted(10, 10, -10, -10));
    }
}

void PetWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->globalPosition().toPoint();
        m_isDragging = false;
    }
}

void PetWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        QPoint delta = event->globalPosition().toPoint() - m_dragStartPos;
        if (!m_isDragging && delta.manhattanLength() > 5) {
            m_isDragging = true;
        }
        if (m_isDragging) {
            move(pos() + delta);
            m_dragStartPos = event->globalPosition().toPoint();
            emit dragged(delta);
        }
    }
}

void PetWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_isDragging) {
        emit clicked();
    }
    m_isDragging = false;
}

void PetWindow::contextMenuEvent(QContextMenuEvent *event)
{
    emit rightClicked(event->globalPos());
}
