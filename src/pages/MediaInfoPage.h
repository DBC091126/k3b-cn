// MediaInfoPage.h — 介质信息页
#pragma once

#include <QWidget>

class QTableWidget;
class QComboBox;
class QLabel;
class QPushButton;

namespace Burn {

class Backend;

class MediaInfoPage : public QWidget
{
    Q_OBJECT
public:
    explicit MediaInfoPage(Backend *backend, QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onBackendChanged();
    void eject();

private:
    void fillRow(const QString &key, const QString &value);

    Backend *m_backend;
    QTableWidget *m_table;
    QComboBox *m_deviceCombo;
    QLabel *m_summary;
    QPushButton *m_refreshBtn;
};

} // namespace Burn
