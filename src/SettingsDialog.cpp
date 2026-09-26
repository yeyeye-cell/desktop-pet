#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QSlider>
#include <QLabel>
#include <QGroupBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("宠物设置"));
    setMinimumSize(360, 220);
    setupUi();
}

void SettingsDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *petGroup = new QGroupBox(QStringLiteral("选择宠物"), this);
    petGroup->setObjectName(QStringLiteral("petGroup"));
    auto *petLayout = new QHBoxLayout(petGroup);
    m_petGroup = new QButtonGroup(this);
    m_petGroup->setExclusive(true);
    petLayout->addStretch();
    mainLayout->addWidget(petGroup);

    auto *scaleGroup = new QGroupBox(QStringLiteral("缩放"), this);
    auto *scaleLayout = new QHBoxLayout(scaleGroup);
    m_scaleSlider = new QSlider(Qt::Horizontal, this);
    m_scaleSlider->setRange(20, 200);
    m_scaleSlider->setValue(100);
    auto *scaleLabel = new QLabel("100%", this);
    connect(m_scaleSlider, &QSlider::valueChanged, this, [scaleLabel](int value) {
        scaleLabel->setText(QString("%1%").arg(value));
    });
    scaleLayout->addWidget(m_scaleSlider);
    scaleLayout->addWidget(scaleLabel);
    mainLayout->addWidget(scaleGroup);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    auto *okButton = new QPushButton(QStringLiteral("确定"), this);
    auto *cancelButton = new QPushButton(QStringLiteral("取消"), this);
    connect(okButton, &QPushButton::clicked, this, &SettingsDialog::onOk);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);
}

void SettingsDialog::setPets(const QVector<SpriteData> &pets, int currentIndex)
{
    for (auto *button : m_petGroup->buttons()) {
        m_petGroup->removeButton(button);
        delete button;
    }

    auto *petGroup = findChild<QGroupBox *>(QStringLiteral("petGroup"));
    if (!petGroup) return;
    auto *petLayout = petGroup->layout();

    for (int i = 0; i < pets.size(); ++i) {
        auto *button = new QPushButton(pets[i].name, this);
        button->setCheckable(true);
        if (i == currentIndex) button->setChecked(true);
        m_petGroup->addButton(button, i);
        petLayout->addWidget(button);
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

void SettingsDialog::setScalePercent(int percent)
{
    if (m_scaleSlider) m_scaleSlider->setValue(qBound(20, percent, 200));
}

void SettingsDialog::onOk()
{
    const int petIndex = currentPetIndex();
    if (petIndex >= 0) emit petChanged(petIndex);
    emit scaleChanged(scalePercent() / 100.0);
    accept();
}
