// Backend.h — 刻录后端：工具检测、介质检测、所有刻录操作编排
#pragma once

#include <QObject>
#include <QQueue>
#include <QHash>
#include <functional>

#include "Types.h"
#include "JobRunner.h"
#include "Projects.h"

namespace Burn {

// 后端把一次"操作"编排成一个或多个底层命令（如：复制 = 先读盘再写入）。
// 所有操作走同一作业队列，串行执行；实时通过信号上报日志与进度。
class Backend : public QObject
{
    Q_OBJECT
public:
    explicit Backend(QObject *parent = nullptr);

    // 重新检测工具与光驱
    void refresh();

    // 工具可用性
    bool haveWodim = false;
    bool haveGenisoimage = false;
    bool haveGrowisofs = false;
    bool haveCdrdao = false;
    bool haveDvdMediainfo = false;
    bool haveDd = false;
    bool haveFfmpeg = false;
    bool haveEject = false;
    bool demoMode = false;      // true = 缺少关键工具或光驱，走演示模式

    QStringList devices;        // 检测到的 /dev/sr*
    QString     device;         // 当前默认设备
    QString     driveVendor;
    QString     driveProduct;
    bool        haveDrive() const;

    MediaStatus mediaStatus() const { return m_media; }

    // 检测某设备上的介质信息（同步，耗时约 1~2 秒）
    MediaStatus detectMedia(const QString &dev);
    // 触发一次介质检测并广播 mediaChanged()
    void probeMedia();

    // 刻录速度列表（"自动" + 检测到的速度）
    QStringList burningSpeeds(const QString &dev);

    // ---- 操作（排队执行）----
    bool createIso(const QStringList &sources, const QString &outIso,
                   const QString &label, bool joliet, bool rockRidge);
    bool burnIso(const QString &dev, const QString &iso, const QString &speed, bool eject);
    bool burnData(const QString &dev, const QStringList &sources,
                  const QString &label, const QString &speed,
                  bool multiSession, bool eject);
    bool burnAudio(const QString &dev, const QStringList &wavFiles,
                   const QString &speed, bool eject);
    // 烧录音乐项目：自动将非 WAV 音轨用 ffmpeg 转换为 44.1kHz/16bit WAV 再刻录
    bool burnAudioProject(const QString &dev, const AudioProject &project,
                          const QString &speed, bool eject);
    bool copyDisc(const QString &srcDev, const QString &dstDev,
                  const QString &speed, bool eject, CopyMode mode);
    bool erase(const QString &dev, EraseMode mode);
    bool ejectDisc(const QString &dev);

    bool busy() const { return m_runner.running() || !m_queue.isEmpty(); }
    QString currentTitle() const { return m_runner.title(); }

public slots:
    void cancel();

signals:
    void logLine(const QString &line, bool isError);
    void progress(int percent);
    void detailChanged(const QString &text);
    void jobFinished(bool ok, int exitCode);
    void stateChanged(const QString &text);
    void mediaChanged();
    void backendChanged();

private:
    struct QueuedJob {
        JobKind kind;
        QString program;
        QStringList args;
        QString title;
        qint64 totalBytes = 0;
        std::function<void()> after;
    };

    void refreshTools();
    void refreshDevices();
    void queueJob(const QueuedJob &job);
    void runNext();
    void startDemoJob(const QueuedJob &job);

    QString findTempIso(const QString &hint) const;
    MediaStatus parseDvdMediaInfo(const QString &dev, const QString &out);
    MediaStatus parseWodimAtip(const QString &dev, const QString &out);
    MediaClass classifyMedia(const QString &mountString) const;
    qint64 defaultCapacityMB(MediaClass c) const;

    JobRunner m_runner;
    QQueue<QueuedJob> m_queue;
    MediaStatus m_media;
    bool m_finishing = false;
};

} // namespace Burn
