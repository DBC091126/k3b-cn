// CopyDiscPage.cpp — 光盘复制页实现
#include "CopyDiscPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QMessageBox>
#include <QGroupBox>

#include "../Backend.h"
#include "../Settings.h"
#include "../Icons.h"

namespace Burn {

CopyDiscPage::CopyDiscPage(Backend *backend, QWidget *parent)
    : QWidget(parent), m_backend(backend)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(12);

    auto *title = new QLabel(QStringLiteral("复制光盘"), this);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);
    layout->addWidget(new QLabel(
        QStringLiteral("将源光盘完整复制到目标光盘。自动模式下数据盘使用 dd 读整盘，音频盘请选择“仅音频”使用 cdrdao。"),
        this));

    auto *box = new QGroupBox(QStringLiteral("复制选项"), this);
    auto *form = new QFormLayout(box);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setHorizontalSpacing(16);

    m_srcCombo = new QComboBox(box);
    m_dstCombo = new QComboBox(box);
    m_modeCombo = new QComboBox(box);
    m_speedCombo = new QComboBox(box);
    m_ejectCheck = new QCheckBox(QStringLiteral("刻录完成后弹出光盘"), box);
    m_ejectCheck->setChecked(Settings::instance().ejectAfterBurn);

    m_modeCombo->addItem(QStringLiteral("自动检测"), static_cast<int>(CopyMode::Auto));
    m_modeCombo->addItem(QStringLiteral("仅数据盘"), static_cast<int>(CopyMode::DataOnly));
    m_modeCombo->addItem(QStringLiteral("仅音频盘"), static_cast<int>(CopyMode::AudioOnly));

    form->addRow(QStringLiteral("源设备："), m_srcCombo);
    form->addRow(QStringLiteral("目标设备："), m_dstCombo);
    form->addRow(QStringLiteral("复制方式："), m_modeCombo);
    form->addRow(QStringLiteral("写入速度："), m_speedCombo);
    form->addRow(QString(), m_ejectCheck);
    layout->addWidget(box);

    m_sourceStatus = new QLabel(this);
    m_sourceStatus->setStyleSheet(QStringLiteral("color:#7a7d85;"));
    layout->addWidget(m_sourceStatus);

    layout->addStretch();

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto *btnCopy = new QPushButton(Icons::play(QColor(255, 255, 255)),
                                    QStringLiteral("开始复制"), this);
    btnCopy->setStyleSheet(QStringLiteral(
        "QPushButton{background:#34a853;color:white;border:none;"
        "padding:9px 24px;border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#2c8f47;}"));
    btnRow->addWidget(btnCopy);
    layout->addLayout(btnRow);

    connect(btnCopy, &QPushButton::clicked, this, &CopyDiscPage::startCopy);
    connect(m_backend, &Backend::backendChanged,
            this, &CopyDiscPage::onBackendChanged);
    onBackendChanged();
}

void CopyDiscPage::onBackendChanged()
{
    const QString prevSrc = m_srcCombo->currentText();
    m_srcCombo->clear();
    m_dstCombo->clear();
    for (const QString &d : m_backend->devices) {
        m_srcCombo->addItem(d);
        m_dstCombo->addItem(d);
    }
    if (m_backend->devices.isEmpty()) {
        m_srcCombo->addItem(QStringLiteral("（无光驱）"));
        m_dstCombo->addItem(QStringLiteral("（无光驱）"));
    }
    if (!prevSrc.isEmpty())
        m_srcCombo->setCurrentText(prevSrc);
    const QString def = Settings::instance().device;
    if (m_srcCombo->findText(def) >= 0)
        m_srcCombo->setCurrentText(def);
    if (m_dstCombo->findText(def) >= 0)
        m_dstCombo->setCurrentText(def);

    m_speedCombo->clear();
    QStringList speeds = speedPresetOptions();
    for (const QString &s : m_backend->burningSpeeds(def))
        if (!speeds.contains(s))
            speeds << s;
    m_speedCombo->addItems(speeds);
    if (!Settings::instance().writeSpeed.isEmpty()) {
        const int i = m_speedCombo->findText(Settings::instance().writeSpeed);
        if (i >= 0)
            m_speedCombo->setCurrentIndex(i);
    }

    if (m_backend->devices.isEmpty()) {
        m_sourceStatus->setText(QStringLiteral("未检测到光驱，复制将以演示模式进行。"));
    } else {
        m_sourceStatus->setText(QStringLiteral("已检测到光驱：%1")
                                    .arg(m_backend->devices.join(QStringLiteral("、"))));
    }
}

void CopyDiscPage::startCopy()
{
    const QString src = m_srcCombo->currentText();
    const QString dst = m_dstCombo->currentText();
    if (src.isEmpty() || dst.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("复制光盘"),
                             QStringLiteral("请选择源设备与目标设备。"));
        return;
    }

    const auto r = QMessageBox::question(
        this, QStringLiteral("复制光盘"),
        QStringLiteral("将把 %1 中的光盘复制到 %2。\n"
                       "如果源盘与目标盘是同一个设备，请在提示后先取出源盘再放入空白盘。\n\n"
                       "确定开始吗？")
            .arg(src, dst),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes)
        return;

    Settings::instance().ejectAfterBurn = m_ejectCheck->isChecked();
    Settings::instance().writeSpeed = m_speedCombo->currentText();
    Settings::save();

    const auto mode = static_cast<CopyMode>(
        m_modeCombo->currentData().toInt());
    emit burnRequested(QStringLiteral("正在复制光盘：%1 → %2").arg(src, dst));
    m_backend->copyDisc(src, dst, m_speedCombo->currentText(),
                        m_ejectCheck->isChecked(), mode);
}

} // namespace Burn
