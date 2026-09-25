#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QSlider>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QImageReader>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("宠物设置"));
    setMinimumSize(440, 420);
    setupUi();
}

const char *SettingsDialog::stateFolderName(int s)
{
    static const char *names[] = {"idle", "walk", "click", "happy", "sleep"};
    return names[s];
}

PetState SettingsDialog::stateFromIndex(int i)
{
    static const PetState states[] = {
        PetState::Idle, PetState::Walking, PetState::Clicked,
        PetState::Happy, PetState::Sleeping
    };
    return states[i];
}

void SettingsDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // --- Preset selection ---
    auto *presetGroup = new QGroupBox(QStringLiteral("选择预设宠物"), this);
    presetGroup->setObjectName(QStringLiteral("presetGroup"));
    auto *presetLayout = new QHBoxLayout(presetGroup);
    m_petGroup = new QButtonGroup(this);
    m_petGroup->setExclusive(true);
    presetLayout->addStretch();
    mainLayout->addWidget(presetGroup);

    // --- Import section ---
    auto *importGroup = new QGroupBox(QStringLiteral("导入自定义素材"), this);
    auto *importLayout = new QFormLayout(importGroup);

    const char *stateNames[] = {
        "待机 (Idle)", "走路 (Walk)", "点击 (Click)",
        "开心 (Happy)", "睡觉 (Sleep)"
    };
    for (int i = 0; i < 5; ++i) {
        auto *row = new QHBoxLayout;
        auto *btn = new QPushButton(QStringLiteral("选择文件..."), this);
        m_importLabels[i] = new QLabel(QStringLiteral("(未设置)"), this);
        m_importLabels[i]->setStyleSheet("color: #888; font-size: 11px;");
        connect(btn, &QPushButton::clicked, this, [this, i]() { onImportState(i); });
        row->addWidget(btn);
        row->addWidget(m_importLabels[i]);
        row->addStretch();
        importLayout->addRow(new QLabel(QString::fromUtf8(stateNames[i]), this), row);
    }
    mainLayout->addWidget(importGroup);

    // --- Scale slider ---
    auto *scaleGroup = new QGroupBox(QStringLiteral("缩放"), this);
    auto *scaleLayout = new QHBoxLayout(scaleGroup);
    m_scaleSlider = new QSlider(Qt::Horizontal, this);
    m_scaleSlider->setRange(20, 200);
    m_scaleSlider->setValue(100);
    auto *scaleLabel = new QLabel("100%", this);
    connect(m_scaleSlider, &QSlider::valueChanged, this, [scaleLabel](int v) {
        scaleLabel->setText(QString("%1%").arg(v));
    });
    scaleLayout->addWidget(m_scaleSlider);
    scaleLayout->addWidget(scaleLabel);
    mainLayout->addWidget(scaleGroup);

    // --- OK/Cancel ---
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto *okBtn = new QPushButton(QStringLiteral("确定"), this);
    auto *cancelBtn = new QPushButton(QStringLiteral("取消"), this);
    connect(okBtn, &QPushButton::clicked, this, &SettingsDialog::onOk);
    connect(cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancel);
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);
}

void SettingsDialog::setPets(const QVector<SpriteData> &pets, int currentIndex)
{
    for (auto *btn : m_petGroup->buttons()) {
        m_petGroup->removeButton(btn);
        delete btn;
    }

    auto *presetGroup = findChild<QGroupBox*>(QStringLiteral("presetGroup"));
    if (!presetGroup) return;
    auto *presetLayout = presetGroup->layout();

    for (int i = 0; i < pets.size(); ++i) {
        auto *btn = new QPushButton(pets[i].name, this);
        btn->setCheckable(true);
        if (i == currentIndex) btn->setChecked(true);
        m_petGroup->addButton(btn, i);
        presetLayout->addWidget(btn);
    }

    m_currentPetIndex = currentIndex;
}

int SettingsDialog::currentPetIndex() const
{
    return m_petGroup ? m_petGroup->checkedId() : -1;
}

int SettingsDialog::scalePercent() const
{
    return m_scaleSlider ? m_scaleSlider->value() : 100;
}

void SettingsDialog::setScalePercent(int percent)
{
    if (m_scaleSlider) m_scaleSlider->setValue(qBound(20, percent, 200));
}

static QPixmap scaleToFit(const QPixmap &src, int maxSize = 128)
{
    QPixmap scaled = src;
    if (src.width() > maxSize || src.height() > maxSize)
        scaled = src.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return scaled;
}

