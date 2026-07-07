#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QVector>
#include <QLabel>
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

    // Returns the newly built SpriteData from import selections
    SpriteData importedSprite() const;
    // Persistence: get/set stored import paths
    void setImportPaths(const QString paths[5]);
    void getImportPaths(QString paths[5]) const;

signals:
    void petChanged(int index);
    void spriteImported(SpriteData data);
    void scaleChanged(qreal factor);

private slots:
    void onImportState(int stateIndex);
    void onOk();

private:
    void setupUi();
    void updateImportLabels();

    QButtonGroup *m_petGroup = nullptr;
    QSlider *m_scaleSlider = nullptr;
    QLabel *m_importLabels[5] = {};  // status label per state
    QString m_importPaths[5];         // selected file/folder per state
    int m_currentPetIndex = -1;
    int m_scalePercent = 100;

    static const char *stateFolderName(int s);
    static PetState stateFromIndex(int i);
};

#endif // SETTINGSDIALOG_H
