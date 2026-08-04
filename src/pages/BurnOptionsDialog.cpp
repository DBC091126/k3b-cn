// BurnOptionsDialog.cpp — 刻录设置对话框实现
#include "BurnOptionsDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QDialogButtonBox>

#include "../Backend.h"
#include "../Settings.h"

namespace Burn {

BurnOptionsDialog::BurnOptionsDialog(Backend *backend, QWidget *parent)
    : QDialog(parent), m_backend(backend)
{
    setWindowTitle(QStringLiteral("刻录设置"));
    setMinimumWidth(440);

    auto *layout = new QVBoxLayout(this);

    auto *g1 = new QGroupBox(QStringLiteral("刻录"), this);
    auto *f1 = new QFormLayout(g1);
    f1->setHorizontalSpacing(14);

    m_deviceCombo = new QComboBox(g1);
    for (const QString &d : m_backend->devices)
        m_deviceCombo->addItem(d);
    if (m_backend->devices.isEmpty())
        m_deviceCombo->addItem(QStringLiteral("（无光驱）"));
    m_deviceCombo->setCurrentText(Settings::instance().device);

    m_speedCombo = new QComboBox(g1);
    QStringList speeds = speedPresetOptions();
    for (const QString &s : m_backend->burningSpeeds(Settings::instance().device))
        if (!speeds.contains(s))
            speeds << s;
    m_speedCombo->addItems(speeds);
    if (!Settings::instance().writeSpeed.isEmpty())
        m_speedCombo->setCurrentText(Settings::instance().writeSpeed);

    m_writeModeCombo = new QComboBox(g1);
    m_writeModeCombo->addItem(QStringLiteral("自动"), 0);
    m_writeModeCombo->addItem(QStringLiteral("DAO（整盘写入）"), 1);
    m_writeModeCombo->addItem(QStringLiteral("TAO（逐轨写入）"), 2);
    m_writeModeCombo->addItem(QStringLiteral("RAW（原始模式）"), 3);
    m_writeModeCombo->setCurrentIndex(qBound(0, Settings::instance().writeMode, 3));

    m_ejectCheck = new QCheckBox(QStringLiteral("刻录完成后弹出光盘"), g1);
    m_ejectCheck->setChecked(Settings::instance().ejectAfterBurn);
    m_verifyCheck = new QCheckBox(QStringLiteral("刻录完成后校验数据"), g1);
    m_verifyCheck->setChecked(Settings::instance().verifyAfterBurn);
    m_multiCheck = new QCheckBox(QStringLiteral("允许多会话刻录（数据盘）"), g1);
    m_multiCheck->setChecked(Settings::instance().multiSession);

    f1->addRow(QStringLiteral("默认刻录设备："), m_deviceCombo);
    f1->addRow(QStringLiteral("写入速度："), m_speedCombo);
    f1->addRow(QStringLiteral("写入模式："), m_writeModeCombo);
    f1->addRow(QString(), m_ejectCheck);
    f1->addRow(QString(), m_verifyCheck);
    f1->addRow(QString(), m_multiCheck);
    layout->addWidget(g1);

    auto *g2 = new QGroupBox(QStringLiteral("临时文件"), this);
    auto *f2 = new QFormLayout(g2);
    m_tempDirEdit = new QLineEdit(Settings::instance().tempImageDir, g2);
    auto *btnBrowse = new QPushButton(QStringLiteral("浏览…"), g2);
    auto *row = new QHBoxLayout;
    row->addWidget(m_tempDirEdit, 1);
    row->addWidget(btnBrowse);
    f2->addRow(QStringLiteral("临时镜像目录："), row);
    layout->addWidget(g2);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok |
                                      QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Ok)->setText(QStringLiteral("保存"));
    btns->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    layout->addWidget(btns);

    connect(btnBrowse, &QPushButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(
            this, QStringLiteral("选择临时镜像目录"), m_tempDirEdit->text());
        if (!dir.isEmpty())
            m_tempDirEdit->setText(dir);
    });
    connect(btns, &QDialogButtonBox::accepted, this, &BurnOptionsDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void BurnOptionsDialog::accept()
{
    Settings &s = Settings::instance();
    s.device = m_deviceCombo->currentText();
    s.writeSpeed = m_speedCombo->currentText();
    s.writeMode = m_writeModeCombo->currentData().toInt();
    s.ejectAfterBurn = m_ejectCheck->isChecked();
    s.verifyAfterBurn = m_verifyCheck->isChecked();
    s.multiSession = m_multiCheck->isChecked();
    s.tempImageDir = m_tempDirEdit->text().trimmed();
    Settings::save();
    QDialog::accept();
}

} // namespace Burn
