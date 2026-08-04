// IsoToolsPage.h — ISO 镜像工具页（制作 / 烧录）
#pragma once

#include <QWidget>

class QListWidget;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QLabel;

namespace Burn {

class Backend;

class IsoToolsPage : public QWidget
{
    Q_OBJECT
public:
    explicit IsoToolsPage(Backend *backend, QWidget *parent = nullptr);

signals:
    void burnRequested(const QString &title);

private slots:
    void addIsoFiles();
    void addIsoFolder();
    void removeIsoEntry();
    void browseOutput();
    void createIso();
    void browseBurnIso();
    void burnIso();

private:
    void refreshDevices();

    Backend *m_backend;

    // 制作镜像
    QListWidget *m_sourceList;
    QLineEdit *m_volumeEdit;
    QLineEdit *m_outputEdit;

    // 烧录镜像
    QLineEdit *m_isoEdit;
    QComboBox *m_burnDeviceCombo;
    QComboBox *m_speedCombo;
    QCheckBox *m_ejectCheck;
    QLabel *m_statusLabel;
};

} // namespace Burn
