// ErasePage.cpp — 擦除页实现
#include "ErasePage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QRadioButton>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QButtonGroup>

#include "../Backend.h"
#include "../Settings.h"
#include "../Icons.h"

namespace Burn {

ErasePage::ErasePage(Backend *backend, QWidget *parent)
    : QWidget(parent), m_backend(backend)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(12);

    auto *title = new QLabel(QStringLiteral("擦除可重写光盘"), this);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    layout->addWidget(new QLabel(
        QStringLiteral("擦除 CD-RW、DVD±RW、DVD-RAM、BD-RE 等可重写介质。擦除后光盘将恢复为空白状态。"),
        this));

    auto *box = new QGroupBox(QStringLiteral("擦除选项"), this);
    auto *form = new QFormLayout(box);
    form->setHorizontalSpacing(16);

    m_deviceCombo = new QComboBox(box);
    m_deviceCombo->setMinimumWidth(200);
    form->addRow(QStringLiteral("设备："), m_deviceCombo);

    m_fastRadio = new QRadioButton(QStringLiteral("快速擦除（几秒钟）"), box);
    m_fullRadio = new QRadioButton(QStringLiteral("完全擦除（耗时较长，更彻底）"), box);
    m_forceRadio = new QRadioButton(QStringLiteral("强制重新格式化（DVD+RW / BD-RE）"), box);
    m_fastRadio->setChecked(true);

    auto *modeGroup = new QButtonGroup(this);
    modeGroup->addButton(m_fastRadio, 0);
    modeGroup->addButton(m_fullRadio, 1);
    modeGroup->addButton(m_forceRadio, 2);

    auto *modeBox = new QVBoxLayout;
    modeBox->addWidget(m_fastRadio);
    modeBox->addWidget(m_fullRadio);
    modeBox->addWidget(m_forceRadio);
    form->addRow(QStringLiteral("擦除方式："), modeBox);
    layout->addWidget(box);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#7a7d85;"));
    layout->addWidget(m_statusLabel);
    layout->addStretch();

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto *btnErase = new QPushButton(Icons::erase(), QStringLiteral("开始擦除"), this);
    btnErase->setStyleSheet(QStringLiteral(
        "QPushButton{background:#d9822b;color:white;border:none;"
        "padding:9px 24px;border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#c26d17;}"));
    btnRow->addWidget(btnErase);
    layout->addLayout(btnRow);

    connect(btnErase, &QPushButton::clicked, this, &ErasePage::startErase);
    connect(m_backend, &Backend::backendChanged,
            this, &ErasePage::onBackendChanged);
    onBackendChanged();
}

void ErasePage::onBackendChanged()
{
    const QString prev = m_deviceCombo->currentText();
    m_deviceCombo->clear();
    for (const QString &d : m_backend->devices)
        m_deviceCombo->addItem(d);
    if (m_backend->devices.isEmpty())
        m_deviceCombo->addItem(QStringLiteral("（无光驱）"));
    if (!prev.isEmpty())
        m_deviceCombo->setCurrentText(prev);
    const QString def = Settings::instance().device;
    if (m_deviceCombo->findText(def) >= 0)
        m_deviceCombo->setCurrentText(def);

    if (m_backend->devices.isEmpty())
        m_statusLabel->setText(QStringLiteral("未检测到光驱。"));
    else if (m_backend->mediaStatus().blank)
        m_statusLabel->setText(QStringLiteral("当前介质为空白盘，无需擦除。"));
    else if (m_backend->mediaStatus().erasable)
        m_statusLabel->setText(QStringLiteral("当前介质可擦除。"));
    else
        m_statusLabel->setText(QStringLiteral("当前介质不可擦除（可能是只读盘）。"));
}

void ErasePage::startErase()
{
    const QString dev = m_deviceCombo->currentText();
    if (dev == QStringLiteral("（无光驱）") || dev.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("擦除光盘"),
                             QStringLiteral("请选择有效的光驱设备。"));
        return;
    }

    EraseMode mode;
    QString modeText;
    if (m_fullRadio->isChecked()) {
        mode = EraseMode::Full;
        modeText = QStringLiteral("完全擦除");
    } else if (m_forceRadio->isChecked()) {
        mode = EraseMode::FormatForce;
        modeText = QStringLiteral("强制重新格式化");
    } else {
        mode = EraseMode::Fast;
        modeText = QStringLiteral("快速擦除");
    }

    const auto r = QMessageBox::warning(
        this, QStringLiteral("确认擦除"),
        QStringLiteral("将对 %1 执行“%2”。\n光盘上的所有数据将被清除且无法恢复！\n\n确定继续吗？")
            .arg(dev, modeText),
        QMessageBox::Yes | QMessageBox::Cancel);
    if (r != QMessageBox::Yes)
        return;

    emit burnRequested(QStringLiteral("正在擦除 %1 上的光盘").arg(dev));
    m_backend->erase(dev, mode);
}

} // namespace Burn
