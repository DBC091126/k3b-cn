// main.cpp — 刻录软件中文版（K3b 风格）入口
#include <QApplication>
#include <QFont>
#include <QTextStream>
#include <QLocale>

#include "MainWindow.h"
#include "Settings.h"
#include "Backend.h"
#include "Types.h"

using namespace Burn;

// 命令行自检：k3b-cn --check-media
static int runCheckMedia()
{
    QTextStream out(stdout);
    Backend b;
    b.refresh();

    out << QStringLiteral("== K3b 中文版 自检 ==\n");
    out << QStringLiteral("光驱：")
        << (b.devices.isEmpty() ? QStringLiteral("（无）") : b.devices.join(QStringLiteral("、")))
        << QLatin1Char('\n');
    out << QStringLiteral("工具：wodim=%1 genisoimage=%2 growisofs=%3 cdrdao=%4 "
                          "dvd+rw-mediainfo=%5 dd=%6 ffmpeg=%7 eject=%8\n")
               .arg(b.haveWodim ? QStringLiteral("有") : QStringLiteral("无"))
               .arg(b.haveGenisoimage ? QStringLiteral("有") : QStringLiteral("无"))
               .arg(b.haveGrowisofs ? QStringLiteral("有") : QStringLiteral("无"))
               .arg(b.haveCdrdao ? QStringLiteral("有") : QStringLiteral("无"))
               .arg(b.haveDvdMediainfo ? QStringLiteral("有") : QStringLiteral("无"))
               .arg(b.haveDd ? QStringLiteral("有") : QStringLiteral("无"))
               .arg(b.haveFfmpeg ? QStringLiteral("有") : QStringLiteral("无"))
               .arg(b.haveEject ? QStringLiteral("有") : QStringLiteral("无"));
    out << QStringLiteral("演示模式：") << (b.demoMode ? QStringLiteral("开") : QStringLiteral("关"))
        << QLatin1Char('\n');

    const MediaStatus ms = b.mediaStatus();
    out << QStringLiteral("介质：\n");
    out << QStringLiteral("  存在：") << (ms.present ? QStringLiteral("是") : QStringLiteral("否"))
        << QLatin1Char('\n');
    if (ms.present) {
        out << QStringLiteral("  类型：") << ms.mediaType
            << QStringLiteral("（") << mediaClassToText(ms.mediaClass) << QStringLiteral("）\n");
        out << QStringLiteral("  空白：") << (ms.blank ? QStringLiteral("是") : QStringLiteral("否"))
            << QStringLiteral("  可写：") << (ms.writable ? QStringLiteral("是") : QStringLiteral("否"))
            << QStringLiteral("  可擦除：") << (ms.erasable ? QStringLiteral("是") : QStringLiteral("否"))
            << QLatin1Char('\n');
        out << QStringLiteral("  容量：") << ms.capacityMB << QStringLiteral(" MB\n");
        out << QStringLiteral("  厂商：") << ms.vendor
            << QStringLiteral("  型号：") << ms.product << QLatin1Char('\n');
    }
    return 0;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("k3b-cn"));
    QApplication::setApplicationDisplayName(QStringLiteral("K3b 刻录软件（中文版）"));
    QApplication::setOrganizationName(QStringLiteral("k3b-cn"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));

    if (app.arguments().contains(QStringLiteral("--check-media")))
        return runCheckMedia();

    // 中文字体：优先使用系统中文字体，保证 CJK 显示
    QFont font = app.font();
    font.setFamily(QStringLiteral("Noto Sans CJK SC"));
    font.setPointSize(10);
    app.setFont(font);

    Settings::load();

    MainWindow win;
    win.resize(1080, 720);
    win.show();

    return app.exec();
}
