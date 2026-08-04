// BurnJobPage.h — 烧录进度页：进度条 + 日志 + 取消
#pragma once

#include <QWidget>

class QLabel;
class QProgressBar;
class QPlainTextEdit;
class QPushButton;

namespace Burn {

class Backend;

class BurnJobPage : public QWidget
{
    Q_OBJECT
public:
    explicit BurnJobPage(Backend *backend, QWidget *parent = nullptr);

signals:
    void backRequested();

public slots:
    void showJob(const QString &title);

private slots:
    void onLog(const QString &line, bool isError);
    void onProgress(int percent);
    void onDetail(const QString &text);
    void onState(const QString &text);
    void onFinished(bool ok, int code);
    void onCancel();

private:
    void setDone(bool ok);

    Backend *m_backend;
    QLabel *m_titleLabel;
    QProgressBar *m_progress;
    QLabel *m_detailLabel;
    QPlainTextEdit *m_log;
    QPushButton *m_cancelBtn;
    QPushButton *m_backBtn;
    bool m_done = false;
};

} // namespace Burn
