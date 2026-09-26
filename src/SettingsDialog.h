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
    void setScalePercent(int percent);

signals:
    void petChanged(int index);
    void scaleChanged(qreal factor);

private slots:
    void onOk();

private:
    void setupUi();

    QButtonGroup *m_petGroup = nullptr;
    QSlider *m_scaleSlider = nullptr;
};

#endif // SETTINGSDIALOG_H
