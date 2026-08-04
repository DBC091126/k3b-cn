// IsoToolsPage.cpp — ISO 镜像工具页实现
#include "IsoToolsPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>

#include "../Backend.h"
#include "../Settings.h"
#include "../Projects.h"
#include "../Icons.h"

namespace Burn {

IsoToolsPage::IsoToolsPage(Backend *backend, QWidget *parent)
    : QWidget(parent), m_backend(backend)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);

    auto *tabs = new QTabWidget(this);

    // ---------- 制作镜像 ----------
    auto *makePage = new QWidget(tabs);
    auto *makeLayout = new QVBoxLayout(makePage);
    makeLayout->setContentsMargins(12, 12, 12, 12);

    auto *btnAddFile = new QPushButton(QStringLiteral("添加文件…"), makePage);
    auto *btnAddFolder = new QPushButton(QStringLiteral("添加文件夹…"), makePage);
    auto *btnRemove = new QPushButton(QStringLiteral("移除选中"), makePage);
    auto *srcRow = new QHBoxLayout;
    srcRow->addWidget(new QLabel(QStringLiteral("源内容："), makePage));
    srcRow->addStretch();
    srcRow->addWidget(btnAddFile);
    srcRow->addWidget(btnAddFolder);
    srcRow->addWidget(btnRemove);
    makeLayout->addLayout(srcRow);

    m_sourceList = new QListWidget(makePage);
    m_sourceList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    makeLayout->addWidget(m_sourceList, 1);

    auto *form = new QFormLayout;
    form->setHorizontalSpacing(12);
    m_volumeEdit = new QLineEdit(QStringLiteral("MY_DATA"), makePage);
    m_outputEdit = new QLineEdit(makePage);
    auto *btnBrowse = new QPushButton(QStringLiteral("浏览…"), makePage);
    auto *outRow = new QHBoxLayout;
    outRow->addWidget(m_outputEdit, 1);
    outRow->addWidget(btnBrowse);
    form->addRow(QStringLiteral("卷标："), m_volumeEdit);
    form->addRow(QStringLiteral("输出镜像："), outRow);
    makeLayout->addLayout(form);

    auto *btnCreate = new QPushButton(QStringLiteral("生成 ISO 镜像"), makePage);
    btnCreate->setStyleSheet(QStringLiteral(
        "QPushButton{background:#2979ff;color:white;border:none;"
        "padding:8px 20px;border-radius:4px;font-weight:bold;}"));
    auto *createRow = new QHBoxLayout;
    createRow->addStretch();
    createRow->addWidget(btnCreate);
    makeLayout->addLayout(createRow);

    connect(btnAddFile, &QPushButton::clicked, this, &IsoToolsPage::addIsoFiles);
    connect(btnAddFolder, &QPushButton::clicked, this, &IsoToolsPage::addIsoFolder);
    connect(btnRemove, &QPushButton::clicked, this, &IsoToolsPage::removeIsoEntry);
    connect(btnBrowse, &QPushButton::clicked, this, &IsoToolsPage::browseOutput);
    connect(btnCreate, &QPushButton::clicked, this, &IsoToolsPage::createIso);

    // ---------- 烧录镜像 ----------
    auto *burnPage = new QWidget(tabs);
    auto *burnLayout = new QVBoxLayout(burnPage);
    burnLayout->setContentsMargins(12, 12, 12, 12);

    auto *g = new QGroupBox(QStringLiteral("烧录选项"), burnPage);
    auto *bf = new QFormLayout(g);
    bf->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    bf->setHorizontalSpacing(12);

    m_isoEdit = new QLineEdit(burnPage);
    auto *btnBrowseIso = new QPushButton(QStringLiteral("浏览…"), burnPage);
    auto *isoRow = new QHBoxLayout;
    isoRow->addWidget(m_isoEdit, 1);
    isoRow->addWidget(btnBrowseIso);
    m_burnDeviceCombo = new QComboBox(burnPage);
    m_speedCombo = new QComboBox(burnPage);
    m_speedCombo->addItem(QStringLiteral("自动"));
    m_ejectCheck = new QCheckBox(QStringLiteral("刻录完成后弹出光盘"), burnPage);
    m_ejectCheck->setChecked(Settings::instance().ejectAfterBurn);

    bf->addRow(QStringLiteral("ISO 镜像："), isoRow);
    bf->addRow(QStringLiteral("目标设备："), m_burnDeviceCombo);
    bf->addRow(QStringLiteral("写入速度："), m_speedCombo);
    bf->addRow(QString(), m_ejectCheck);
    burnLayout->addWidget(g);

    auto *btnBurnIso = new QPushButton(Icons::burn(), QStringLiteral("烧录 ISO 镜像"), burnPage);
    btnBurnIso->setStyleSheet(QStringLiteral(
        "QPushButton{background:#e84393;color:white;border:none;"
        "padding:9px 22px;border-radius:4px;font-weight:bold;}"));
    auto *burnRow = new QHBoxLayout;
    burnRow->addStretch();
    burnRow->addWidget(btnBurnIso);
    burnLayout->addLayout(burnRow);

    m_statusLabel = new QLabel(burnPage);
    m_statusLabel->setStyleSheet(QStringLiteral("color:#7a7d85;"));
    burnLayout->addWidget(m_statusLabel);
    burnLayout->addStretch();

    connect(btnBrowseIso, &QPushButton::clicked, this, &IsoToolsPage::browseBurnIso);
    connect(btnBurnIso, &QPushButton::clicked, this, &IsoToolsPage::burnIso);
    connect(m_backend, &Backend::backendChanged, this, &IsoToolsPage::refreshDevices);

    tabs->addTab(makePage, QStringLiteral("制作镜像"));
    tabs->addTab(burnPage, QStringLiteral("烧录镜像"));
    layout->addWidget(tabs);

    refreshDevices();
}

