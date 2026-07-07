#include "PetController.h"
#include "PetWindow.h"
#include "SpriteManager.h"
#include "AnimationEngine.h"
#include "BehaviorEngine.h"
#include "SettingsDialog.h"

#include <QApplication>
#include <QCoreApplication>
#include <QMenu>
#include <QSettings>
#include <QDebug>

PetController::PetController(QObject *parent)
    : QObject(parent)
{
}

PetController::~PetController()
{
    shutdown();
}

void PetController::init()
{
    // 1. Create window
    m_window = new PetWindow;
    m_window->show();

    // 2. Create sprite manager and load presets
    m_spriteManager = new SpriteManager(this);
    m_spriteManager->loadPresets(QCoreApplication::applicationDirPath() + "/resources/presets");

    // 3. Create animation engine
    m_animation = new AnimationEngine(this);
    applyCurrentPet();

    // 4. Create behavior engine
    m_behavior = new BehaviorEngine(this);
    m_behavior->start();

    // 5. Create settings dialog (lazy — on demand)
    m_settingsDialog = new SettingsDialog;

    // 6. Wire signals
    // Animation → Window
    connect(m_animation, &AnimationEngine::frameChanged,
            m_window, &PetWindow::setPetPixmap);

    // Behavior → Animation
    connect(m_behavior, &BehaviorEngine::stateChanged,
            m_animation, &AnimationEngine::setState);

    // Behavior → Window (move)
    connect(m_behavior, &BehaviorEngine::moveRequested, this, [this](QPoint delta) {
        m_window->move(m_window->pos() + delta);
    });

    // Window → Behavior
    connect(m_window, &PetWindow::clicked,
            m_behavior, &BehaviorEngine::onClicked);
    connect(m_window, &PetWindow::dragged,
            m_behavior, &BehaviorEngine::onDragged);

    // Also move window on drag
    connect(m_window, &PetWindow::dragged, this, [this](QPoint delta) {
        m_window->move(m_window->pos() + delta);
    });

    connect(m_window, &PetWindow::rightClicked,
            this, &PetController::onRightClicked);

    // Settings dialog
    connect(m_settingsDialog, &SettingsDialog::petChanged,
            this, &PetController::onPetSelected);
    connect(m_settingsDialog, &SettingsDialog::spriteImported,
            this, &PetController::onSpriteImported);
    connect(m_settingsDialog, &SettingsDialog::scaleChanged,
            this, &PetController::onScaleChanged);

    // Build context menu
    m_contextMenu = new QMenu;
    auto *switchMenu = m_contextMenu->addMenu(QStringLiteral("切换宠物"));
    auto *settingsAction = m_contextMenu->addAction(QStringLiteral("宠物设置..."));
    auto *hideAction = m_contextMenu->addAction(QStringLiteral("隐藏"));
    m_contextMenu->addSeparator();
    auto *quitAction = m_contextMenu->addAction(QStringLiteral("退出"));

    connect(settingsAction, &QAction::triggered, this, &PetController::onSettingsTriggered);
    connect(hideAction, &QAction::triggered, this, &PetController::onHide);
    connect(quitAction, &QAction::triggered, this, &PetController::onQuit);

    qDebug() << "PetController initialized.";
}

void PetController::shutdown()
{
    QSettings settings("DesktopPet", "DesktopPet");
    settings.setValue("currentPet", m_spriteManager->current() ?
                      m_spriteManager->current()->name : "");
    settings.setValue("scale", static_cast<int>(m_settingsDialog->scalePercent()));
}

void PetController::onRightClicked(QPoint globalPos)
{
    showContextMenu(globalPos);
}

void PetController::showContextMenu(QPoint globalPos)
{
    // Update switch menu items
    auto *switchMenu = m_contextMenu->actions().first()->menu();
    if (!switchMenu) return;
    switchMenu->clear();

    auto pets = m_spriteManager->allPets();
    for (int i = 0; i < pets.size(); ++i) {
        auto *action = switchMenu->addAction(pets[i].name);
        connect(action, &QAction::triggered, this, [this, i]() { onPetSelected(i); });
    }

    m_contextMenu->popup(globalPos);
}

void PetController::onSettingsTriggered()
{
    m_settingsDialog->setPets(m_spriteManager->allPets(),
                              m_spriteManager->current() ? 0 : -1);
    if (m_settingsDialog->exec() == QDialog::Accepted) {
        onScaleChanged(m_settingsDialog->scalePercent() / 100.0);
    }
}

void PetController::onPetSelected(int index)
{
    if (m_spriteManager->switchTo(index)) {
        applyCurrentPet();
    }
}

void PetController::onSpriteImported(SpriteData data)
{
    m_spriteManager->addSprite(data);
    applyCurrentPet();
    qDebug() << "Custom sprite imported:" << data.name
             << "with" << data.frames.size() << "states";
}

void PetController::onScaleChanged(qreal factor)
{
    // Scaling will be implemented later
    Q_UNUSED(factor);
}

void PetController::onHide()
{
    m_window->hide();
    // TODO: system tray restore
}

void PetController::onQuit()
{
    shutdown();
    qApp->quit();
}

void PetController::applyCurrentPet()
{
    const SpriteData *data = m_spriteManager->current();
    if (data) {
        m_animation->setSpriteData(data);
    }
}
