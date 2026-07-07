#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QVector>
#include "SpriteData.h"

class QButtonGroup;
class QSlider;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    void setPets(const QVector<SpriteData> &pets, int currentIndex);
    int currentPetIndex() const;
    int scalePercent() const;

signals:
    void petChanged(int index);
    void spriteImported(SpriteData data);
    void scaleChanged(qreal factor);

private slots:
    void onImportState(int stateIndex);
    void onOk();
    void onCancel();

private:
    void setupUi();

    QButtonGroup *m_petGroup = nullptr;
    QSlider *m_scaleSlider = nullptr;
    QVector<SpriteData> m_importData;  // per-state mapping for import
    int m_currentPetIndex = -1;
    int m_scalePercent = 100;
};

#endif // SETTINGSDIALOG_H