void IsoToolsPage::refreshDevices()
{
    const QString prev = m_burnDeviceCombo->currentText();
    m_burnDeviceCombo->clear();
    for (const QString &d : m_backend->devices)
        m_burnDeviceCombo->addItem(d);
    if (m_backend->devices.isEmpty())
        m_burnDeviceCombo->addItem(QStringLiteral("（无光驱）"));
    if (!prev.isEmpty())
        m_burnDeviceCombo->setCurrentText(prev);
    const QString def = Settings::instance().device;
    if (m_burnDeviceCombo->findText(def) >= 0)
        m_burnDeviceCombo->setCurrentText(def);

    m_speedCombo->clear();
    QStringList speeds = speedPresetOptions();
    for (const QString &s : m_backend->burningSpeeds(def))
        if (!speeds.contains(s))
            speeds << s;
    m_speedCombo->addItems(speeds);

    if (m_backend->demoMode)
        m_statusLabel->setText(QStringLiteral("演示模式：未检测到刻录工具/光驱，将模拟烧录。"));
    else
        m_statusLabel->setText(QStringLiteral("烧录工具就绪。"));
}

void IsoToolsPage::addIsoFiles()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, QStringLiteral("选择文件"), QDir::homePath());
    for (const QString &p : files)
        m_sourceList->addItem(p);
}

void IsoToolsPage::addIsoFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择文件夹"), QDir::homePath());
    if (!dir.isEmpty())
        m_sourceList->addItem(dir);
}

void IsoToolsPage::removeIsoEntry()
{
    qDeleteAll(m_sourceList->selectedItems());
}

void IsoToolsPage::browseOutput()
{
    const QString p = QFileDialog::getSaveFileName(
        this, QStringLiteral("选择输出镜像"), QDir::homePath() + QStringLiteral("/my_data.iso"),
        QStringLiteral("ISO 镜像 (*.iso)"));
    if (!p.isEmpty())
        m_outputEdit->setText(p);
}

void IsoToolsPage::createIso()
{
    if (m_sourceList->count() == 0) {
        QMessageBox::information(this, QStringLiteral("生成 ISO"),
                                 QStringLiteral("请先添加要制作成镜像的文件或文件夹。"));
        return;
    }
    const QString out = m_outputEdit->text().trimmed();
    if (out.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("生成 ISO"),
                             QStringLiteral("请选择输出镜像文件的路径。"));
        return;
    }
    QStringList sources;
    for (int i = 0; i < m_sourceList->count(); ++i)
        sources << m_sourceList->item(i)->text();

    const QString title = QStringLiteral("正在制作 ISO 镜像：%1").arg(out);
    emit burnRequested(title);
    m_backend->createIso(sources, out,
                         m_volumeEdit->text().trimmed().isEmpty()
                             ? QStringLiteral("MY_DATA")
                             : m_volumeEdit->text().trimmed(),
                         true, true);
}

void IsoToolsPage::browseBurnIso()
{
    const QString p = QFileDialog::getOpenFileName(
        this, QStringLiteral("选择 ISO 镜像"),
        QDir::homePath(), QStringLiteral("ISO 镜像 (*.iso);;所有文件 (*)"));
    if (!p.isEmpty())
        m_isoEdit->setText(p);
}

void IsoToolsPage::burnIso()
{
    const QString iso = m_isoEdit->text().trimmed();
    if (iso.isEmpty() || !QFileInfo::exists(iso)) {
        QMessageBox::warning(this, QStringLiteral("烧录 ISO"),
                             QStringLiteral("请选择存在的 ISO 镜像文件。"));
        return;
    }
    Settings::instance().ejectAfterBurn = m_ejectCheck->isChecked();
    Settings::instance().writeSpeed = m_speedCombo->currentText();
    Settings::save();

    emit burnRequested(QStringLiteral("正在烧录镜像：%1").arg(iso));
    m_backend->burnIso(m_burnDeviceCombo->currentText(), iso,
                       m_speedCombo->currentText(), m_ejectCheck->isChecked());
}

} // namespace Burn
