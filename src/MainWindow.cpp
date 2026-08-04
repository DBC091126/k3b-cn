// MainWindow.cpp — 主窗口实现
#include "MainWindow.h"

#include <QListWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QAction>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QSettings>
#include <QDir>
#include <QSignalBlocker>
#include <QStyle>

#include "Backend.h"
#include "Projects.h"
#include "Icons.h"
#include "Settings.h"
#include "pages/WelcomePage.h"
#include "pages/DataProjectPage.h"
#include "pages/AudioProjectPage.h"
#include "pages/CopyDiscPage.h"
#include "pages/IsoToolsPage.h"
#include "pages/MediaInfoPage.h"
#include "pages/ErasePage.h"
#include "pages/BurnJobPage.h"
#include "pages/BurnOptionsDialog.h"

namespace Burn {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_backend = new Backend(this);
    m_dataProject = new DataProject(this);
    m_audioProject = new AudioProject(this);

    setupCentral();
    setupMenu();
    setupToolbar();
    setupStatusBar();

    connect(m_backend, &Backend::mediaChanged, this, &MainWindow::refreshStatusBar);
    connect(m_backend, &Backend::backendChanged, this, &MainWindow::refreshStatusBar);
    connect(m_backend, &Backend::jobFinished, this, &MainWindow::onJobFinished);

    setWindowTitle(QStringLiteral("K3b 刻录软件（中文版）"));
    refreshStatusBar();
}

