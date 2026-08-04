// Settings.h — 应用设置（基于 QSettings 持久化）
#pragma once

#include <QString>

#include "Types.h"

namespace Burn {

struct Settings {
    // 刻录默认设置
    QString device = QStringLiteral("/dev/sr0");   // 默认刻录设备
    QString writeSpeed;                            // 写入速度（空 = 自动/最高）
    bool    ejectAfterBurn = true;                 // 刻录完成后弹出
    bool    verifyAfterBurn = false;               // 刻录后校验
    int     writeMode = 0;                         // 写入模式（0 自动）
    bool    multiSession = false;                  // 数据盘多会话
    QString tempImageDir;                          // 临时镜像目录

    // 复制默认设置
    CopyMode copyMode = CopyMode::Auto;

    // 加载 / 保存
    static Settings &instance();
    static void load();
    static void save();
};

} // namespace Burn
