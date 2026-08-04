// Projects.h — 项目数据模型：数据光盘项目、音乐 CD 项目
#pragma once

#include <QObject>
#include <QUrl>
#include <QVector>
#include <QStringList>

namespace Burn {

// 数据光盘项目：一组文件/文件夹 + 卷标与格式设置
class DataProject : public QObject
{
    Q_OBJECT
public:
    explicit DataProject(QObject *parent = nullptr);

    QString name = QStringLiteral("新建数据项目");
    QString volumeLabel = QStringLiteral("MY_DATA");   // 卷标
    bool joliet = true;      // 生成 Joliet 文件名
    bool rockRidge = true;   // 生成 RockRidge 属性
    bool multiSession = false;

    QVector<QUrl> entries;

    bool empty() const { return entries.isEmpty(); }
    int entryCount() const { return entries.size(); }
    // 递归统计项目总大小（字节）
    qint64 totalSize() const;
};

// 一条音轨
struct AudioTrack {
    QUrl    url;
    QString title;           // 标题（默认文件名）
    qint64  durationSec = 0; // 时长（尽力探测，0 = 未知）
    qint64  size = 0;        // 字节数
    QString wavPath;         // 转码后的 wav 路径（为空表示无需转换）
};

// 音乐 CD 项目
class AudioProject : public QObject
{
    Q_OBJECT
public:
    explicit AudioProject(QObject *parent = nullptr);

    QString name = QStringLiteral("新建音乐 CD");
    QVector<AudioTrack> tracks;

    bool empty() const { return tracks.isEmpty(); }
    int trackCount() const { return tracks.size(); }
    qint64 totalDurationSec() const;
};

// 工具函数：探测音频文件时长与大小
void probeAudioFile(AudioTrack &track);

// 通用：递归统计文件夹大小
qint64 dirSize(const QString &path);

// 判断文件是否为 wav（按扩展名）
bool isWavFile(const QUrl &url);

} // namespace Burn
