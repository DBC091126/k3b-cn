// Settings.cpp — 设置读写实现
#include "Settings.h"

#include <QSettings>
#include <QStandardPaths>
#include <QDir>

namespace Burn {

Settings &Settings::instance()
{
    static Settings s;
    return s;
}

void Settings::load()
{
    QSettings cfg;
    auto &s = instance();

    s.device = cfg.value(QStringLiteral("device"),
                         QStringLiteral("/dev/sr0")).toString();
    s.writeSpeed = cfg.value(QStringLiteral("writeSpeed"), QString()).toString();
    s.ejectAfterBurn = cfg.value(QStringLiteral("ejectAfterBurn"), true).toBool();
    s.verifyAfterBurn = cfg.value(QStringLiteral("verifyAfterBurn"), false).toBool();
    s.writeMode = cfg.value(QStringLiteral("writeMode"), 0).toInt();
    s.multiSession = cfg.value(QStringLiteral("multiSession"), false).toBool();

    QString tmp = cfg.value(QStringLiteral("tempImageDir")).toString();
    if (tmp.isEmpty()) {
        tmp = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                  .filePath(QStringLiteral("k3b-cn"));
        QDir().mkpath(tmp);
    }
    s.tempImageDir = tmp;

    s.copyMode = static_cast<CopyMode>(
        cfg.value(QStringLiteral("copyMode"), 0).toInt());
}

void Settings::save()
{
    QSettings cfg;
    const auto &s = instance();

    cfg.setValue(QStringLiteral("device"), s.device);
    cfg.setValue(QStringLiteral("writeSpeed"), s.writeSpeed);
    cfg.setValue(QStringLiteral("ejectAfterBurn"), s.ejectAfterBurn);
    cfg.setValue(QStringLiteral("verifyAfterBurn"), s.verifyAfterBurn);
    cfg.setValue(QStringLiteral("writeMode"), s.writeMode);
    cfg.setValue(QStringLiteral("multiSession"), s.multiSession);
    cfg.setValue(QStringLiteral("tempImageDir"), s.tempImageDir);
    cfg.setValue(QStringLiteral("copyMode"), static_cast<int>(s.copyMode));
}

} // namespace Burn
