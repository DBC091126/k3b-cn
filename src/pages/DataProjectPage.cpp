// DataProjectPage.cpp — 数据项目页实现
#include "DataProjectPage.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QProgressBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QStyle>

#include "../Backend.h"
#include "../Projects.h"
#include "../Settings.h"
#include "../Icons.h"

namespace Burn {

// ---------- EntryTree ----------
EntryTree::EntryTree(QWidget *parent)
    : QTreeWidget(parent)
{
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
}

void EntryTree::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void EntryTree::dragMoveEvent(QDragMoveEvent *e)
{
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void EntryTree::dropEvent(QDropEvent *e)
{
    if (!e->mimeData()->hasUrls())
        return;
    QVector<QUrl> urls;
    const auto list = e->mimeData()->urls();
    for (const QUrl &u : list)
        if (u.isLocalFile())
            urls << u;
    if (!urls.isEmpty())
        emit filesDropped(urls);
    e->acceptProposedAction();
}

// ---------- DataProjectPage ----------
DataProjectPage::DataProjectPage(Backend *backend, DataProject *project,
                                 QWidget *parent)
    : QWidget(parent), m_backend(backend), m_project(project)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(8);

    // 顶部工具条
    auto *toolRow = new QHBoxLayout;
    auto *btnAddFile = new QPushButton(QStringLiteral("添加文件…"), this);
    auto *btnAddFolder = new QPushButton(QStringLiteral("添加文件夹…"), this);
    auto *btnRemove = new QPushButton(QStringLiteral("移除选中"), this);
    auto *btnClear = new QPushButton(QStringLiteral("清空"), this);
    toolRow->addWidget(btnAddFile);
    toolRow->addWidget(btnAddFolder);
    toolRow->addWidget(btnRemove);
    toolRow->addWidget(btnClear);

    toolRow->addSpacing(24);
    toolRow->addWidget(new QLabel(QStringLiteral("卷标："), this));
    m_volumeEdit = new QLineEdit(m_project->volumeLabel, this);
    m_volumeEdit->setMaximumWidth(160);
    toolRow->addWidget(m_volumeEdit);

    toolRow->addSpacing(12);
    toolRow->addWidget(new QLabel(QStringLiteral("写入速度："), this));
    m_speedCombo = new QComboBox(this);
    m_speedCombo->addItem(QStringLiteral("自动"));
    m_speedCombo->setMaximumWidth(110);
    toolRow->addWidget(m_speedCombo);

    toolRow->addStretch();
    auto *btnBurn = new QPushButton(Icons::play(QColor(255, 255, 255)),
                                    QStringLiteral("开始烧录"), this);
    btnBurn->setStyleSheet(QStringLiteral(
        "QPushButton{background:#2979ff;color:white;border:none;"
        "padding:8px 20px;border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#1967d2;}"));
    toolRow->addWidget(btnBurn);
    layout->addLayout(toolRow);

    connect(btnAddFile, &QPushButton::clicked, this, &DataProjectPage::addFiles);
    connect(btnAddFolder, &QPushButton::clicked, this, &DataProjectPage::addFolder);
    connect(btnRemove, &QPushButton::clicked, this, &DataProjectPage::removeSelected);
    connect(btnClear, &QPushButton::clicked, this, &DataProjectPage::clearProject);
    connect(btnBurn, &QPushButton::clicked, this, &DataProjectPage::requestBurn);

    // 树
    m_tree = new EntryTree(this);
    m_tree->setColumnCount(4);
    m_tree->setHeaderLabels({QStringLiteral("名称"), QStringLiteral("大小"),
                             QStringLiteral("类型"), QStringLiteral("路径")});
    layout->addWidget(m_tree, 1);

    // 底部容量
    auto *capRow = new QHBoxLayout;
    m_sizeLabel = new QLabel(this);
    m_capacityLabel = new QLabel(this);
    m_capacityBar = new QProgressBar(this);
    m_capacityBar->setRange(0, 100);
    m_capacityBar->setTextVisible(false);
    m_capacityBar->setMaximumHeight(14);
    capRow->addWidget(m_sizeLabel);
    capRow->addSpacing(12);
    capRow->addWidget(m_capacityBar, 1);
    capRow->addSpacing(12);
    capRow->addWidget(m_capacityLabel);
    layout->addLayout(capRow);

    connect(m_tree, &EntryTree::filesDropped,
            this, &DataProjectPage::onFilesDropped);
    connect(m_backend, &Backend::mediaChanged,
            this, &DataProjectPage::onMediaChanged);
    connect(m_backend, &Backend::backendChanged,
            this, &DataProjectPage::onMediaChanged);

    // 速度列表：预置（自动/1x/4x）+ 光驱检测到的速度
    QStringList speeds = speedPresetOptions();
    for (const QString &s : m_backend->burningSpeeds(Settings::instance().device))
        if (!speeds.contains(s))
            speeds << s;
    m_speedCombo->addItems(speeds);
    if (!Settings::instance().writeSpeed.isEmpty()) {
        const int i = m_speedCombo->findText(Settings::instance().writeSpeed);
        if (i >= 0)
            m_speedCombo->setCurrentIndex(i);
    }

    rebuildTree();
}

void DataProjectPage::setProject(DataProject *project)
{
    m_project = project;
    m_volumeEdit->setText(project->volumeLabel);
    rebuildTree();
}

void DataProjectPage::addFiles()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, QStringLiteral("选择要刻录的文件"), QDir::homePath());
    QVector<QUrl> urls;
    for (const QString &p : files)
        urls << QUrl::fromLocalFile(p);
    addUrls(urls);
}

