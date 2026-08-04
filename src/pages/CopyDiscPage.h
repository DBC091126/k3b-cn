// CopyDiscPage.h — 光盘复制页
#pragma once

#include <QWidget>

class QComboBox;
class QCheckBox;
class QLabel;

namespace Burn {

class Backend;

class CopyDiscPage : public QWidget
{
    Q_OBJECT
public:
    explicit CopyDiscPage(Backend *backend, QWidget *parent = nullptr);

signals:
    void burnRequested(const QString &title);

private slots:
    void startCopy();
    void onBackendChanged();

private:
    Backend *m_backend;
    QComboBox *m_srcCombo;
    QComboBox *m_dstCombo;
    QComboBox *m_modeCombo;
    QComboBox *m_speedCombo;
    QCheckBox *m_ejectCheck;
    QLabel *m_sourceStatus;
};

} // namespace Burn
