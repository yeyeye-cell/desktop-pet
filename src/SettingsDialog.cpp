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
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("宠物设置"));
    setMinimumSize(400, 400);
    setupUi();
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
    // Buttons will be populated in setPets()
    mainLayout->addWidget(presetGroup);

    // --- Import section ---
    auto *importGroup = new QGroupBox(QStringLiteral("导入自定义素材"), this);
    auto *importLayout = new QFormLayout(importGroup);

    const char *stateNames[] = {"待机 (Idle)", "走路 (Walk)", "点击 (Click)", "开心 (Happy)", "睡觉 (Sleep)"};
    for (int i = 0; i < 5; ++i) {
        auto *btn = new QPushButton(QStringLiteral("选择文件..."), this);
        connect(btn, &QPushButton::clicked, this, [this, i]() { onImportState(i); });
        importLayout->addRow(new QLabel(QString::fromUtf8(stateNames[i]), this), btn);
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
    // Clear existing buttons
    for (auto *btn : m_petGroup->buttons()) {
        m_petGroup->removeButton(btn);
        delete btn;
    }

    for (int i = 0; i < pets.size(); ++i) {
        auto *btn = new QPushButton(pets[i].name, this);
        btn->setCheckable(true);
        btn->setChecked(i == currentIndex);
        m_petGroup->addButton(btn, i);
        // The parent layout manages the button
        auto *presetGroup = findChild<QGroupBox*>();
        if (presetGroup) {
            presetGroup->layout()->addWidget(btn);
        }
    }

    // Add import button
    auto *importBtn = new QPushButton(QStringLiteral("＋ 导入"), this);
    m_petGroup->addButton(importBtn, pets.size());
    auto *presetGroup = findChild<QGroupBox*>();
    if (presetGroup) {
        presetGroup->layout()->addWidget(importBtn);
    }
}

int SettingsDialog::currentPetIndex() const
{
    return m_petGroup ? m_petGroup->checkedId() : -1;
}

int SettingsDialog::scalePercent() const
{
    return m_scaleSlider ? m_scaleSlider->value() : 100;
}

void SettingsDialog::onImportState(int stateIndex)
{
    QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择动画文件"),
        QString(),
        QStringLiteral("Images (*.png *.jpg *.bmp *.gif);;All Files (*)")
    );
    if (path.isEmpty()) return;
    qDebug() << "Import state" << stateIndex << "from" << path;
    // Full import logic will be refined later
}

void SettingsDialog::onOk()
{
    accept();
}

void SettingsDialog::onCancel()
{
    reject();
}
