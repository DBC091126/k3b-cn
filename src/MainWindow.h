// MainWindow.h — 主窗口：菜单/工具栏/左侧导航/堆叠页面/状态栏
#pragma once

#include <QMainWindow>

class QListWidget;
class QStackedWidget;
class QLabel;

namespace Burn {

class Backend;
class DataProject;
class AudioProject;

class WelcomePage;
class DataProjectPage;
class AudioProjectPage;
class CopyDiscPage;
class IsoToolsPage;
class MediaInfoPage;
class ErasePage;
class BurnJobPage;
class BurnOptionsDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    Backend *backend() const { return m_backend; }
    DataProject *dataProject() const { return m_dataProject; }
    AudioProject *audioProject() const { return m_audioProject; }

    // 页面索引（与导航列表对应）
    enum PageIndex {
        PageWelcome = 0,
        PageData,
        PageAudio,
        PageCopy,
        PageIso,
        PageMedia,
        PageErase,
        PageBurn,
        PageCount
    };
    void gotoPage(int index);

public slots:
    void refreshStatusBar();

private slots:
    void onNewDataProject();
    void onNewAudioProject();
    void onOpenProject();
    void onSaveProject();
    void onSettings();
    void onAbout();
    void onRefreshMedia();
    void onBurnRequested(const QString &title);
    void onJobFinished(bool ok, int code);
    void onNavChanged(int row);

private:
    void setupMenu();
    void setupToolbar();
    void setupCentral();
    void setupStatusBar();
    void createActions();

    Backend *m_backend = nullptr;
    DataProject *m_dataProject = nullptr;
    AudioProject *m_audioProject = nullptr;

    QListWidget *m_nav = nullptr;
    QStackedWidget *m_stack = nullptr;

    WelcomePage *m_welcome = nullptr;
    DataProjectPage *m_dataPage = nullptr;
    AudioProjectPage *m_audioPage = nullptr;
    CopyDiscPage *m_copyPage = nullptr;
    IsoToolsPage *m_isoPage = nullptr;
    MediaInfoPage *m_mediaPage = nullptr;
    ErasePage *m_erasePage = nullptr;
    BurnJobPage *m_burnPage = nullptr;
    BurnOptionsDialog *m_optionsDialog = nullptr;

    QLabel *m_statusDevice = nullptr;
    QLabel *m_statusMedia = nullptr;
    QLabel *m_statusTools = nullptr;

    bool m_projectDirty = false;
    QString m_projectFilePath;
};

} // namespace Burn
