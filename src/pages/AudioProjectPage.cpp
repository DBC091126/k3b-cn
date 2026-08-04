// AudioProjectPage.cpp — 音乐 CD 项目页实现
#include "AudioProjectPage.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

#include "../Backend.h"
#include "../Projects.h"
#include "../Settings.h"
#include "../Icons.h"

namespace Burn {

// 标准 CD 容量：74 分钟（这里放宽到 80 分钟）
static constexpr qint64 kAudioCdLimitSec = 80 * 60;

namespace {

QString fmtDuration(qint64 sec)
{
    return QString::asprintf("%02lld:%02lld", sec / 60, sec % 60);
}

} // namespace

// 支持拖拽的树
class AudioTree : public QTreeWidget
{
    Q_OBJECT
public:
    explicit AudioTree(QWidget *parent = nullptr)
        : QTreeWidget(parent)
    {
        setAcceptDrops(true);
        setDragDropMode(QAbstractItemView::DropOnly);
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        setRootIsDecorated(false);
        setUniformRowHeights(true);
    }

signals:
    void filesDropped(const QVector<QUrl> &urls);

protected:
    void dragEnterEvent(QDragEnterEvent *e) override
    {
        if (e->mimeData() && e->mimeData()->hasUrls())
            e->acceptProposedAction();
    }
    void dragMoveEvent(QDragMoveEvent *e) override
    {
        if (e->mimeData() && e->mimeData()->hasUrls())
            e->acceptProposedAction();
    }
    void dropEvent(QDropEvent *e) override
    {
        if (!e->mimeData() || !e->mimeData()->hasUrls())
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
};

AudioProjectPage::AudioProjectPage(Backend *backend, AudioProject *project,
                                   QWidget *parent)
    : QWidget(parent), m_backend(backend), m_project(project)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(8);

    auto *toolRow = new QHBoxLayout;
    auto *btnAdd = new QPushButton(QStringLiteral("添加音轨…"), this);
    auto *btnRemove = new QPushButton(QStringLiteral("移除选中"), this);
    auto *btnClear = new QPushButton(QStringLiteral("清空"), this);
    toolRow->addWidget(btnAdd);
    toolRow->addWidget(btnRemove);
    toolRow->addWidget(btnClear);

    toolRow->addSpacing(24);
    toolRow->addWidget(new QLabel(QStringLiteral("写入速度："), this));
    m_speedCombo = new QComboBox(this);
    m_speedCombo->addItem(QStringLiteral("自动"));
    m_speedCombo->setMaximumWidth(110);
    toolRow->addWidget(m_speedCombo);

    toolRow->addStretch();
    auto *btnBurn = new QPushButton(Icons::play(QColor(255, 255, 255)),
                                    QStringLiteral("刻录音乐 CD"), this);
    btnBurn->setStyleSheet(QStringLiteral(
        "QPushButton{background:#e84393;color:white;border:none;"
        "padding:8px 20px;border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#d81b60;}"));
    toolRow->addWidget(btnBurn);
    layout->addLayout(toolRow);

    connect(btnAdd, &QPushButton::clicked, this, &AudioProjectPage::addTracks);
    connect(btnRemove, &QPushButton::clicked, this, &AudioProjectPage::removeSelected);
    connect(btnClear, &QPushButton::clicked, this, &AudioProjectPage::clearProject);
    connect(btnBurn, &QPushButton::clicked, this, &AudioProjectPage::requestBurn);

    m_tree = new AudioTree(this);
    m_tree->setColumnCount(5);
    m_tree->setHeaderLabels({QStringLiteral("轨道"), QStringLiteral("标题"),
                             QStringLiteral("时长"), QStringLiteral("格式"),
                             QStringLiteral("大小")});
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    layout->addWidget(m_tree, 1);

    // 底部信息
    auto *capRow = new QHBoxLayout;
    m_durationLabel = new QLabel(this);
    m_capacityLabel = new QLabel(this);
    capRow->addWidget(m_durationLabel);
    capRow->addStretch();
    capRow->addWidget(m_capacityLabel);
    layout->addLayout(capRow);

    connect(static_cast<AudioTree *>(m_tree), &AudioTree::filesDropped,
            this, &AudioProjectPage::onFilesDropped);
    connect(m_backend, &Backend::mediaChanged,
            this, &AudioProjectPage::onMediaChanged);
    connect(m_backend, &Backend::backendChanged,
            this, &AudioProjectPage::onMediaChanged);

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

void AudioProjectPage::setProject(AudioProject *project)
{
    m_project = project;
    rebuildTree();
}

void AudioProjectPage::addTracks()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, QStringLiteral("选择音乐文件"), QDir::homePath(),
        QStringLiteral("音频文件 (*.wav *.mp3 *.ogg *.flac *.opus *.m4a *.aac);;"
                       "所有文件 (*)"));
    QVector<QUrl> urls;
    for (const QString &p : files)
        urls << QUrl::fromLocalFile(p);
    addUrls(urls);
}

