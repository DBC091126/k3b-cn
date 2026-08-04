// JobRunner.cpp — 作业执行器实现
#include "JobRunner.h"

#include <QFileInfo>
#include <QRegularExpression>

namespace Burn {

JobRunner::JobRunner(QObject *parent)
    : QObject(parent)
{
}

JobRunner::~JobRunner()
{
    if (m_proc) {
        m_proc->kill();
        delete m_proc;
    }
    delete m_demoTimer;
}

bool JobRunner::start(const QString &program, const QStringList &args,
                      JobKind kind, const QString &title, qint64 totalBytes)
{
    if (m_proc)
        return false;

    m_kind = kind;
    m_title = title;
    m_totalBytes = totalBytes;
    m_lastPercent = -1;
    m_cancelled = false;

    m_proc = new QProcess(this);
    m_proc->setProgram(program);
    m_proc->setArguments(args);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_proc, &QProcess::readyReadStandardOutput, this, &JobRunner::onStdout);
    connect(m_proc, &QProcess::readyReadStandardError, this, &JobRunner::onStderr);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &JobRunner::onProcessFinished);

    emit stateChanged(QStringLiteral("启动：%1").arg(title));
    m_proc->start();

    if (!m_proc->waitForStarted(3000)) {
        emit logLine(QStringLiteral("无法启动命令：%1（%2）")
                         .arg(program, m_proc->errorString()), true);
        delete m_proc;
        m_proc = nullptr;
        emit finished(false, -1);
        return false;
    }
    return true;
}

void JobRunner::startDemo(JobKind kind, const QString &title, const QString &desc)
{
    if (m_proc)
        return;

    m_kind = kind;
    m_title = title;
    m_lastPercent = -1;
    m_cancelled = false;
    m_demoKind = kind;
    m_demoDesc = desc;
    m_demoTick = 0;
    m_demoSteps = 90;  // 90 步，约 12 秒走完

    emit logLine(QStringLiteral("—— 演示模式：未检测到 %1 ——")
                     .arg(desc.isEmpty() ? QStringLiteral("可用工具/光驱") : desc),
                 false);
    emit logLine(QStringLiteral("（本系统缺少可用的刻录工具或光驱，以下为模拟刻录演示）"),
                 false);

    m_demoTimer = new QTimer(this);
    connect(m_demoTimer, &QTimer::timeout, this, &JobRunner::onDemoTick);
    m_demoTimer->start(130);
}

void JobRunner::appendLog(const QString &line, bool isError)
{
    emit logLine(line, isError);
}

void JobRunner::cancel()
{
    m_cancelled = true;
    if (m_proc) {
        emit stateChanged(QStringLiteral("正在中止作业…"));
        emit logLine(QStringLiteral("用户请求取消，正在终止刻录进程…"), false);
        m_proc->terminate();
        QTimer::singleShot(2000, m_proc, &QProcess::kill);
    } else if (m_demoTimer) {
        emit logLine(QStringLiteral("演示作业已取消。"), false);
        m_demoTimer->stop();
        emit finished(false, 1);
    }
}

void JobRunner::onStdout()
{
    const QByteArray data = m_proc->readAllStandardOutput();
    const QString text = QString::fromUtf8(data);
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (!line.isEmpty())
            parseLine(line, false);
    }
}

void JobRunner::onStderr()
{
    const QByteArray data = m_proc->readAllStandardError();
    const QString text = QString::fromUtf8(data);
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (!line.isEmpty())
            parseLine(line, true);
    }
}

void JobRunner::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    const bool ok = (status == QProcess::NormalExit && exitCode == 0 && !m_cancelled);
    emit stateChanged(ok ? QStringLiteral("完成")
                         : QStringLiteral("失败（退出码 %1）").arg(exitCode));
    emit logLine(QStringLiteral("—— 作业结束，%1 ——")
                     .arg(ok ? QStringLiteral("成功") : QStringLiteral("失败")),
                 !ok);

    if (m_proc) {
        delete m_proc;
        m_proc = nullptr;
    }
    emit finished(ok, exitCode);
}

