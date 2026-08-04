// Types.h — 公共类型定义：作业类型、介质信息、设置项等
#pragma once

#include <QString>
#include <QStringList>
#include <QMetaType>

namespace Burn {

// 作业类型
enum class JobKind {
    None,
    CreateIso,   // 制作 ISO 镜像
    BurnIso,     // 烧录 ISO 镜像
    BurnData,    // 刻录数据光盘
    BurnAudio,   // 刻录音乐 CD
    ConvertAudio,// 音频格式转换（ffmpeg）
    CopyRead,    // 复制光盘（读取源盘）
    CopyWrite,   // 复制光盘（写入目标盘）
    Erase,       // 擦除光盘
    MediaInfo,   // 检测介质信息
    Demo         // 演示模式（模拟刻录）
};

// 光盘介质类型（粗略分类）
enum class MediaClass {
    Unknown,
    CdRom,    // 只读 CD
    CdR,      // CD-R
    CdRw,     // CD-RW
    DvdRom,   // 只读 DVD
    DvdR,     // DVD-R / DVD+R
    DvdRw,    // DVD-RW / DVD+RW
    DvdRam,   // DVD-RAM
    BdR,      // BD-R
    BdRe,     // BD-RE
    None      // 无介质
};

// 介质状态
struct MediaStatus {
    bool    valid = false;      // 检测是否成功
    bool    present = false;    // 是否有光盘
    bool    writable = false;   // 是否可写介质
    bool    blank = false;      // 是否为空白光盘
    bool    appendable = false; // 是否可续刻
    bool    erasable = false;   // 是否可擦除（可重写）
    int     discStatus = -1;    // wodim 状态码（-1 未知）
    QString mediaType;          // 介质类型字符串，如 "DVD-RW Sequential"
    MediaClass mediaClass = MediaClass::Unknown;
    QString vendor;             // 驱动器厂商
    QString product;            // 驱动器型号
    QString device;             // 设备路径，如 /dev/sr0
    qint64  capacityMB = 0;     // 理论容量（MB，数据盘按 2048 B/扇区）
    qint64  sectors = 0;        // 介质总扇区数（CD 由 ATIP lead-out 得到）
    qint64  usedMB = 0;         // 已用容量（MB）
    qint64  freeMB = 0;         // 剩余容量（MB）
};

// 一条日志
struct LogEntry {
    QString text;
    bool    isError = false;
};

// 擦除方式
enum class EraseMode { Fast, Full, FormatForce };

// 复制方式
enum class CopyMode { Auto, DataOnly, AudioOnly };

// 通用辅助函数
inline QString mediaClassToText(MediaClass c)
{
    switch (c) {
    case MediaClass::CdRom:  return QStringLiteral("只读 CD");
    case MediaClass::CdR:    return QStringLiteral("CD-R");
    case MediaClass::CdRw:   return QStringLiteral("CD-RW");
    case MediaClass::DvdRom: return QStringLiteral("只读 DVD");
    case MediaClass::DvdR:   return QStringLiteral("DVD ± R");
    case MediaClass::DvdRw:  return QStringLiteral("DVD ± RW");
    case MediaClass::DvdRam: return QStringLiteral("DVD-RAM");
    case MediaClass::BdR:    return QStringLiteral("BD-R");
    case MediaClass::BdRe:   return QStringLiteral("BD-RE");
    case MediaClass::None:   return QStringLiteral("无介质");
    default:                 return QStringLiteral("未知介质");
    }
}

// 字节大小格式化：1048576 -> "1.0 GiB"
inline QString formatBytes(qint64 bytes)
{
    if (bytes <= 0) return QStringLiteral("0 B");
    const double kb = 1024.0, mb = kb * 1024, gb = mb * 1024, tb = gb * 1024;
    if (bytes >= tb) return QString::asprintf("%.2f TiB", bytes / tb);
    if (bytes >= gb) return QString::asprintf("%.2f GiB", bytes / gb);
    if (bytes >= mb) return QString::asprintf("%.1f MiB", bytes / mb);
    if (bytes >= kb) return QString::asprintf("%.1f KiB", bytes / kb);
    return QString::number(bytes) + QStringLiteral(" B");
}

// 写入速度预置选项（所有速度下拉框共用）：自动 / 1x / 4x
inline QStringList speedPresetOptions()
{
    return {QStringLiteral("自动"), QStringLiteral("1x"), QStringLiteral("4x")};
}

// 把 UI 速度选项转为 wodim/growisofs 的 -speed 参数值：
// "4x" → "4"，"自动" → 空（表示不指定）
inline QString speedArg(const QString &s)
{
    QString t = s.trimmed();
    if (t.isEmpty() || t == QStringLiteral("自动"))
        return QString();
    if (t.endsWith(QLatin1Char('x'), Qt::CaseInsensitive))
        t.chop(1);
    return t;
}

} // namespace Burn
