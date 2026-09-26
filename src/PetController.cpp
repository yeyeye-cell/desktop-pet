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
#include <QSystemTrayIcon>
#include <QStyle>
#include <QAction>

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

    // 3. Load saved state
    QSettings s("DesktopPet", "DesktopPet");
    QString lastPet = s.value("currentPet").toString();
    m_scalePercent = qBound(20, s.value("scale", 100).toInt(), 200);
    for (int i = 0; i < 5; ++i) s.remove(QString("customPath_%1").arg(i));

    // Switch to last pet (or first preset)
    if (!lastPet.isEmpty()) {
        m_spriteManager->switchTo(lastPet);
    }

    // 4. Create animation engine
    m_animation = new AnimationEngine(this);
    applyCurrentPet();

    // 5. Create behavior engine
    m_behavior = new BehaviorEngine(this);
    m_behavior->start();

    // 6. Create settings dialog
    m_settingsDialog = new SettingsDialog;
    m_settingsDialog->setPets(m_spriteManager->allPets(),
                              m_spriteManager->currentIndex());
    m_settingsDialog->setScalePercent(m_scalePercent);

    // 7. Wire signals
    // Animation → Window
    connect(m_animation, &AnimationEngine::frameChanged,
            m_window, &PetWindow::setPetPixmap);

    // Behavior → Animation
    connect(m_behavior, &BehaviorEngine::stateChanged,
            m_animation, &AnimationEngine::setState);

    // Behavior → Window (move) — also sync position back
    connect(m_behavior, &BehaviorEngine::moveRequested, this, [this](QPoint delta) {
        m_window->move(m_window->pos() + delta);
        m_behavior->setPetPosition(m_window->pos());
    });

    // Window → Behavior
    connect(m_window, &PetWindow::clicked,
            m_behavior, &BehaviorEngine::onClicked);
    connect(m_window, &PetWindow::dragged,
            m_behavior, &BehaviorEngine::onDragged);
    connect(m_window, &PetWindow::dragReleased,
            m_behavior, &BehaviorEngine::onDragReleased);

    // Sync behavior position on drag (move already done by PetWindow)
    connect(m_window, &PetWindow::dragged, this, [this](QPoint) {
        m_behavior->setPetPosition(m_window->pos());
    });

    connect(m_window, &PetWindow::rightClicked,
            this, &PetController::onRightClicked);

    // Settings dialog
    connect(m_settingsDialog, &SettingsDialog::petChanged,
            this, &PetController::onPetSelected);
    connect(m_settingsDialog, &SettingsDialog::scaleChanged,
            this, &PetController::onScaleChanged);

    // Initial position sync
    m_behavior->setPetPosition(m_window->pos());

    // Build context menu
    m_contextMenu = new QMenu(m_window);
    m_contextMenu->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    auto *switchMenu = m_contextMenu->addMenu(QStringLiteral("切换宠物"));
    auto *settingsAction = m_contextMenu->addAction(QStringLiteral("宠物设置..."));
    m_hideAction = m_contextMenu->addAction(QStringLiteral("隐藏"));
    m_contextMenu->addSeparator();
    auto *quitAction = m_contextMenu->addAction(QStringLiteral("退出"));

    connect(settingsAction, &QAction::triggered, this, &PetController::onSettingsTriggered);
    connect(m_hideAction, &QAction::triggered, this, &PetController::onHide);
    connect(quitAction, &QAction::triggered, this, &PetController::onQuit);

    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        m_trayIcon = new QSystemTrayIcon(this);
        m_trayIcon->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
        m_trayIcon->setToolTip(QStringLiteral("DesktopPet"));
        auto *trayMenu = new QMenu(m_settingsDialog);
        auto *toggleAction = trayMenu->addAction(QStringLiteral("隐藏宠物"));
        auto *traySettingsAction = trayMenu->addAction(QStringLiteral("宠物设置..."));
        trayMenu->addSeparator();
        auto *trayQuitAction = trayMenu->addAction(QStringLiteral("退出"));
        connect(toggleAction, &QAction::triggered, this, &PetController::onToggleVisibility);
        connect(traySettingsAction, &QAction::triggered, this, &PetController::onSettingsTriggered);
        connect(trayQuitAction, &QAction::triggered, this, &PetController::onQuit);
        connect(m_trayIcon, &QSystemTrayIcon::activated, this,
                [this](QSystemTrayIcon::ActivationReason reason) {
                    if (reason == QSystemTrayIcon::Trigger
                        || reason == QSystemTrayIcon::DoubleClick) onShowPet();
                });
        m_trayIcon->setContextMenu(trayMenu);
        m_trayIcon->show();
    } else {
        m_hideAction->setEnabled(false);
    }

    qDebug() << "PetController initialized. Current pet:"
             << (m_spriteManager->current() ? m_spriteManager->current()->name : "(none)");
}

void PetController::shutdown()
{
    QSettings s("DesktopPet", "DesktopPet");
    if (m_spriteManager && m_spriteManager->current()) {
        s.setValue("currentPet", m_spriteManager->current()->name);
    }
    s.setValue("scale", m_scalePercent);

}

void PetController::onRightClicked(QPoint globalPos)
{
    showContextMenu(globalPos);
}

void PetController::showContextMenu(QPoint globalPos)
{
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
                              m_spriteManager->currentIndex());
    m_settingsDialog->setScalePercent(m_scalePercent);
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

void PetController::onScaleChanged(qreal factor)
{
    m_scalePercent = qBound(20, qRound(factor * 100.0), 200);
    m_window->setScaleFactor(m_scalePercent / 100.0);
}

void PetController::onHide()
{
    m_window->hide();
    if (m_trayIcon && m_trayIcon->contextMenu()) {
        m_trayIcon->contextMenu()->actions().first()->setText(QStringLiteral("显示宠物"));
    }
}

void PetController::onToggleVisibility()
{
    if (m_window->isVisible()) m_window->hide();
    else onShowPet();
    if (m_trayIcon && m_trayIcon->contextMenu()) {
        auto *action = m_trayIcon->contextMenu()->actions().first();
        action->setText(m_window->isVisible() ? QStringLiteral("隐藏宠物")
                                              : QStringLiteral("显示宠物"));
    }
}

void PetController::onShowPet()
{
    m_window->show();
    m_window->raise();
    if (m_trayIcon && m_trayIcon->contextMenu()) {
        m_trayIcon->contextMenu()->actions().first()->setText(QStringLiteral("隐藏宠物"));
    }
}

void PetController::onQuit()
{
    qApp->quit();
}

void PetController::applyCurrentPet()
{
    const SpriteData *data = m_spriteManager->current();
    if (data) {
        QSize logicalSize(1, 1);
        for (auto it = data->frames.cbegin(); it != data->frames.cend(); ++it) {
            for (const QPixmap &frame : it.value()) {
                logicalSize.setWidth(qMax(logicalSize.width(), frame.width()));
                logicalSize.setHeight(qMax(logicalSize.height(), frame.height()));
            }
        }
        m_window->setLogicalSize(logicalSize);
        m_window->setScaleFactor(m_scalePercent / 100.0);
        m_animation->setSpriteData(data);
    }
}