void MainWindow::setupCentral()
{
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    m_nav = new QListWidget(splitter);
    m_nav->setFixedWidth(188);
    m_nav->setIconSize(QSize(32, 32));
    m_nav->setSpacing(2);
    m_nav->addItem(new QListWidgetItem(Icons::dataDisc(), QStringLiteral("数据项目")));
    m_nav->addItem(new QListWidgetItem(Icons::musicDisc(), QStringLiteral("音乐 CD 项目")));
    m_nav->addItem(new QListWidgetItem(Icons::copyDisc(), QStringLiteral("复制光盘")));
    m_nav->addItem(new QListWidgetItem(Icons::isoImage(), QStringLiteral("ISO 镜像工具")));
    m_nav->addItem(new QListWidgetItem(Icons::mediaInfo(), QStringLiteral("介质信息")));
    m_nav->addItem(new QListWidgetItem(Icons::erase(), QStringLiteral("擦除光盘")));
    m_nav->addItem(new QListWidgetItem(Icons::burn(), QStringLiteral("刻录进度")));

    m_stack = new QStackedWidget(splitter);
    m_welcome    = new WelcomePage(m_backend, m_stack);
    m_dataPage   = new DataProjectPage(m_backend, m_dataProject, m_stack);
    m_audioPage  = new AudioProjectPage(m_backend, m_audioProject, m_stack);
    m_copyPage   = new CopyDiscPage(m_backend, m_stack);
    m_isoPage    = new IsoToolsPage(m_backend, m_stack);
    m_mediaPage  = new MediaInfoPage(m_backend, m_stack);
    m_erasePage  = new ErasePage(m_backend, m_stack);
    m_burnPage   = new BurnJobPage(m_backend, m_stack);

    m_stack->addWidget(m_welcome);    // 0
    m_stack->addWidget(m_dataPage);   // 1
    m_stack->addWidget(m_audioPage);  // 2
    m_stack->addWidget(m_copyPage);   // 3
    m_stack->addWidget(m_isoPage);    // 4
    m_stack->addWidget(m_mediaPage);  // 5
    m_stack->addWidget(m_erasePage);  // 6
    m_stack->addWidget(m_burnPage);   // 7

    splitter->addWidget(m_nav);
    splitter->addWidget(m_stack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setCollapsible(0, false);
    setCentralWidget(splitter);

    // 导航与页面切换
    connect(m_nav, &QListWidget::currentRowChanged,
            this, &MainWindow::onNavChanged);

    // 欢迎页入口
    connect(m_welcome, &WelcomePage::newDataProject,
            this, &MainWindow::onNewDataProject);
    connect(m_welcome, &WelcomePage::newAudioProject,
            this, &MainWindow::onNewAudioProject);
    connect(m_welcome, &WelcomePage::openCopy,
            this, [this]() { gotoPage(PageCopy); });
    connect(m_welcome, &WelcomePage::openIso,
            this, [this]() { gotoPage(PageIso); });
    connect(m_welcome, &WelcomePage::openMedia,
            this, [this]() { gotoPage(PageMedia); });
    connect(m_welcome, &WelcomePage::openErase,
            this, [this]() { gotoPage(PageErase); });

    // 各页发起刻录 → 跳到刻录进度页
    connect(m_dataPage, &DataProjectPage::burnRequested,
            this, &MainWindow::onBurnRequested);
    connect(m_audioPage, &AudioProjectPage::burnRequested,
            this, &MainWindow::onBurnRequested);
    connect(m_copyPage, &CopyDiscPage::burnRequested,
            this, &MainWindow::onBurnRequested);
    connect(m_isoPage, &IsoToolsPage::burnRequested,
            this, &MainWindow::onBurnRequested);
    connect(m_erasePage, &ErasePage::burnRequested,
            this, &MainWindow::onBurnRequested);

    // 进度页返回主界面
    connect(m_burnPage, &BurnJobPage::backRequested,
            this, [this]() { gotoPage(PageWelcome); });

    m_nav->setCurrentRow(0);
}

void MainWindow::setupMenu()
{
    auto *mFile = menuBar()->addMenu(QStringLiteral("文件(&F)"));
    auto *mEdit = menuBar()->addMenu(QStringLiteral("编辑(&E)"));
    auto *mTools = menuBar()->addMenu(QStringLiteral("工具(&T)"));
    auto *mSet = menuBar()->addMenu(QStringLiteral("设置(&S)"));
    auto *mHelp = menuBar()->addMenu(QStringLiteral("帮助(&H)"));

    mFile->addAction(QStringLiteral("新建数据光盘项目"), this,
                     &MainWindow::onNewDataProject);
    mFile->addAction(QStringLiteral("新建音乐 CD 项目"), this,
                     &MainWindow::onNewAudioProject);
    mFile->addSeparator();
    mFile->addAction(QStringLiteral("打开项目…"), this,
                     &MainWindow::onOpenProject);
    mFile->addAction(QStringLiteral("保存项目"), this,
                     &MainWindow::onSaveProject);
    mFile->addSeparator();
    mFile->addAction(QStringLiteral("退出(&Q)"), this, &QWidget::close);

    mEdit->addAction(QStringLiteral("添加文件…"), m_dataPage,
                     &DataProjectPage::addFiles);
    mEdit->addAction(QStringLiteral("添加文件夹…"), m_dataPage,
                     &DataProjectPage::addFolder);
    mEdit->addAction(QStringLiteral("移除选中项"), m_dataPage,
                     &DataProjectPage::removeSelected);
    mEdit->addSeparator();
    mEdit->addAction(QStringLiteral("清空数据项目"), m_dataPage,
                     &DataProjectPage::clearProject);

    mTools->addAction(QStringLiteral("复制光盘"), this,
                      [this]() { gotoPage(PageCopy); });
    mTools->addAction(QStringLiteral("ISO 镜像工具"), this,
                      [this]() { gotoPage(PageIso); });
    mTools->addAction(QStringLiteral("介质信息"), this,
                      [this]() { gotoPage(PageMedia); });
    mTools->addAction(QStringLiteral("擦除光盘"), this,
                      [this]() { gotoPage(PageErase); });

    mSet->addAction(QStringLiteral("刻录设置…"), this, &MainWindow::onSettings);
    mSet->addAction(QStringLiteral("刷新设备与介质"), this,
                    &MainWindow::onRefreshMedia);

    mHelp->addAction(QStringLiteral("关于 K3b 中文版"), this, &MainWindow::onAbout);
}

void MainWindow::setupToolbar()
{
    auto *tb = addToolBar(QStringLiteral("主工具栏"));
    tb->setMovable(false);
    tb->setIconSize(QSize(28, 28));

    auto *actData = new QAction(Icons::dataDisc(), QStringLiteral("新建数据项目"), this);
    auto *actAudio = new QAction(Icons::musicDisc(), QStringLiteral("新建音乐 CD"), this);
    auto *actOpen = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton),
                                QStringLiteral("打开项目"), this);
    auto *actSave = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton),
                                QStringLiteral("保存项目"), this);
    auto *actBurn = new QAction(Icons::burn(), QStringLiteral("开始刻录"), this);
    auto *actEject = new QAction(Icons::eject(), QStringLiteral("弹出光盘"), this);

    connect(actData, &QAction::triggered, this, &MainWindow::onNewDataProject);
    connect(actAudio, &QAction::triggered, this, &MainWindow::onNewAudioProject);
    connect(actOpen, &QAction::triggered, this, &MainWindow::onOpenProject);
    connect(actSave, &QAction::triggered, this, &MainWindow::onSaveProject);
    connect(actEject, &QAction::triggered, this, [this]() {
        m_backend->ejectDisc(Settings::instance().device);
    });

    tb->addAction(actData);
    tb->addAction(actAudio);
    tb->addSeparator();
    tb->addAction(actOpen);
    tb->addAction(actSave);
    tb->addSeparator();
    tb->addAction(actBurn);
    tb->addSeparator();
    tb->addAction(actEject);
}

