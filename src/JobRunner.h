// JobRunner.h — 作业执行器：启动外部刻录工具、流式日志、进度解析、取消
#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>

#include "Types.h"

namespace Burn {

// 作业执行器：同一时间只运行一个作业。
// 输出被实时解析：growisofs / wodim / cdrdao / dd 的进度行会被转换为 0~100 的进度。
// 当真实工具不可用时（demo=true），会播放一段模拟输出。
class JobRunner : public QObject
{
    Q_OBJECT
public:
    explicit JobRunner(QObject *parent = nullptr);
    ~JobRunner();

    // 启动真实命令
    bool start(const QString &program, const QStringList &args,
               JobKind kind, const QString &title,
               qint64 totalBytes = 0);

    // 启动演示作业（模拟刻录进度与日志）
    void startDemo(JobKind kind, const QString &title, const QString &desc);

    // 追加一条日志（供外部直接写日志）
    void appendLog(const QString &line, bool isError = false);

    void cancel();
    bool running() const { return m_proc != nullptr; }
    QString title() const { return m_title; }
    int percent() const { return m_lastPercent; }

signals:
    void logLine(const QString &line, bool isError);
    void progress(int percent);                 // -1 表示不定进度
    void detailChanged(const QString &text);    // 状态栏/大字状态
    void finished(bool ok, int exitCode);
    void stateChanged(const QString &text);

private slots:
    void onStdout();
    void onStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onDemoTick();

private:
    void parseLine(const QString &raw, bool isErr);
    void parseProgress(const QString &line);
    int  parseDdBytes(const QString &line);

    QProcess *m_proc = nullptr;
    JobKind   m_kind = JobKind::None;
    QString   m_title;
    qint64    m_totalBytes = 0;
    int       m_lastPercent = -1;
    bool      m_cancelled = false;

    // 演示模式状态
    QTimer   *m_demoTimer = nullptr;
    int       m_demoTick = 0;
    int       m_demoSteps = 0;
    QString   m_demoDesc;
    JobKind   m_demoKind = JobKind::None;
};

} // namespace Burn
