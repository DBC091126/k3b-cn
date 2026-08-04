// MediaInfoPage.cpp — 介质信息页实现
#include "MediaInfoPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>

#include "../Backend.h"
#include "../Settings.h"

namespace Burn {

MediaInfoPage::MediaInfoPage(Backend *backend, QWidget *parent)
    : QWidget(parent), m_backend(backend)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("介质信息"), this);
    QFont tf = title->font();
    tf.setPointSize(16);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    auto *topRow = new QHBoxLayout;
    topRow->addWidget(new QLabel(QStringLiteral("设备："), this));
    m_deviceCombo = new QComboBox(this);
    m_deviceCombo->setMinimumWidth(140);
    m_refreshBtn = new QPushButton(QStringLiteral("重新检测"), this);
    auto *btnEject = new QPushButton(QStringLiteral("弹出光盘"), this);
    topRow->addWidget(m_deviceCombo);
    topRow->addSpacing(12);
    topRow->addWidget(m_refreshBtn);
    topRow->addWidget(btnEject);
    topRow->addStretch();
    layout->addLayout(topRow);

    m_summary = new QLabel(this);
    m_summary->setStyleSheet(QStringLiteral("color:#7a7d85;"));
    layout->addWidget(m_summary);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({QStringLiteral("属性"), QStringLiteral("值")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    layout->addWidget(m_table, 1);

    connect(m_refreshBtn, &QPushButton::clicked, this, &MediaInfoPage::refresh);
    connect(btnEject, &QPushButton::clicked, this, &MediaInfoPage::eject);
    connect(m_deviceCombo, &QComboBox::currentTextChanged,
            this, [this](const QString &) { refresh(); });
    connect(m_backend, &Backend::backendChanged,
            this, &MediaInfoPage::onBackendChanged);
    connect(m_backend, &Backend::mediaChanged,
            this, &MediaInfoPage::refresh);

    onBackendChanged();
}

void MediaInfoPage::onBackendChanged()
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
    refresh();
}

void MediaInfoPage::refresh()
{
    m_table->setRowCount(0);
    const QString dev = m_deviceCombo->currentText();
    if (m_backend->devices.isEmpty() || dev.isEmpty() ||
        dev == QStringLiteral("（无光驱）")) {
        m_summary->setText(QStringLiteral("未检测到光盘驱动器。"));
        return;
    }

    const MediaStatus ms = m_backend->detectMedia(dev);

    const auto add = [this](const QString &k, const QString &v) {
        fillRow(k, v);
    };

    if (!ms.present) {
        add(QStringLiteral("设备"), dev);
        add(QStringLiteral("状态"), QStringLiteral("光驱中无光盘"));
        m_summary->setText(QStringLiteral("请放入一张光盘后点击“重新检测”。"));
        return;
    }

    add(QStringLiteral("设备"), dev);
    add(QStringLiteral("厂商"), ms.vendor);
    add(QStringLiteral("型号"), ms.product);
    add(QStringLiteral("介质类型"), ms.mediaType);
    add(QStringLiteral("介质类别"), mediaClassToText(ms.mediaClass));
    add(QStringLiteral("是否可写"), ms.writable ? QStringLiteral("是") : QStringLiteral("否"));
    add(QStringLiteral("是否空白"), ms.blank ? QStringLiteral("是") : QStringLiteral("否"));
    add(QStringLiteral("可续刻"), ms.appendable ? QStringLiteral("是") : QStringLiteral("否"));
    add(QStringLiteral("可擦除"), ms.erasable ? QStringLiteral("是") : QStringLiteral("否"));
    if (ms.capacityMB > 0)
        add(QStringLiteral("容量"), QStringLiteral("%1 MB（%2）")
                .arg(ms.capacityMB).arg(formatBytes(ms.capacityMB * 1024 * 1024)));
    if (ms.discStatus >= 0)
        add(QStringLiteral("wodim 状态码"), QString::number(ms.discStatus));

    QString status;
    if (ms.blank)
        status = QStringLiteral("空白可写光盘，可以开始刻录。");
    else if (ms.writable)
        status = QStringLiteral("已使用的可写光盘（%1）。").arg(ms.appendable ? QStringLiteral("可续刻") : QStringLiteral("已封闭"));
    else
        status = QStringLiteral("只读介质。");
    m_summary->setText(status);
}

void MediaInfoPage::fillRow(const QString &key, const QString &value)
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    auto *k = new QTableWidgetItem(key);
    auto *v = new QTableWidgetItem(value);
    k->setFlags(Qt::ItemIsEnabled);
    v->setFlags(Qt::ItemIsEnabled);
    m_table->setItem(row, 0, k);
    m_table->setItem(row, 1, v);
}

void MediaInfoPage::eject()
{
    const QString dev = m_deviceCombo->currentText();
    if (dev == QStringLiteral("（无光驱）") || dev.isEmpty())
        return;
    m_backend->ejectDisc(dev);
}

} // namespace Burn