void AudioProjectPage::addUrls(const QVector<QUrl> &urls)
{
    for (const QUrl &u : urls) {
        const QString p = u.toLocalFile();
        const QFileInfo fi(p);
        if (!fi.exists())
            continue;

        bool dup = false;
        for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
            if (m_tree->topLevelItem(i)->data(0, Qt::UserRole).toString() ==
                fi.absoluteFilePath()) {
                dup = true;
                break;
            }
        }
        if (dup)
            continue;

        AudioTrack t;
        t.url = u;
        probeAudioFile(t);

        auto *item = new QTreeWidgetItem;
        item->setData(0, Qt::UserRole, fi.absoluteFilePath());
        item->setText(0, QStringLiteral("轨道 %1").arg(m_tree->topLevelItemCount() + 1, 2, 10, QLatin1Char('0')));
        item->setText(1, t.title);
        item->setText(2, fmtDuration(t.durationSec));
        item->setText(3, fi.suffix().toUpper());
        item->setText(4, formatBytes(t.size));
        item->setIcon(0, Icons::musicDisc());
        m_tree->addTopLevelItem(item);
    }
    syncFromTree();
}

void AudioProjectPage::onFilesDropped(const QVector<QUrl> &urls)
{
    addUrls(urls);
}

void AudioProjectPage::removeSelected()
{
    const auto items = m_tree->selectedItems();
    for (QTreeWidgetItem *it : items)
        delete it;
    syncFromTree();
}

void AudioProjectPage::clearProject()
{
    if (m_tree->topLevelItemCount() > 0) {
        const auto r = QMessageBox::question(
            this, QStringLiteral("清空项目"),
            QStringLiteral("确定要清空音乐 CD 项目吗？"),
            QMessageBox::Yes | QMessageBox::Cancel);
        if (r != QMessageBox::Yes)
            return;
    }
    m_tree->clear();
    syncFromTree();
}

void AudioProjectPage::syncFromTree()
{
    if (!m_project)
        return;
    m_project->tracks.clear();
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        const QTreeWidgetItem *it = m_tree->topLevelItem(i);
        AudioTrack t;
        t.url = QUrl::fromLocalFile(it->data(0, Qt::UserRole).toString());
        t.title = it->text(1);
        probeAudioFile(t);
        m_project->tracks << t;
    }
    refreshSummary();
}

void AudioProjectPage::rebuildTree()
{
    m_tree->clear();
    if (!m_project)
        return;
    for (const AudioTrack &t : m_project->tracks) {
        auto *item = new QTreeWidgetItem;
        item->setData(0, Qt::UserRole, t.url.toLocalFile());
        item->setText(0, QStringLiteral("轨道 %1").arg(m_tree->topLevelItemCount() + 1, 2, 10, QLatin1Char('0')));
        item->setText(1, t.title);
        item->setText(2, fmtDuration(t.durationSec));
        item->setText(3, QFileInfo(t.url.toLocalFile()).suffix().toUpper());
        item->setText(4, formatBytes(t.size));
        item->setIcon(0, Icons::musicDisc());
        m_tree->addTopLevelItem(item);
    }
    refreshSummary();
}

void AudioProjectPage::refreshSummary()
{
    if (!m_project)
        return;
    const qint64 total = m_project->totalDurationSec();
    m_durationLabel->setText(
        QStringLiteral("总时长：%1（%2 个音轨）").arg(fmtDuration(total))
            .arg(m_project->trackCount()));

    // 音频容量：优先用介质真实扇区数（75 扇区/秒，支持 8cm 迷你盘），否则 80 分钟
    qint64 limitSec = kAudioCdLimitSec;
    const MediaStatus ms = m_backend->mediaStatus();
    if (ms.sectors > 0)
        limitSec = ms.sectors / 75;

    const qint64 remain = limitSec - total;
    if (remain < 0) {
        m_capacityLabel->setText(
            QStringLiteral("超出 CD 容量 %1").arg(fmtDuration(-remain)));
        m_capacityLabel->setStyleSheet(QStringLiteral("color:#e53935;font-weight:bold;"));
    } else {
        m_capacityLabel->setText(
            QStringLiteral("CD 剩余容量：%1 / %2").arg(fmtDuration(remain),
                                                       fmtDuration(limitSec)));
        m_capacityLabel->setStyleSheet(QString());
    }
}

void AudioProjectPage::onMediaChanged()
{
    refreshSummary();
}

void AudioProjectPage::requestBurn()
{
    if (!m_project || m_project->empty()) {
        QMessageBox::information(this, QStringLiteral("刻录音乐 CD"),
                                 QStringLiteral("音乐 CD 项目中还没有音轨，请先添加音乐文件。"));
        return;
    }
    qint64 limitSec = kAudioCdLimitSec;
    const MediaStatus ms = m_backend->mediaStatus();
    if (ms.sectors > 0)
        limitSec = ms.sectors / 75;
    if (m_project->totalDurationSec() > limitSec) {
        const auto r = QMessageBox::question(
            this, QStringLiteral("超出 CD 容量"),
            QStringLiteral("音轨总时长超过光盘容量（%1），部分音轨将被截断或刻录失败。仍然继续吗？")
                .arg(fmtDuration(limitSec)),
            QMessageBox::Yes | QMessageBox::Cancel);
        if (r != QMessageBox::Yes)
            return;
    }

    Settings::instance().writeSpeed = m_speedCombo->currentText();
    Settings::save();

    const QString dev = Settings::instance().device;
    const QString title =
        QStringLiteral("正在刻录音乐 CD：%1").arg(m_project->name);
    emit burnRequested(title);
    m_backend->burnAudioProject(dev, *m_project, m_speedCombo->currentText(),
                                Settings::instance().ejectAfterBurn);
}

} // namespace Burn

#include "AudioProjectPage.moc"