void JobRunner::onDemoTick()
{
    ++m_demoTick;
    const int percent = m_demoTick * 100 / m_demoSteps;

    if (m_demoTick == 1) {
        emit logLine(QStringLiteral("Executing 'mkisofs/growisofs' (模拟)"), false);
        emit logLine(QStringLiteral("正在扫描源文件…"), false);
    }

    if (m_demoTick % 3 == 0) {
        // 模拟 growisofs 进度输出
        const qint64 total = 2673545;
        const qint64 done = total * percent / 100;
        emit logLine(QString::asprintf("%10lld/ %10lld ( %5.1f%%)  %.1fx",
                                       done, total, double(percent), 2.2 + percent / 100.0),
                     false);
    }
    emit progress(percent);
    emit detailChanged(QStringLiteral("正在写入… %1%").arg(percent));

    if (m_demoTick >= m_demoSteps) {
        emit progress(100);
        emit logLine(QStringLiteral("演示刻录完成（%1）").arg(m_demoDesc), false);
        emit stateChanged(QStringLiteral("演示完成"));
        m_demoTimer->stop();
        delete m_demoTimer;
        m_demoTimer = nullptr;
        emit finished(true, 0);
    }
}

// 将一行输出转换为日志 + 尝试解析进度
void JobRunner::parseLine(const QString &raw, bool isErr)
{
    QString line = raw.trimmed();
    if (line.isEmpty())
        return;

    // 去掉 ANSI 颜色转义序列（部分工具会输出）
    line.remove(QRegularExpression(QStringLiteral("\\x1B\\[[0-9;]*[A-Za-z]")));

    emit logLine(line, isErr);
    parseProgress(line);
}

void JobRunner::parseProgress(const QString &line)
{
    int percent = -1;

    // 1) growisofs / cdrdao 形式： "( 27.2%)" 或 "( 69 %)"
    static const QRegularExpression reParen(
        QStringLiteral("\\(\\s*([0-9]{1,3}(?:\\.[0-9]+)?)\\s*%\\)"));
    QRegularExpressionMatch m = reParen.match(line);
    if (m.hasMatch()) {
        percent = qBound(0, qRound(m.captured(1).toDouble()), 100);
    }

    // 2) wodim 形式："Track 01: 22% written" 或 " 45 of 200 MB written ( 22%)"
    if (percent < 0) {
        static const QRegularExpression reWodim(
            QStringLiteral("([0-9]{1,3})\\s*%\\s*written"));
        m = reWodim.match(line);
        if (m.hasMatch())
            percent = qBound(0, m.captured(1).toInt(), 100);
    }

    // 3) dd 形式："123456 bytes (123 MB) copied, ..."（需要已知总字节数）
    if (percent < 0 && m_totalBytes > 0 && m_kind == JobKind::CopyRead) {
        const int bytes = parseDdBytes(line);
        if (bytes >= 0)
            percent = qBound(0, int(qreal(bytes) * 100.0 / m_totalBytes), 100);
    }

    if (percent >= 0 && percent != m_lastPercent) {
        m_lastPercent = percent;
        emit progress(percent);
    }
}

int JobRunner::parseDdBytes(const QString &line)
{
    static const QRegularExpression reBytes(
        QStringLiteral("^\\s*([0-9,]+)\\s+bytes\\s+\\(.*\\)\\s+copied"));
    QRegularExpressionMatch m = reBytes.match(line);
    if (!m.hasMatch())
        return -1;
    QString num = m.captured(1);
    num.remove(QLatin1Char(','));
    bool ok = false;
    const qint64 v = num.toLongLong(&ok);
    return ok ? static_cast<int>(v) : -1;
}

} // namespace Burn
