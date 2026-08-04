// ErasePage.h — 擦除可重写光盘页
#pragma once

#include <QWidget>

class QComboBox;
class QRadioButton;
class QLabel;
class QGroupBox;

namespace Burn {

class Backend;

class ErasePage : public QWidget
{
    Q_OBJECT
public:
    explicit ErasePage(Backend *backend, QWidget *parent = nullptr);

signals:
    void burnRequested(const QString &title);

private slots:
    void startErase();
    void onBackendChanged();

private:
    Backend *m_backend;
    QComboBox *m_deviceCombo;
    QRadioButton *m_fastRadio;
    QRadioButton *m_fullRadio;
    QRadioButton *m_forceRadio;
    QLabel *m_statusLabel;
};

} // namespace Burn
