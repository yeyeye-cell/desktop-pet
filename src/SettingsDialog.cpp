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
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
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

    auto *presetGroup = findChild<QGroupBox*>(QStringLiteral("选择预设宠物"));
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

SpriteData SettingsDialog::importedSprite() const
{
    SpriteData data;
    bool hasAny = false;

    for (int i = 0; i < 5; ++i) {
        if (m_importPaths[i].isEmpty()) continue;

        QFileInfo fi(m_importPaths[i]);
        QVector<QPixmap> frames;

        if (fi.suffix().toLower() == "gif") {
            QImageReader reader(m_importPaths[i], "gif");
            int count = reader.imageCount();
            if (count == 0) {
                while (true) {
                    QImage img = reader.read();
                    if (img.isNull()) break;
                    frames.append(QPixmap::fromImage(img));
                    if (!reader.jumpToNextImage()) break;
                }
            } else {
                for (int f = 0; f < count; ++f) {
                    reader.jumpToImage(f);
                    QImage img = reader.read();
                    if (!img.isNull()) frames.append(QPixmap::fromImage(img));
                }
            }
        } else if (fi.isDir()) {
            QDir dir(m_importPaths[i]);
            QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
            for (const auto &f : dir.entryList(filters, QDir::Files, QDir::Name)) {
                QPixmap px(dir.absoluteFilePath(f));
                if (!px.isNull()) frames.append(px);
            }
        } else {
            QPixmap px(m_importPaths[i]);
            if (!px.isNull()) frames.append(px);
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

    m_importPaths[stateIndex] = path;
    QFileInfo fi(path);
    m_importLabels[stateIndex]->setText(QStringLiteral("✔ ") + fi.fileName());
    m_importLabels[stateIndex]->setStyleSheet("color: #4caf50; font-size: 11px; font-weight: bold;");

    qDebug() << "Import state" << stateIndex << "(" << stateFolderName(stateIndex) << "):" << path;
}

void SettingsDialog::onOk()
{
    m_scalePercent = m_scaleSlider ? m_scaleSlider->value() : 100;

    // If user imported custom sprite, emit it
    SpriteData imported = importedSprite();
    if (!imported.isEmpty()) {
        emit spriteImported(imported);
    }

    int idx = currentPetIndex();
    if (idx >= 0) {
        emit petChanged(idx);
    }

    emit scaleChanged(m_scalePercent / 100.0);
    accept();
}
