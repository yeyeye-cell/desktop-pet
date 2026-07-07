#include "PetWindow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QShowEvent>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
typedef struct _MARGINS { int cxLeftWidth, cxRightWidth, cyTopHeight, cyBottomHeight; } MARGINS;
#endif

PetWindow::PetWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint
                   | Qt::WindowStaysOnTopHint
                   | Qt::NoDropShadowWindowHint
                   | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAutoFillBackground(false);
    setStyleSheet("QWidget { border: none; margin: 0px; padding: 0px; background: transparent; }");
    setContentsMargins(0, 0, 0, 0);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::transparent);
    pal.setColor(QPalette::Base, Qt::transparent);
    setPalette(pal);

    resize(128, 128);
}

void PetWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
#ifdef Q_OS_WIN
    HWND hwnd = HWND(winId());

    // Force clean window style
    SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW);

    // DWM: remove shadow, border color, round corners
    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (dwm) {
        typedef HRESULT (WINAPI *DwmExtend_t)(HWND, const MARGINS *);
        auto extendFn = (DwmExtend_t)GetProcAddress(dwm, "DwmExtendFrameIntoClientArea");
        if (extendFn) { MARGINS m = {-1, -1, -1, -1}; extendFn(hwnd, &m); }

        typedef HRESULT (WINAPI *DwmAttr_t)(HWND, DWORD, LPCVOID, DWORD);
        auto attrFn = (DwmAttr_t)GetProcAddress(dwm, "DwmSetWindowAttribute");
        if (attrFn) {
            // Disable DWM non-client rendering entirely
            int ncrp = 1; // DWMNCRP_DISABLED
            attrFn(hwnd, 2, &ncrp, sizeof(ncrp)); // DWMWA_NCRENDERING_POLICY

            // Border color → transparent
            COLORREF borderNone = 0x00FFFFFF;
            attrFn(hwnd, 34, &borderNone, sizeof(borderNone)); // DWMWA_BORDER_COLOR

            // Win11: don't round corners
            int cornerPref = 1; // DWMWCP_DONOTROUND
            attrFn(hwnd, 33, &cornerPref, sizeof(cornerPref)); // DWMWA_WINDOW_CORNER_PREFERENCE
        }
        FreeLibrary(dwm);
    }

    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
#endif
}

void PetWindow::setPetPixmap(const QPixmap &pixmap)
{
    if (pixmap.isNull()) return;
    m_currentFrame = pixmap;
    if (size() != pixmap.size()) resize(pixmap.size());
    update();
}

void PetWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    if (!m_currentFrame.isNull()) {
        painter.drawPixmap(0, 0, m_currentFrame);
    }
}

bool PetWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_NCCALCSIZE)   { *result = 0; return true; }
        if (msg->message == WM_NCHITTEST)    { *result = HTCLIENT; return true; }
        if (msg->message == WM_MOUSEACTIVATE) { *result = MA_NOACTIVATE; return true; }
        if (msg->message == WM_NCPAINT)      { *result = 0; return true; }
        if (msg->message == WM_ERASEBKGND)   { *result = 1; return true; }
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
        if (!m_isDragging && delta.manhattanLength() > 5) m_isDragging = true;
        if (m_isDragging) {
            move(pos() + delta);
            m_dragStartPos = event->globalPosition().toPoint();
            emit dragged(delta);
        }
    }
}

void PetWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_isDragging) emit clicked();
    m_isDragging = false;
}

void PetWindow::contextMenuEvent(QContextMenuEvent *event)
{
    emit rightClicked(event->globalPos());
}