void DataProjectPage::addFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择要刻录的文件夹"), QDir::homePath());
    if (!dir.isEmpty())
        addUrls({QUrl::fromLocalFile(dir)});
}

void DataProjectPage::removeSelected()
{
    const auto items = m_tree->selectedItems();
    for (QTreeWidgetItem *it : items)
        delete it;
    syncFromTree();
}

void DataProjectPage::clearProject()
{
    if (m_tree->topLevelItemCount() > 0) {
        const auto r = QMessageBox::question(
            this, QStringLiteral("清空项目"),
            QStringLiteral("确定要移除数据项目中的所有条目吗？"),
            QMessageBox::Yes | QMessageBox::Cancel);
        if (r != QMessageBox::Yes)
            return;
    }
    m_tree->clear();
    syncFromTree();
}

void DataProjectPage::onFilesDropped(const QVector<QUrl> &urls)
{
    addUrls(urls);
}

void DataProjectPage::addUrls(const QVector<QUrl> &urls)
{
    for (const QUrl &u : urls) {
        const QString p = u.toLocalFile();
        const QFileInfo fi(p);
        if (!fi.exists())
            continue;

        // 去重（按绝对路径）
        bool dup = false;
        for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
            if (m_tree->topLevelItem(i)->text(3) == fi.absoluteFilePath()) {
                dup = true;
                break;
            }
        }
        if (dup)
            continue;

        auto *item = new QTreeWidgetItem;
        item->setText(0, fi.fileName().isEmpty() ? p : fi.fileName());
        const qint64 sz = fi.isDir() ? dirSize(p) : fi.size();
        item->setText(1, formatBytes(sz));
        item->setText(2, fi.isDir() ? QStringLiteral("文件夹")
                                    : fi.suffix().toUpper() + QStringLiteral(" 文件"));
        item->setText(3, fi.absoluteFilePath());
        item->setToolTip(3, p);
        item->setIcon(0, style()->standardIcon(
                          fi.isDir() ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
        m_tree->addTopLevelItem(item);
    }
    syncFromTree();
}

void DataProjectPage::syncFromTree()
{
    if (!m_project)
        return;
    m_project->entries.clear();
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i)
        m_project->entries << QUrl::fromLocalFile(m_tree->topLevelItem(i)->text(3));
    m_project->volumeLabel = m_volumeEdit->text().trimmed();
    refreshCapacity();
}

