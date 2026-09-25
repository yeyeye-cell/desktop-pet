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
#include <QFileInfo>
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

    // Restore custom pet import paths
    for (int i = 0; i < 5; ++i) {
        m_customImportPaths[i] = s.value(QString("customPath_%1").arg(i)).toString();
    }

    // Re-import custom pet if paths exist
    bool hasCustom = false;
    for (int i = 0; i < 5; ++i) {
        if (!m_customImportPaths[i].isEmpty() && QFileInfo::exists(m_customImportPaths[i])) {
            hasCustom = true;
            break;
        }
    }
    if (hasCustom) {
        auto data = buildCustomSprite(m_customImportPaths);
        if (!data.isEmpty()) {
            m_spriteManager->addSprite(data);
        }
    }

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
    m_settingsDialog->setImportPaths(m_customImportPaths);

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
    connect(m_settingsDialog, &SettingsDialog::spriteImported,
            this, &PetController::onSpriteImported);
    connect(m_settingsDialog, &SettingsDialog::scaleChanged,
            this, &PetController::onScaleChanged);

    // Initial position sync
    m_behavior->setPetPosition(m_window->pos());

    // Build context menu
    m_contextMenu = new QMenu;
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

    // Save custom import paths
    for (int i = 0; i < 5; ++i) {
        s.setValue(QString("customPath_%1").arg(i), m_customImportPaths[i]);
    }
}

static QPixmap scaleToFit(const QPixmap &src, int maxSize = 128)
{
    if (src.width() <= maxSize && src.height() <= maxSize)
        return src;
    return src.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

static QPixmap removeWhiteBg(const QPixmap &src)
{
    QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);
    int w = img.width(), h = img.height();

    QList<QColor> corners;
    corners << img.pixelColor(0, 0) << img.pixelColor(w-1, 0)
            << img.pixelColor(0, h-1) << img.pixelColor(w-1, h-1);
    int bgR = 0, bgG = 0, bgB = 0;
    for (auto &c : corners) { bgR += c.red(); bgG += c.green(); bgB += c.blue(); }
    bgR /= 4; bgG /= 4; bgB /= 4;

    QVector<QVector<bool>> visited(h, QVector<bool>(w, false));
    struct Point { int x, y; };
    QList<Point> queue;

    auto addIfBg = [&](int x, int y) {
        if (x < 0 || x >= w || y < 0 || y >= h || visited[y][x]) return;
        QColor c = img.pixelColor(x, y);
        if (std::abs(c.red()-bgR) < 40 && std::abs(c.green()-bgG) < 40 && std::abs(c.blue()-bgB) < 40) {
            visited[y][x] = true;
            queue.append({x, y});
        }
    };

    for (int x = 0; x < w; ++x) { addIfBg(x, 0); addIfBg(x, h-1); }
    for (int y = 0; y < h; ++y) { addIfBg(0, y); addIfBg(w-1, y); }

    while (!queue.isEmpty()) {
        Point p = queue.takeFirst();
        addIfBg(p.x+1, p.y); addIfBg(p.x-1, p.y);
        addIfBg(p.x, p.y+1); addIfBg(p.x, p.y-1);
    }

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (visited[y][x]) img.setPixelColor(x, y, Qt::transparent);

    return QPixmap::fromImage(img);
}

SpriteData PetController::buildCustomSprite(const QString paths[5])
{
    // Reuse the same logic as SettingsDialog::importedSprite()
    // Build SpriteData from file paths
    SpriteData data;
    bool hasAny = false;

    static const PetState states[] = {
        PetState::Idle, PetState::Walking, PetState::Clicked,
        PetState::Happy, PetState::Sleeping
    };

    for (int i = 0; i < 5; ++i) {
        if (paths[i].isEmpty()) continue;
        QFileInfo fi(paths[i]);
        if (!fi.exists()) continue;
        QVector<QPixmap> frames;

        if (fi.suffix().toLower() == "gif") {
            QImageReader reader(paths[i], "gif");
            int count = reader.imageCount();
            if (count == 0) {
                while (true) {
                    QImage img = reader.read();
                    if (img.isNull()) break;
                    frames.append(removeWhiteBg(scaleToFit(QPixmap::fromImage(img))));
                    if (!reader.jumpToNextImage()) break;
                }
            } else {
                for (int f = 0; f < count; ++f) {
                    reader.jumpToImage(f);
                    QImage img = reader.read();
                    if (!img.isNull()) frames.append(removeWhiteBg(scaleToFit(QPixmap::fromImage(img))));
                }
            }
        } else if (fi.isDir()) {
            QDir dir(paths[i]);
            QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
            for (const auto &f : dir.entryList(filters, QDir::Files, QDir::Name)) {
                QPixmap px(dir.absoluteFilePath(f));
                if (!px.isNull()) frames.append(removeWhiteBg(scaleToFit(px)));
            }
        } else {
            QPixmap px(paths[i]);
            if (!px.isNull()) frames.append(removeWhiteBg(scaleToFit(px)));
        }

        if (!frames.isEmpty()) {
            data.frames[states[i]] = frames;
            hasAny = true;
        }
    }

    if (!hasAny) return {};

    data.name = QStringLiteral("自定义宠物");
    data.frameRate = 100;

    auto idleIt = data.frames.find(PetState::Idle);
    if (idleIt != data.frames.end()) {
        for (int s = 0; s < 5; ++s) {
            PetState st = states[s];
            if (!data.frames.contains(st) || data.frames[st].isEmpty()) {
                data.frames[st] = idleIt.value();
            }
        }
    }

    return data;
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

void PetController::onSpriteImported(SpriteData data)
{
    m_spriteManager->addSprite(data);

    // Save import paths
    m_settingsDialog->getImportPaths(m_customImportPaths);
    // Scale and white-bg-remove imported frames
    applyCurrentPet();

    qDebug() << "Custom sprite imported:" << data.name
             << "with" << data.frames.size() << "states";
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
