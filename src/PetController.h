#ifndef PETCONTROLLER_H
#define PETCONTROLLER_H

#include <QObject>
#include <QMenu>
#include "SpriteData.h"

class PetWindow;
class SpriteManager;
class AnimationEngine;
class BehaviorEngine;
class SettingsDialog;

class PetController : public QObject {
    Q_OBJECT

public:
    explicit PetController(QObject *parent = nullptr);
    ~PetController();

    void init();
    void shutdown();

private slots:
    void onRightClicked(QPoint globalPos);
    void onSettingsTriggered();
    void onPetSelected(int index);
    void onSpriteImported(SpriteData data);
    void onScaleChanged(qreal factor);
    void onHide();
    void onQuit();

private:
    void applyCurrentPet();
    void showContextMenu(QPoint globalPos);

    PetWindow *m_window = nullptr;
    SpriteManager *m_spriteManager = nullptr;
    AnimationEngine *m_animation = nullptr;
    BehaviorEngine *m_behavior = nullptr;
    SettingsDialog *m_settingsDialog = nullptr;

    QMenu *m_contextMenu = nullptr;
};

#endif // PETCONTROLLER_H