void DataProjectPage::rebuildTree()
{
    m_tree->clear();
    if (!m_project)
        return;
    QVector<QUrl> urls;
    for (const QUrl &u : m_project->entries)
        urls << u;
    addUrls(urls);
    refreshCapacity();
}

void DataProjectPage::refreshCapacity()
{
    if (!m_project)
        return;
    const qint64 total = m_project->totalSize();
    m_sizeLabel->setText(QStringLiteral("项目大小：%1（%2 个条目）")
                             .arg(formatBytes(total))
                             .arg(m_project->entryCount()));

    const MediaStatus ms = m_backend->mediaStatus();
    const qint64 capBytes = ms.capacityMB * 1024 * 1024;
    if (!ms.present || capBytes <= 0) {
        m_capacityBar->setValue(0);
        m_capacityLabel->setText(QStringLiteral("容量：未知（未放入光盘）"));
        return;
    }
    const int pct = int(qreal(total) * 100.0 / capBytes);
    m_capacityBar->setValue(qBound(0, pct, 100));
    m_capacityLabel->setText(
        QStringLiteral("%1 / %2").arg(formatBytes(total), formatBytes(capBytes)));

    if (total > capBytes) {
        m_capacityBar->setStyleSheet(QStringLiteral(
            "QProgressBar{border:none;border-radius:6px;background:#ffe0e0;}"
            "QProgressBar::chunk{border-radius:6px;background:#e53935;}"));
        m_sizeLabel->setStyleSheet(QStringLiteral("color:#e53935;font-weight:bold;"));
    } else {
        m_capacityBar->setStyleSheet(QStringLiteral(
            "QProgressBar{border:none;border-radius:6px;background:#e3e7ee;}"
            "QProgressBar::chunk{border-radius:6px;background:#2979ff;}"));
        m_sizeLabel->setStyleSheet(QString());
    }
}

void DataProjectPage::onMediaChanged()
{
    refreshCapacity();
}

bool DataProjectPage::confirmOverwrite()
{
    const MediaStatus ms = m_backend->mediaStatus();
    if (!ms.present) {
        QMessageBox::warning(this, QStringLiteral("无法烧录"),
                             QStringLiteral("光驱中未检测到光盘，请先放入一张可写光盘。"));
        return false;
    }
    if (!ms.blank) {
        const auto r = QMessageBox::question(
            this, QStringLiteral("光盘不为空白"),
            QStringLiteral("光驱中的光盘不是空白盘。继续烧录将覆盖其内容，是否继续？"),
            QMessageBox::Yes | QMessageBox::Cancel);
        if (r != QMessageBox::Yes)
            return false;
    }
    return true;
}

void DataProjectPage::requestBurn()
{
    if (!m_project || m_project->empty()) {
        QMessageBox::information(this, QStringLiteral("开始烧录"),
                                 QStringLiteral("数据项目中还没有文件，请先添加文件或文件夹。"));
        return;
    }

    m_project->volumeLabel = m_volumeEdit->text().trimmed();
    if (m_project->volumeLabel.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("卷标"),
                             QStringLiteral("请填写光盘卷标（最多 16 个字符）。"));
        return;
    }

    if (!m_backend->demoMode && !confirmOverwrite())
        return;

    // 保存当前选择的速度
    Settings::instance().writeSpeed = m_speedCombo->currentText();
    Settings::save();

    const QString dev = Settings::instance().device;
    QStringList sources;
    for (const QUrl &u : m_project->entries)
        sources << u.toLocalFile();

    const QString title =
        QStringLiteral("正在刻录数据光盘：%1").arg(m_project->name);
    emit burnRequested(title);
    m_backend->burnData(dev, sources, m_project->volumeLabel,
                        m_speedCombo->currentText(), m_project->multiSession,
                        Settings::instance().ejectAfterBurn);
}

} // namespace Burn
