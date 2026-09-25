#ifndef PETWINDOW_H
#define PETWINDOW_H

#include <QWidget>
#include <QPixmap>

class PetWindow : public QWidget {
    Q_OBJECT

public:
    explicit PetWindow(QWidget *parent = nullptr);
    void setPetPixmap(const QPixmap &pixmap);
    void setLogicalSize(const QSize &size);
    void setScaleFactor(qreal factor);

signals:
    void clicked();
    void dragged(QPoint delta);
    void dragReleased();
    void rightClicked(QPoint pos);
    void mouseNear(bool isNear);

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    QPixmap m_currentFrame;
    QSize m_logicalSize = QSize(128, 128);
    qreal m_scaleFactor = 1.0;
    QPoint m_dragStartPos;
    bool m_isDragging = false;
};

#endif // PETWINDOW_H