void MainWindow::setupStatusBar()
{
    m_statusDevice = new QLabel(this);
    m_statusMedia = new QLabel(this);
    m_statusTools = new QLabel(this);
    statusBar()->addWidget(m_statusDevice);
    statusBar()->addWidget(m_statusMedia, 1);
    statusBar()->addPermanentWidget(m_statusTools);
    refreshStatusBar();
}

void MainWindow::gotoPage(int index)
{
    if (index < 0 || index >= PageCount)
        return;
    if (m_nav->currentRow() != index) {
        QSignalBlocker blocker(m_nav);
        m_nav->setCurrentRow(index);
    }
    m_stack->setCurrentIndex(index);
}

void MainWindow::onNavChanged(int row)
{
    if (row >= 0 && row < PageCount)
        m_stack->setCurrentIndex(row);
}

void MainWindow::refreshStatusBar()
{
    if (!m_statusDevice)
        return;

    const bool hasDrive = !m_backend->devices.isEmpty();
    const QString dev = m_backend->device.isEmpty()
                            ? QStringLiteral("（无光驱）")
                            : m_backend->device;
    m_statusDevice->setText(
        QStringLiteral("设备：%1  %2 %3")
            .arg(dev)
            .arg(m_backend->driveVendor)
            .arg(m_backend->driveProduct));

    const MediaStatus ms = m_backend->mediaStatus();
    if (!hasDrive) {
        m_statusMedia->setText(QStringLiteral("未检测到光盘驱动器"));
    } else if (!ms.present) {
        m_statusMedia->setText(QStringLiteral("光驱中无光盘"));
    } else if (ms.blank) {
        m_statusMedia->setText(QStringLiteral("介质：空白 %1（%2 MB）")
                                   .arg(mediaClassToText(ms.mediaClass))
                                   .arg(ms.capacityMB));
    } else if (ms.writable) {
        m_statusMedia->setText(QStringLiteral("介质：可写 %1").arg(ms.mediaType));
    } else {
        m_statusMedia->setText(QStringLiteral("介质：%1（只读）").arg(ms.mediaType));
    }

    if (m_backend->demoMode) {
        m_statusTools->setText(QStringLiteral("● 演示模式（未检测到刻录工具/光驱）"));
        m_statusTools->setStyleSheet(QStringLiteral("color:#d9822b;"));
    } else {
        m_statusTools->setText(QStringLiteral("刻录工具就绪"));
        m_statusTools->setStyleSheet(QString());
    }
}

void MainWindow::onNewDataProject()
{
    if (m_dataProject->entryCount() > 0) {
        const auto r = QMessageBox::question(
            this, QStringLiteral("新建数据项目"),
            QStringLiteral("当前数据项目尚未刻录，确定要新建一个吗？"),
            QMessageBox::Yes | QMessageBox::Cancel);
        if (r != QMessageBox::Yes)
            return;
    }
    delete m_dataProject;
    m_dataProject = new DataProject(this);
    m_dataPage->setProject(m_dataProject);
    m_projectDirty = false;
    m_projectFilePath.clear();
    gotoPage(PageData);
}

void MainWindow::onNewAudioProject()
{
    if (m_audioProject->trackCount() > 0) {
        const auto r = QMessageBox::question(
            this, QStringLiteral("新建音乐 CD"),
            QStringLiteral("当前音乐 CD 项目尚未刻录，确定要新建一个吗？"),
            QMessageBox::Yes | QMessageBox::Cancel);
        if (r != QMessageBox::Yes)
            return;
    }
    delete m_audioProject;
    m_audioProject = new AudioProject(this);
    m_audioPage->setProject(m_audioProject);
    m_projectDirty = false;
    m_projectFilePath.clear();
    gotoPage(PageAudio);
}

