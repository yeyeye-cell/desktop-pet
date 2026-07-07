#include "PetWindow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QShowEvent>
#include <QBitmap>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

PetWindow::PetWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint
                   | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);
    setFixedSize(128, 128);

    QBitmap mask(128, 128);
    mask.clear();
    setMask(mask);
}

void PetWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
#ifdef Q_OS_WIN
    // After window creation, strip ALL border/edge styles at Win32 level
    HWND hwnd = HWND(winId());
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    // Remove any possible border-causing styles
    style &= ~(WS_BORDER | WS_DLGFRAME | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE);

    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    // Force redraw of non-client area
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
#endif
}

void PetWindow::setPetPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull()) return;
    m_currentFrame = pixmap;

    // Fast: convert alpha channel to 1-bit mask → shapes window to pet outline
    QImage alphaImg = pixmap.toImage().convertToFormat(QImage::Format_Alpha8);
    setMask(QBitmap::fromImage(alphaImg));

    update();
}

void PetWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (!m_currentFrame.isNull()) {
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect(), Qt::transparent);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawPixmap(0, 0, m_currentFrame);
    }
}

bool PetWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG *msg = static_cast<MSG *>(message);

        if (msg->message == WM_NCCALCSIZE) {
            *result = 0;
            return true;
        }

        if (msg->message == WM_NCHITTEST) {
            *result = HTCLIENT;
            return true;
        }

        if (msg->message == WM_MOUSEACTIVATE) {
            *result = MA_NOACTIVATE;
            return true;
        }

        if (msg->message == WM_NCPAINT) {
            *result = 0;
            return true;
        }
    }
#endif
    return QWidget::nativeEvent(eventType, message, result);
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
