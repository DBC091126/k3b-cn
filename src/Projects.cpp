// Projects.cpp — 项目数据模型实现
#include "Projects.h"

#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QProcess>

namespace Burn {

DataProject::DataProject(QObject *parent)
    : QObject(parent)
{
}

AudioProject::AudioProject(QObject *parent)
    : QObject(parent)
{
}

qint64 DataProject::totalSize() const
{
    qint64 total = 0;
    for (const QUrl &u : entries) {
        const QString p = u.toLocalFile();
        const QFileInfo fi(p);
        if (!fi.exists())
            continue;
        if (fi.isDir())
            total += dirSize(p);
        else
            total += fi.size();
    }
    return total;
}

qint64 AudioProject::totalDurationSec() const
{
    qint64 s = 0;
    for (const AudioTrack &t : tracks)
        s += t.durationSec;
    return s;
}

qint64 dirSize(const QString &path)
{
    qint64 total = 0;
    QDirIterator it(path, QDir::Files | QDir::Hidden | QDir::System,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    return total;
}

bool isWavFile(const QUrl &url)
{
    const QString ext = QFileInfo(url.toLocalFile()).suffix().toLower();
    return ext == QStringLiteral("wav");
}

void probeAudioFile(AudioTrack &track)
{
    const QString p = track.url.toLocalFile();
    const QFileInfo fi(p);
    if (!fi.exists())
        return;
    track.size = fi.size();
    if (track.title.isEmpty())
        track.title = fi.completeBaseName();
    track.wavPath = isWavFile(track.url) ? p : QString();

    // 尽力探测时长：
    // 1) WAV：直接读 RIFF 头
    if (track.wavPath.isEmpty() == false) {
        QFile f(p);
        if (f.open(QIODevice::ReadOnly)) {
            // RIFF 头 12 字节后为 fmt 块
            QByteArray head = f.read(12);
            if (head.startsWith("RIFF") && head.mid(8, 4) == "WAVE") {
                while (!f.atEnd()) {
                    QByteArray id = f.read(4);
                    if (id.size() < 4)
                        break;
                    QByteArray szb = f.read(4);
                    if (szb.size() < 4)
                        break;
                    quint32 chunk = 0;
                    for (int i = 0; i < 4; ++i)
                        chunk |= quint32(uchar(szb.at(i))) << (8 * i);
                    if (id == "fmt " && chunk >= 16) {
                        QByteArray fmt = f.read(16);
                        quint16 bits = quint16(uchar(fmt.at(14))) |
                                       (quint16(uchar(fmt.at(15))) << 8);
                        quint32 rate = 0;
                        for (int i = 0; i < 4; ++i)
                            rate |= quint32(uchar(fmt.at(4 + i))) << (8 * i);
                        quint16 ch = quint16(uchar(fmt.at(2))) |
                                     (quint16(uchar(fmt.at(3))) << 8);
                        qint64 dataBytes = track.size - 44; // 近似
                        if (rate > 0 && ch > 0)
                            track.durationSec = dataBytes / (qint64(rate) * ch * (bits / 8));
                        break;
                    }
                    f.seek(f.pos() + chunk + (chunk & 1));
                }
            }
            f.close();
        }
    }
    // 2) ffprobe（若有）探测 mp3/ogg/flac 等
    if (track.durationSec <= 0) {
        QProcess fp;
        fp.setProgram(QStringLiteral("ffprobe"));
        fp.setArguments({QStringLiteral("-v"), QStringLiteral("error"),
                         QStringLiteral("-show_entries"),
                         QStringLiteral("format=duration"),
                         QStringLiteral("-of"), QStringLiteral("default=nw=1:nk=1"),
                         p});
        fp.start();
        if (fp.waitForFinished(4000)) {
            const QString out = QString::fromUtf8(fp.readAllStandardOutput()).trimmed();
            bool ok = false;
            const double d = out.toDouble(&ok);
            if (ok && d > 0)
                track.durationSec = qint64(d + 0.5);
        }
    }
    // 3) 粗略估算：假设平均码率 128kbps
    if (track.durationSec <= 0 && track.size > 0)
        track.durationSec = track.size * 8 / (128 * 1024);
}

} // namespace Burn