// Remove background by flood-fill from edges.
// Only pixels connected to the image border get removed — this preserves
// white body parts that are enclosed within the character outline.
static QPixmap removeWhiteBg(const QPixmap &src)
{
    QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);
    int w = img.width(), h = img.height();

    // Sample background color from the 4 corners (median)
    QList<QColor> corners;
    corners << img.pixelColor(0, 0) << img.pixelColor(w-1, 0)
            << img.pixelColor(0, h-1) << img.pixelColor(w-1, h-1);
    int bgR = 0, bgG = 0, bgB = 0;
    for (auto &c : corners) { bgR += c.red(); bgG += c.green(); bgB += c.blue(); }
    bgR /= 4; bgG /= 4; bgB /= 4;

    // Flood-fill from edges: mark all edge-connected bg-colored pixels
    QVector<QVector<bool>> visited(h, QVector<bool>(w, false));
    struct Point { int x, y; };
    QList<Point> queue;

    // Seed queue with all edge pixels that match the background color
    auto addIfBg = [&](int x, int y) {
        if (x < 0 || x >= w || y < 0 || y >= h || visited[y][x]) return;
        QColor c = img.pixelColor(x, y);
        int dr = std::abs(c.red() - bgR);
        int dg = std::abs(c.green() - bgG);
        int db = std::abs(c.blue() - bgB);
        if (dr < 40 && dg < 40 && db < 40) {
            visited[y][x] = true;
            queue.append({x, y});
        }
    };

    // Seed from all 4 edges
    for (int x = 0; x < w; ++x) { addIfBg(x, 0); addIfBg(x, h-1); }
    for (int y = 0; y < h; ++y) { addIfBg(0, y); addIfBg(w-1, y); }

    // BFS flood-fill
    while (!queue.isEmpty()) {
        Point p = queue.takeFirst();
        addIfBg(p.x + 1, p.y);
        addIfBg(p.x - 1, p.y);
        addIfBg(p.x, p.y + 1);
        addIfBg(p.x, p.y - 1);
    }

    // Make visited (background) pixels transparent
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (visited[y][x]) {
                img.setPixelColor(x, y, Qt::transparent);
            }
        }
    }

    return QPixmap::fromImage(img);
}

SpriteData SettingsDialog::importedSprite() const
{
    SpriteData data;
    bool hasAny = false;

    for (int i = 0; i < 5; ++i) {
        if (m_draftImportPaths[i].isEmpty()) continue;

        QFileInfo fi(m_draftImportPaths[i]);
        QVector<QPixmap> frames;

        if (fi.suffix().toLower() == "gif") {
            QImageReader reader(m_draftImportPaths[i], "gif");
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
            QDir dir(m_draftImportPaths[i]);
            QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
            for (const auto &f : dir.entryList(filters, QDir::Files, QDir::Name)) {
                QPixmap px(dir.absoluteFilePath(f));
                if (!px.isNull()) frames.append(removeWhiteBg(scaleToFit(px)));
            }
        } else {
            QPixmap px(m_draftImportPaths[i]);
            if (!px.isNull()) frames.append(removeWhiteBg(scaleToFit(px)));
        }

        if (!frames.isEmpty()) {
            data.frames[stateFromIndex(i)] = frames;
            hasAny = true;
        }
    }

    if (!hasAny) return {};

    data.name = QStringLiteral("自定义宠物");
    data.frameRate = 100;

    // Fallback: unset states use idle
    auto idleIt = data.frames.find(PetState::Idle);
    if (idleIt != data.frames.end()) {
        for (int s = 0; s < 5; ++s) {
            PetState st = stateFromIndex(s);
            if (!data.frames.contains(st) || data.frames[st].isEmpty()) {
                data.frames[st] = idleIt.value();
            }
        }
    }

    return data;
}

void SettingsDialog::onImportState(int stateIndex)
{
    QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择动画素材"),
        QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*)")
    );
    if (path.isEmpty()) return;

    m_draftImportPaths[stateIndex] = path;
    m_importChanged = true;
    QFileInfo fi(path);
    m_importLabels[stateIndex]->setText(QStringLiteral("✔ ") + fi.fileName());
    m_importLabels[stateIndex]->setStyleSheet("color: #4caf50; font-size: 11px; font-weight: bold;");

    qDebug() << "Import state" << stateIndex << "(" << stateFolderName(stateIndex) << "):" << path;
}

void SettingsDialog::updateImportLabels()
{
    for (int i = 0; i < 5; ++i) {
        if (m_importPaths[i].isEmpty()) {
            m_importLabels[i]->setText(QStringLiteral("(未设置)"));
            m_importLabels[i]->setStyleSheet("color: #888; font-size: 11px;");
        } else {
            QFileInfo fi(m_importPaths[i]);
            m_importLabels[i]->setText(QStringLiteral("✔ ") + fi.fileName());
            m_importLabels[i]->setStyleSheet("color: #4caf50; font-size: 11px; font-weight: bold;");
        }
    }
}

void SettingsDialog::setImportPaths(const QString paths[5])
{
    for (int i = 0; i < 5; ++i) {
        m_importPaths[i] = paths[i];
        m_draftImportPaths[i] = paths[i];
    }
    m_importChanged = false;
    updateImportLabels();
}

void SettingsDialog::getImportPaths(QString paths[5]) const
{
    for (int i = 0; i < 5; ++i) {
        paths[i] = m_importPaths[i];
    }
}

void SettingsDialog::onOk()
{
    m_scalePercent = m_scaleSlider ? m_scaleSlider->value() : 100;
    const int selectedPet = currentPetIndex();
    const bool importChanged = m_importChanged;

    if (importChanged) {
        SpriteData imported = importedSprite();
        if (!imported.isEmpty()) {
            for (int i = 0; i < 5; ++i) m_importPaths[i] = m_draftImportPaths[i];
            emit spriteImported(imported);
        }
        m_importChanged = false;
    }

    // An explicit selection wins; importing alone selects the newly added pet.
    if (selectedPet >= 0 && (!importChanged || selectedPet != m_currentPetIndex)) {
        emit petChanged(selectedPet);
    }

    emit scaleChanged(m_scalePercent / 100.0);
    accept();
}

void SettingsDialog::onCancel()
{
    for (int i = 0; i < 5; ++i) m_draftImportPaths[i] = m_importPaths[i];
    m_importChanged = false;
    updateImportLabels();
    QDialog::reject();
}

void SettingsDialog::reject()
{
    onCancel();
}
