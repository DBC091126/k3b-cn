// BurnOptionsDialog.h — 刻录设置对话框
#pragma once

#include <QDialog>

class QComboBox;
class QCheckBox;
class QLineEdit;

namespace Burn {

class Backend;

class BurnOptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BurnOptionsDialog(Backend *backend, QWidget *parent = nullptr);

private slots:
    void accept() override;

private:
    Backend *m_backend;
    QComboBox *m_deviceCombo;
    QComboBox *m_speedCombo;
    QComboBox *m_writeModeCombo;
    QCheckBox *m_ejectCheck;
    QCheckBox *m_verifyCheck;
    QCheckBox *m_multiCheck;
    QLineEdit *m_tempDirEdit;
};

} // namespace Burn