void MainWindow::onOpenProject()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开项目"), QDir::homePath(),
        QStringLiteral("刻录项目 (*.k3bcn);;所有文件 (*)"));
    if (path.isEmpty())
        return;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("打开项目"),
                             QStringLiteral("无法打开文件：%1").arg(path));
        return;
    }
    QTextStream in(&f);
    QString type;
    const QStringList lines = in.readAll().split(QLatin1Char('\n'));
    f.close();

    QStringList files;
    QString name, label;
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            type = line.mid(1, line.size() - 2).toLower();
        } else if (line.startsWith(QStringLiteral("name="))) {
            name = line.mid(5);
        } else if (line.startsWith(QStringLiteral("volumeLabel="))) {
            label = line.mid(12);
        } else if (line.startsWith(QStringLiteral("file="))) {
            files << line.mid(5);
        }
    }

    if (type == QStringLiteral("audio")) {
        delete m_audioProject;
        m_audioProject = new AudioProject(this);
        if (!name.isEmpty())
            m_audioProject->name = name;
        for (const QString &p : files) {
            AudioTrack t;
            t.url = QUrl::fromLocalFile(p);
            probeAudioFile(t);
            m_audioProject->tracks << t;
        }
        m_audioPage->setProject(m_audioProject);
        gotoPage(PageAudio);
    } else {
        delete m_dataProject;
        m_dataProject = new DataProject(this);
        if (!name.isEmpty())
            m_dataProject->name = name;
        if (!label.isEmpty())
            m_dataProject->volumeLabel = label;
        for (const QString &p : files)
            m_dataProject->entries << QUrl::fromLocalFile(p);
        m_dataPage->setProject(m_dataProject);
        gotoPage(PageData);
    }
    m_projectDirty = false;
    m_projectFilePath = path;
}

void MainWindow::onSaveProject()
{
    const bool audio = (m_stack->currentIndex() == PageAudio);
    QString path = m_projectFilePath;
    if (path.isEmpty()) {
        path = QFileDialog::getSaveFileName(
            this, QStringLiteral("保存项目"),
            QDir::homePath() + QStringLiteral("/")
                + (audio ? m_audioProject->name : m_dataProject->name)
                + QStringLiteral(".k3bcn"),
            QStringLiteral("刻录项目 (*.k3bcn)"));
        if (path.isEmpty())
            return;
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("保存项目"),
                             QStringLiteral("无法写入文件：%1").arg(path));
        return;
    }
    QTextStream out(&f);
    if (audio) {
        out << QStringLiteral("[audio]\n");
        out << QStringLiteral("name=") << m_audioProject->name << QLatin1Char('\n');
        for (const AudioTrack &t : m_audioProject->tracks)
            out << QStringLiteral("file=") << t.url.toLocalFile() << QLatin1Char('\n');
    } else {
        out << QStringLiteral("[data]\n");
        out << QStringLiteral("name=") << m_dataProject->name << QLatin1Char('\n');
        out << QStringLiteral("volumeLabel=") << m_dataProject->volumeLabel
            << QLatin1Char('\n');
        for (const QUrl &u : m_dataProject->entries)
            out << QStringLiteral("file=") << u.toLocalFile() << QLatin1Char('\n');
    }
    f.close();
    m_projectFilePath = path;
    m_projectDirty = false;
    statusBar()->showMessage(QStringLiteral("项目已保存到 %1").arg(path), 4000);
}

void MainWindow::onSettings()
{
    if (!m_optionsDialog)
        m_optionsDialog = new BurnOptionsDialog(m_backend, this);
    m_optionsDialog->exec();
    m_backend->refresh();
}

void MainWindow::onAbout()
{
    QMessageBox::about(
        this, QStringLiteral("关于 K3b 刻录软件（中文版）"),
        QStringLiteral(
            "<b>K3b 刻录软件（中文版）</b> v1.0.0<br><br>"
            "一个界面与 K3b 类似的 CD/DVD/BD 刻录工具，完全中文化。<br><br>"
            "功能：数据光盘、音乐 CD、光盘复制、ISO 镜像、介质信息、擦除。<br>"
            "底层调用系统刻录工具：<br>"
            "&nbsp;&nbsp;• wodim / genisoimage / mkisofs（CD）<br>"
            "&nbsp;&nbsp;• growisofs（DVD/BD）<br>"
            "&nbsp;&nbsp;• cdrdao / dd / dvd+rw-format（复制与擦除）<br>"
            "&nbsp;&nbsp;• ffmpeg（音频转码）<br><br>"
            "该软件为演示与学习用途的中文实现，与 KDE 的 K3b 官方项目无关。"));
}

void MainWindow::onRefreshMedia()
{
    m_backend->refresh();
    refreshStatusBar();
    statusBar()->showMessage(QStringLiteral("已刷新设备与介质信息"), 3000);
}

void MainWindow::onBurnRequested(const QString &title)
{
    m_burnPage->showJob(title);
    gotoPage(PageBurn);
}

void MainWindow::onJobFinished(bool ok, int code)
{
    Q_UNUSED(code);
    refreshStatusBar();
    if (isVisible())
        statusBar()->showMessage(ok ? QStringLiteral("作业完成 ✔")
                                    : QStringLiteral("作业失败，请查看日志"),
                                 6000);
}

} // namespace Burn
