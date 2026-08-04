// Backend.cpp — 刻录后端实现
#include "Backend.h"
#include "Settings.h"

#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDateTime>
#include <QStandardPaths>
#include <QDebug>
#include <memory>

namespace Burn {

Backend::Backend(QObject *parent)
    : QObject(parent)
{
    connect(&m_runner, &JobRunner::logLine, this, &Backend::logLine);
    connect(&m_runner, &JobRunner::progress, this, &Backend::progress);
    connect(&m_runner, &JobRunner::detailChanged, this, &Backend::detailChanged);
    connect(&m_runner, &JobRunner::stateChanged, this, &Backend::stateChanged);
    connect(&m_runner, &JobRunner::finished, this, [this](bool ok, int code) {
        Q_UNUSED(code);
        if (!m_queue.isEmpty()) {
            auto after = m_queue.head().after;
            if (after)
                after();
            m_queue.dequeue();
        }
        if (m_queue.isEmpty()) {
            m_finishing = false;
        } else {
            runNext();
        }
    });

    refresh();
}

// ---------- 刷新：工具 / 设备 / 介质 ----------
void Backend::refresh()
{
    refreshTools();
    refreshDevices();
    // 没有光驱，或缺少核心刻录工具（wodim/growisofs）时进入演示模式
    demoMode = !haveDrive() || !(haveGrowisofs || haveWodim);
    if (devices.isEmpty()) {
        // 无光驱时，只要演示模式能跑即可；设备留空
    } else if (device.isEmpty() || !devices.contains(device)) {
        device = devices.first();
    }
    probeMedia();
    emit backendChanged();
}

bool Backend::haveDrive() const
{
    return !devices.isEmpty();
}

void Backend::refreshTools()
{
    // 用 PATH 搜索判断工具是否可用，避免执行 --version（dd/eject 等不支持该参数）
    const auto have = [](const QString &name) {
        return !QStandardPaths::findExecutable(name).isEmpty();
    };
    haveWodim       = have(QStringLiteral("wodim"));
    haveGenisoimage = have(QStringLiteral("genisoimage"));
    haveGrowisofs   = have(QStringLiteral("growisofs"));
    haveCdrdao      = have(QStringLiteral("cdrdao"));
    haveDvdMediainfo= have(QStringLiteral("dvd+rw-mediainfo"));
    haveDd          = have(QStringLiteral("dd"));
    haveEject       = have(QStringLiteral("eject"));
    haveFfmpeg      = have(QStringLiteral("ffmpeg"));
}

void Backend::refreshDevices()
{
    devices.clear();
    QDir dir(QStringLiteral("/dev"));
    const QFileInfoList list =
        dir.entryInfoList({QStringLiteral("sr*")}, QDir::System, QDir::Name);
    for (const QFileInfo &fi : list)
        devices << fi.absoluteFilePath();
}

// ---------- 介质检测 ----------
MediaStatus Backend::detectMedia(const QString &dev)
{
    MediaStatus st;
    st.device = dev;

    // 1) dvd+rw-mediainfo（对 DVD / BD / 空白盘最可靠）
    if (haveDvdMediainfo) {
        QProcess p;
        p.setProgram(QStringLiteral("dvd+rw-mediainfo"));
        p.setArguments({dev});
        p.start();
        if (p.waitForFinished(4000)) {
            const QString out = QString::fromUtf8(p.readAllStandardOutput());
            if (!out.contains(QStringLiteral("no media"), Qt::CaseInsensitive))
                st = parseDvdMediaInfo(dev, out);
        }
    }

    // 2) wodim -atip（CD 介质）
    if (haveWodim) {
        QProcess p;
        p.setProgram(QStringLiteral("wodim"));
        p.setArguments({QStringLiteral("dev=") + dev, QStringLiteral("-atip")});
        p.start();
        if (p.waitForFinished(4000)) {
            const QString out = QString::fromUtf8(p.readAllStandardOutput())
                                  + QStringLiteral("\n")
                                  + QString::fromUtf8(p.readAllStandardError());
            const MediaStatus cd = parseWodimAtip(dev, out);
            if (cd.present)
                st = cd;
        }
    }

    // 3) CD 空白判定：
    //    -msinfo 成功返回会话号 = 有 ISO 会话（数据盘）
    //    -toc 能列出音轨 = 有轨道（音频盘）
    //    两者都失败 = 无任何数据 → 空白盘
    if (st.present && (st.mediaClass == MediaClass::CdR ||
                       st.mediaClass == MediaClass::CdRw) && haveWodim) {
        bool hasSession = false;
        bool hasTrack = false;

        QProcess q;
        q.setProgram(QStringLiteral("wodim"));
        q.setArguments({QStringLiteral("dev=") + dev, QStringLiteral("-msinfo")});
        q.start();
        if (q.waitForFinished(4000)) {
            const QString out = QString::fromUtf8(q.readAllStandardOutput())
                                  + QStringLiteral("\n")
                                  + QString::fromUtf8(q.readAllStandardError());
            if (!(out.contains(QStringLiteral("Input/output error")) ||
                  out.contains(QStringLiteral("No valid"), Qt::CaseInsensitive) ||
                  out.contains(QStringLiteral("No session"), Qt::CaseInsensitive)))
                hasSession = true;    // 返回了会话号 → 有数据会话
        }

        QProcess t;
        t.setProgram(QStringLiteral("wodim"));
        t.setArguments({QStringLiteral("dev=") + dev, QStringLiteral("-toc")});
        t.start();
        if (t.waitForFinished(4000)) {
            const QString out = QString::fromUtf8(t.readAllStandardOutput())
                                  + QStringLiteral("\n")
                                  + QString::fromUtf8(t.readAllStandardError());
            // "track:" 行存在说明盘上有音轨（音频盘/数据盘）
            hasTrack = out.contains(QRegularExpression(
                QStringLiteral("^track:\\s*\\d+"), QRegularExpression::MultilineOption));
        }

        if (hasSession || hasTrack) {
            st.blank = false;                 // 盘上有数据，不是空白盘
            st.writable = st.appendable || st.erasable;
        } else {
            st.blank = true;                  // 无会话也无轨道 → 空白
            st.appendable = false;
            st.writable = true;
        }
    }

    m_media = st;
    return st;
}

void Backend::probeMedia()
{
    MediaStatus st;
    if (!devices.isEmpty())
        st = detectMedia(device);
    m_media = st;
    emit mediaChanged();
}

MediaStatus Backend::parseDvdMediaInfo(const QString &dev, const QString &out)
{
    MediaStatus st;
    st.valid = true;
    st.device = dev;

    QHash<QString, QString> kv;
    for (const QString &rawLine : out.split(QLatin1Char('\n'))) {
        const QString line = rawLine.trimmed();
        const int idx = line.indexOf(QLatin1Char(':'));
        if (idx <= 0)
            continue;
        kv[line.left(idx).trimmed()] = line.mid(idx + 1).trimmed();
    }

    // 是否有介质
    if (kv.contains(QStringLiteral("Drive status"))) {
        const QString ds = kv.value(QStringLiteral("Drive status"));
        if (ds.contains(QStringLiteral("no media"), Qt::CaseInsensitive)) {
            st.valid = true;
            st.present = false;
            return st;
        }
    }

    const QString mounted = kv.value(QStringLiteral("Mounted Media"));
    if (mounted.isEmpty()) {
        // 可能完全没有输出介质信息
        st.present = false;
        return st;
    }

    st.present = true;
    st.mediaType = mounted;
    st.mediaClass = classifyMedia(mounted);

    const QString discStatus = kv.value(QStringLiteral("Disc Status")).toLower();
    st.blank = discStatus.contains(QStringLiteral("blank"));
    st.appendable = discStatus.contains(QStringLiteral("appendable"));
    st.erasable = kv.value(QStringLiteral("Erasable")).contains(QStringLiteral("true"), Qt::CaseInsensitive);
    st.writable = st.blank || st.appendable ||
                  st.mediaClass == MediaClass::CdRw ||
                  st.mediaClass == MediaClass::DvdRw ||
                  st.mediaClass == MediaClass::DvdRam ||
                  st.mediaClass == MediaClass::BdRe;

    // 容量："11810600 * 2048 = 24188108800 bytes"
    if (kv.contains(QStringLiteral("Disc Capacity"))) {
        const QString cap = kv.value(QStringLiteral("Disc Capacity"));
        static const QRegularExpression re(QStringLiteral("=\\s*([0-9]+)\\s+bytes"));
        const QRegularExpressionMatch m = re.match(cap);
        if (m.hasMatch())
            st.capacityMB = m.captured(1).toLongLong() / (1024 * 1024);
    }
    if (st.capacityMB <= 0)
        st.capacityMB = defaultCapacityMB(st.mediaClass);

    // 厂商 / 型号
    st.vendor = kv.value(QStringLiteral("Vendor ID"));
    st.product = kv.value(QStringLiteral("Product ID"));
    if (st.vendor.isEmpty() && kv.contains(QStringLiteral("INQUIRY"))) {
        const QStringList parts =
            kv.value(QStringLiteral("INQUIRY"))
                .split(QLatin1Char('['));
        if (parts.size() >= 2) {
            st.vendor = parts[1].section(QLatin1Char(']'), 0, 0).trimmed();
            st.product = parts.size() >= 3
                             ? parts[2].section(QLatin1Char(']'), 0, 0).trimmed()
                             : QString();
        }
    }
    if (st.vendor.isEmpty())
        st.vendor = driveVendor;
    if (st.product.isEmpty())
        st.product = driveProduct;

    return st;
}

MediaStatus Backend::parseWodimAtip(const QString &dev, const QString &out)
{
    MediaStatus st;
    st.valid = true;
    st.device = dev;

    // 厂商/型号
    static const QRegularExpression reVendor(QStringLiteral("Vendor_info\\s*:\\s*'([^']*)'"));
    static const QRegularExpression reProd(QStringLiteral("Identification\\s*:\\s*'([^']*)'"));
    const QRegularExpressionMatch mv = reVendor.match(out);
    if (mv.hasMatch())
        st.vendor = mv.captured(1).trimmed();
    const QRegularExpressionMatch mp = reProd.match(out);
    if (mp.hasMatch())
        st.product = mp.captured(1).trimmed();

    // 无介质：errno 5 / "No media" / "Cannot load media" / "Not Ready"
    if (out.contains(QStringLiteral("No media"), Qt::CaseInsensitive) ||
        out.contains(QStringLiteral("No medium"), Qt::CaseInsensitive) ||
        out.contains(QStringLiteral("Cannot load media"), Qt::CaseInsensitive) ||
        (out.contains(QStringLiteral("Input/output error")) &&
         !out.contains(QStringLiteral("Disk type")))) {
        st.valid = true;
        st.present = false;
        return st;
    }

    // 介质类型：按是否可擦除区分 CD-R / CD-RW
    static const QRegularExpression reErasable(QStringLiteral("Is not erasable"));
    st.erasable = !reErasable.match(out).hasMatch() &&
                  out.contains(QStringLiteral("Is erasable"), Qt::CaseInsensitive);
    st.mediaClass = st.erasable ? MediaClass::CdRw : MediaClass::CdR;
    st.mediaType = st.erasable ? QStringLiteral("CD-RW") : QStringLiteral("CD-R");
    st.present = true;

    // 状态码（部分光驱/固件会输出）：0 空盘 / 1 可追加 / 2 完成 / 3 最终化
    static const QRegularExpression reStatus(QStringLiteral("(?:Disk|Disc)\\s*status\\s*:\\s*(\\d)"));
    const QRegularExpressionMatch ms = reStatus.match(out);
    if (ms.hasMatch()) {
        st.discStatus = ms.captured(1).toInt();
        st.blank = (st.discStatus == 0);
        st.appendable = (st.discStatus == 1);
    }
    st.writable = st.blank || st.appendable || st.erasable;

    // 容量：从 ATIP lead-out 扇区数精确计算（支持 8cm 迷你盘等非标容量）
    // 例："ATIP start of lead out: 112500 (25:02/00)" → 112500 扇区
    static const QRegularExpression reLeadOut(
        QStringLiteral("ATIP start of lead\\s*out:\\s*(\\d+)"));
    const QRegularExpressionMatch mlo = reLeadOut.match(out);
    if (mlo.hasMatch()) {
        st.sectors = mlo.captured(1).toLongLong();
        st.capacityMB = st.sectors * 2048 / (1024 * 1024);
    } else {
        // 兜底：按介质类型默认容量
        st.capacityMB = defaultCapacityMB(st.mediaClass);
    }
    return st;
}

MediaClass Backend::classifyMedia(const QString &mount) const
{
    const QString m = mount.toUpper();
    if (m.contains(QStringLiteral("BD-RE")))    return MediaClass::BdRe;
    if (m.contains(QStringLiteral("BD-R")))     return MediaClass::BdR;
    if (m.contains(QStringLiteral("DVD-RAM")))  return MediaClass::DvdRam;
    if (m.contains(QStringLiteral("DVD-RW")) ||
        m.contains(QStringLiteral("DVD+RW")))   return MediaClass::DvdRw;
    if (m.contains(QStringLiteral("DVD+R")) ||
        m.contains(QStringLiteral("DVD-R")))    return MediaClass::DvdR;
    if (m.contains(QStringLiteral("DVD-ROM")))  return MediaClass::DvdRom;
    if (m.contains(QStringLiteral("CD-RW")))    return MediaClass::CdRw;
    if (m.contains(QStringLiteral("CD-R")))     return MediaClass::CdR;
    if (m.contains(QStringLiteral("CD-ROM")))   return MediaClass::CdRom;
    return MediaClass::Unknown;
}

qint64 Backend::defaultCapacityMB(MediaClass c) const
{
    switch (c) {
    case MediaClass::CdR:
    case MediaClass::CdRw:    return 700;
    case MediaClass::DvdR:
    case MediaClass::DvdRw:   return 4483;
    case MediaClass::DvdRam:  return 4483;
    case MediaClass::BdR:
    case MediaClass::BdRe:    return 23866;
    default:                  return 0;
    }
}

QStringList Backend::burningSpeeds(const QString &dev)
{
    QStringList speeds;
    speeds << QStringLiteral("自动");

    if (haveDvdMediainfo) {
        QProcess p;
        p.setProgram(QStringLiteral("dvd+rw-mediainfo"));
        p.setArguments({dev});
        p.start();
        if (p.waitForFinished(4000)) {
            const QString out = QString::fromUtf8(p.readAllStandardOutput());
            static const QRegularExpression re(
                QStringLiteral("Write Speed\\s*:\\s*(.+)$"));
            const QRegularExpressionMatch m = re.match(out);
            if (m.hasMatch()) {
                const QStringList parts =
                    m.captured(1).split(QLatin1Char(','),
                                        Qt::SkipEmptyParts);
                for (QString s : parts) {
                    s = s.trimmed();
                    if (!s.isEmpty())
                        speeds << s;
                }
            }
        }
    }
    return speeds;
}

// ---------- 作业队列 ----------
void Backend::queueJob(const QueuedJob &job)
{
    m_queue.enqueue(job);
    if (!m_finishing) {
        m_finishing = true;
        runNext();
    }
}

void Backend::runNext()
{
    if (m_queue.isEmpty())
        return;
    const QueuedJob job = m_queue.head();
    if (demoMode) {
        startDemoJob(job);
        return;
    }
    m_runner.start(job.program, job.args, job.kind, job.title, job.totalBytes);
}

void Backend::startDemoJob(const QueuedJob &job)
{
    QString desc;
    switch (job.kind) {
    case JobKind::CreateIso: desc = QStringLiteral("制作 ISO 镜像"); break;
    case JobKind::BurnIso:
    case JobKind::BurnData:  desc = QStringLiteral("刻录数据光盘"); break;
    case JobKind::BurnAudio: desc = QStringLiteral("刻录音乐 CD"); break;
    case JobKind::CopyRead:
    case JobKind::CopyWrite: desc = QStringLiteral("复制光盘"); break;
    case JobKind::Erase:     desc = QStringLiteral("擦除光盘"); break;
    default: break;
    }
    m_runner.startDemo(job.kind, job.title, desc);
}

void Backend::cancel()
{
    m_runner.cancel();
}

// ---------- 具体操作 ----------
bool Backend::createIso(const QStringList &sources, const QString &outIso,
                        const QString &label, bool joliet, bool rockRidge)
{
    if (sources.isEmpty() || outIso.isEmpty())
        return false;

    QStringList args;
    if (rockRidge) args << QStringLiteral("-r");
    if (joliet)    args << QStringLiteral("-J") << QStringLiteral("-joliet-long");
    args << QStringLiteral("-iso-level") << QStringLiteral("3");
    if (!label.isEmpty())
        args << QStringLiteral("-V") << label;
    args << QStringLiteral("-o") << outIso;
    args << sources;

    QueuedJob j;
    j.kind = JobKind::CreateIso;
    j.program = haveGenisoimage ? QStringLiteral("genisoimage")
                                : QStringLiteral("mkisofs");
    j.args = args;
    j.title = QStringLiteral("制作 ISO 镜像：%1").arg(outIso);
    queueJob(j);
    return true;
}

bool Backend::burnIso(const QString &dev, const QString &iso,
                      const QString &speed, bool eject)
{
    QFileInfo fi(iso);
    if (!fi.exists()) {
        emit logLine(QStringLiteral("找不到 ISO 镜像文件：%1").arg(iso), true);
        return false;
    }

    MediaStatus ms = detectMedia(dev);
    const bool isCd = (ms.mediaClass == MediaClass::CdR ||
                       ms.mediaClass == MediaClass::CdRw ||
                       ms.mediaClass == MediaClass::CdRom);

    QueuedJob j;
    j.kind = JobKind::BurnIso;
    const QString sp = speedArg(speed);
    if (haveGrowisofs && !isCd) {
        // DVD / BD / 未知 → growisofs
        QStringList args;
        if (!sp.isEmpty())
            args << QStringLiteral("-speed=") + sp;
        args << QStringLiteral("-Z") << dev + QStringLiteral("=") + iso;
        if (eject)
            args << QStringLiteral("-Eject");
        j.program = QStringLiteral("growisofs");
        j.args = args;
        j.title = QStringLiteral("烧录 ISO 镜像到 %1").arg(dev);
    } else {
        // CD → wodim（TAO 模式 + 显式 FIFO 大小：
        // 规避 cdrkit 默认缓冲区 mmap 在低内存锁限制下的 EAGAIN 失败）
        QStringList args;
        args << QStringLiteral("-v") << QStringLiteral("fs=6000");
        if (!sp.isEmpty())
            args << QStringLiteral("-speed=") + sp;
        if (eject)
            args << QStringLiteral("-eject");
        args << QStringLiteral("dev=") + dev << iso;
        j.program = QStringLiteral("wodim");
        j.args = args;
        j.title = QStringLiteral("烧录 ISO 镜像到 %1").arg(dev);
    }
    queueJob(j);
    return true;
}

bool Backend::burnData(const QString &dev, const QStringList &sources,
                       const QString &label, const QString &speed,
                       bool multiSession, bool eject)
{
    if (sources.isEmpty())
        return false;

    MediaStatus ms = detectMedia(dev);
    if (!ms.present) {
        emit logLine(QStringLiteral("设备 %1 中没有可写的光盘介质。").arg(dev), true);
        return false;
    }

    const bool isCd = (ms.mediaClass == MediaClass::CdR ||
                       ms.mediaClass == MediaClass::CdRw ||
                       ms.mediaClass == MediaClass::CdRom);

    const QString sp = speedArg(speed);
    if (haveGrowisofs && !isCd) {
        // DVD / BD：growisofs 直接写入，不生成临时镜像
        QStringList args;
        if (!sp.isEmpty())
            args << QStringLiteral("-speed=") + sp;
        // 多会话 / 追加
        if (multiSession && ms.appendable)
            args << QStringLiteral("-M") << dev;
        else
            args << QStringLiteral("-Z") << dev;
        args << QStringLiteral("-r") << QStringLiteral("-J");
        args << QStringLiteral("-V") << label;
        args << sources;

        QueuedJob j;
        j.kind = JobKind::BurnData;
        j.program = QStringLiteral("growisofs");
        j.args = args;
        j.title = QStringLiteral("刻录数据光盘到 %1").arg(dev);
        queueJob(j);
        return true;
    }

    // CD（或没有 growisofs）：先生成临时 ISO，再写盘
    const QString tmp = findTempIso(QStringLiteral("data"));
    QueuedJob j;
    j.kind = JobKind::CreateIso;
    j.program = haveGenisoimage ? QStringLiteral("genisoimage")
                                : QStringLiteral("mkisofs");
    QStringList args;
    args << QStringLiteral("-r") << QStringLiteral("-J") << QStringLiteral("-joliet-long");
    args << QStringLiteral("-iso-level") << QStringLiteral("3");
    if (!label.isEmpty())
        args << QStringLiteral("-V") << label;
    args << QStringLiteral("-o") << tmp << sources;
    j.args = args;
    j.title = QStringLiteral("制作数据光盘镜像（CD）");
    j.after = [this, dev, tmp, speed, eject, multiSession]() {
        QStringList wArgs;
        wArgs << QStringLiteral("-v") << QStringLiteral("fs=6000");
        if (multiSession)
            wArgs << QStringLiteral("-multi");
        const QString sp = speedArg(speed);
        if (!sp.isEmpty())
            wArgs << QStringLiteral("-speed=") + sp;
        if (eject)
            wArgs << QStringLiteral("-eject");
        wArgs << QStringLiteral("dev=") + dev << tmp;
        QueuedJob w;
        w.kind = JobKind::BurnData;
        w.program = QStringLiteral("wodim");
        w.args = wArgs;
        w.title = QStringLiteral("刻录数据光盘到 %1").arg(dev);
        queueJob(w);
    };
    queueJob(j);
    return true;
}

bool Backend::burnAudio(const QString &dev, const QStringList &wavFiles,
                        const QString &speed, bool eject)
{
    if (wavFiles.isEmpty())
        return false;

    QStringList args;
    // TAO 模式刻录音频（兼容性最好；DAO 在老光驱上常见 CUE sheet 拒绝问题）
    args << QStringLiteral("-v") << QStringLiteral("-audio")
         << QStringLiteral("fs=6000");
    const QString sp = speedArg(speed);
    if (!sp.isEmpty())
        args << QStringLiteral("-speed=") + sp;
    if (eject)
        args << QStringLiteral("-eject");
    args << QStringLiteral("dev=") + dev;
    args << wavFiles;

    QueuedJob j;
    j.kind = JobKind::BurnAudio;
    j.program = QStringLiteral("wodim");
    j.args = args;
    j.title = QStringLiteral("刻录音乐 CD 到 %1").arg(dev);
    queueJob(j);
    return true;
}

bool Backend::burnAudioProject(const QString &dev, const AudioProject &project,
                               const QString &speed, bool eject)
{
    if (project.tracks.isEmpty())
        return false;

    struct Conv { QString in; QString out; };
    QVector<Conv> convs;
    QStringList wavs;

    const QDir tmpDir(Settings::instance().tempImageDir);
    int idx = 0;
    for (const AudioTrack &t : project.tracks) {
        const QString p = t.url.toLocalFile();
        if (t.wavPath.isEmpty() == false && QFileInfo::exists(t.wavPath)) {
            wavs << t.wavPath;
        } else if (isWavFile(t.url)) {
            wavs << p;
        } else if (haveFfmpeg) {
            const QString out = tmpDir.filePath(
                QStringLiteral("track-%1-%2.wav").arg(idx).arg(
                    QFileInfo(p).completeBaseName()));
            convs.append({p, out});
            wavs << out;
        } else {
            emit logLine(QStringLiteral(
                "无法转换音轨「%1」：系统缺少 ffmpeg，请只添加 WAV 文件。")
                             .arg(t.title), true);
            return false;
        }
        ++idx;
    }

    if (convs.isEmpty()) {
        burnAudio(dev, wavs, speed, eject);
        return true;
    }

    // 依次入队转换作业，最后一张转完再入队刻录作业
    auto enqueueConv = std::make_shared<std::function<void(int)>>();
    *enqueueConv = [this, dev, wavs, speed, eject, convs, enqueueConv](int i) {
        QueuedJob c;
        c.kind = JobKind::ConvertAudio;
        c.program = QStringLiteral("ffmpeg");
        c.args = {QStringLiteral("-y"), QStringLiteral("-i"), convs[i].in,
                  QStringLiteral("-ac"), QStringLiteral("2"),
                  QStringLiteral("-ar"), QStringLiteral("44100"),
                  QStringLiteral("-sample_fmt"), QStringLiteral("s16"),
                  QStringLiteral("-b:a"), QStringLiteral("1411k"),
                  convs[i].out};
        c.title = QStringLiteral("转换音轨（%1/%2）：%3")
                      .arg(i + 1)
                      .arg(convs.size())
                      .arg(QFileInfo(convs[i].in).completeBaseName());
        if (i + 1 < convs.size()) {
            c.after = [enqueueConv, i]() { (*enqueueConv)(i + 1); };
        } else {
            c.after = [this, dev, wavs, speed, eject]() {
                burnAudio(dev, wavs, speed, eject);
            };
        }
        queueJob(c);
    };
    (*enqueueConv)(0);
    return true;
}

bool Backend::copyDisc(const QString &srcDev, const QString &dstDev,
                       const QString &speed, bool eject, CopyMode mode)
{
    MediaStatus src = detectMedia(srcDev);
    if (!src.present) {
        emit logLine(QStringLiteral("源设备 %1 中没有光盘。").arg(srcDev), true);
        return false;
    }

    const bool isAudio = (mode == CopyMode::AudioOnly) && haveCdrdao;

    if (isAudio) {
        // 音频盘：cdrdao 读取 TOC + 音轨，再写入
        QString base = findTempIso(QStringLiteral("copy"));
        base.chop(4); // 去掉 .iso
        const QString toc = base + QStringLiteral(".toc");
        const QString image = base + QStringLiteral(".bin");

        QueuedJob r;
        r.kind = JobKind::CopyRead;
        r.program = QStringLiteral("cdrdao");
        r.args = {QStringLiteral("read-cd"),
                  QStringLiteral("--device"), srcDev,
                  QStringLiteral("--driver"), QStringLiteral("generic-mmc-raw"),
                  QStringLiteral("--datafile"), image, toc};
        r.title = QStringLiteral("读取源音频光盘…");
        r.after = [this, dstDev, toc, speed, eject]() {
            QStringList wArgs;
            wArgs << QStringLiteral("write");
            if (!speed.isEmpty() && speed != QStringLiteral("自动"))
                wArgs << QStringLiteral("--speed") << speed;
            wArgs << QStringLiteral("--device") << dstDev << toc;
            QueuedJob w;
            w.kind = JobKind::CopyWrite;
            w.program = QStringLiteral("cdrdao");
            w.args = wArgs;
            w.title = QStringLiteral("写入目标光盘…");
            queueJob(w);
        };
        queueJob(r);
        return true;
    }

    // 数据盘：dd 整盘读出 → 再写盘
    const QString tmp = findTempIso(QStringLiteral("copy"));
    qint64 total = src.capacityMB * 1024 * 1024;

    QueuedJob r;
    r.kind = JobKind::CopyRead;
    r.program = QStringLiteral("dd");
    r.args = {QStringLiteral("if=") + srcDev, QStringLiteral("of=") + tmp,
              QStringLiteral("bs=1M"), QStringLiteral("status=progress"),
              QStringLiteral("conv=noerror,sync")};
    r.title = QStringLiteral("读取源光盘 → %1").arg(tmp);
    r.totalBytes = total;
    r.after = [this, dstDev, tmp, speed, eject]() {
        burnIso(dstDev, tmp, speed, eject);
    };
    queueJob(r);
    return true;
}

bool Backend::erase(const QString &dev, EraseMode mode)
{
    MediaStatus ms = detectMedia(dev);
    if (!ms.present) {
        emit logLine(QStringLiteral("设备 %1 中没有光盘，无法擦除。").arg(dev), true);
        return false;
    }

    QueuedJob j;
    j.kind = JobKind::Erase;
    j.title = QStringLiteral("擦除 %1 上的光盘").arg(dev);

    const bool isCd = (ms.mediaClass == MediaClass::CdR ||
                       ms.mediaClass == MediaClass::CdRw ||
                       ms.mediaClass == MediaClass::CdRom);

    if (isCd || (ms.mediaClass == MediaClass::DvdRw && haveWodim &&
                 mode != EraseMode::FormatForce)) {
        j.program = QStringLiteral("wodim");
        j.args = {QStringLiteral("dev=") + dev,
                  mode == EraseMode::Full ? QStringLiteral("blank=all")
                                          : QStringLiteral("blank=fast")};
    } else {
        // DVD+RW / DVD-RAM / BD-RE → dvd+rw-format
        j.program = QStringLiteral("dvd+rw-format");
        if (mode == EraseMode::Full || mode == EraseMode::FormatForce)
            j.args = {QStringLiteral("-blank=full"), dev};
        else
            j.args = {QStringLiteral("-blank=quick"), dev};
    }
    queueJob(j);
    return true;
}

bool Backend::ejectDisc(const QString &dev)
{
    if (!haveEject)
        return false;
    QProcess::startDetached(QStringLiteral("eject"), {dev});
    return true;
}

QString Backend::findTempIso(const QString &hint) const
{
    const QString stamp = QDateTime::currentDateTime().toString(
        QStringLiteral("yyyyMMdd-hhmmss"));
    const QString name = QStringLiteral("k3bcn-%1-%2.iso").arg(hint, stamp);
    return QDir(Settings::instance().tempImageDir).filePath(name);
}

} // namespace Burn
