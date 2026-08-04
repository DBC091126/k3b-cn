// BurnJobPage.cpp — 烧录进度页实现
#include "BurnJobPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QFontDatabase>
#include <QDateTime>

#include "../Backend.h"

namespace Burn {

BurnJobPage::BurnJobPage(Backend *backend, QWidget *parent)
    : QWidget(parent), m_backend(backend)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);

    m_titleLabel = new QLabel(QStringLiteral("刻录作业"), this);
    QFont tf = m_titleLabel->font();
    tf.setPointSize(16);
    tf.setBold(true);
    m_titleLabel->setFont(tf);
    layout->addWidget(m_titleLabel);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setMinimumHeight(26);
    m_progress->setTextVisible(true);
    layout->addWidget(m_progress);

    m_detailLabel = new QLabel(QStringLiteral("准备中…"), this);
    m_detailLabel->setStyleSheet(QStringLiteral("color:#555;"));
    layout->addWidget(m_detailLabel);

    auto *logCaption = new QLabel(QStringLiteral("详细日志："), this);
    layout->addWidget(logCaption);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(5000);
    m_log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_log, 1);

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    m_cancelBtn = new QPushButton(QStringLiteral("取消刻录"), this);
    m_backBtn = new QPushButton(QStringLiteral("返回主界面"), this);
    m_backBtn->setEnabled(false);
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_backBtn);
    layout->addLayout(btnRow);

    connect(m_cancelBtn, &QPushButton::clicked, this, &BurnJobPage::onCancel);
    connect(m_backBtn, &QPushButton::clicked, this, &BurnJobPage::backRequested);

    // 关联后端信号
    connect(m_backend, &Backend::logLine, this, &BurnJobPage::onLog);
    connect(m_backend, &Backend::progress, this, &BurnJobPage::onProgress);
    connect(m_backend, &Backend::detailChanged, this, &BurnJobPage::onDetail);
    connect(m_backend, &Backend::stateChanged, this, &BurnJobPage::onState);
    connect(m_backend, &Backend::jobFinished, this, &BurnJobPage::onFinished);
}

void BurnJobPage::showJob(const QString &title)
{
    m_titleLabel->setText(title);
    m_progress->setValue(0);
    m_detailLabel->setText(QStringLiteral("准备中…"));
    m_log->clear();
    m_cancelBtn->setEnabled(true);
    m_backBtn->setEnabled(false);
    m_done = false;
}

void BurnJobPage::onLog(const QString &line, bool isError)
{
    if (!m_done) {
        const QString ts = QDateTime::currentDateTime().toString(
            QStringLiteral("HH:mm:ss"));
        m_log->appendPlainText(
            QStringLiteral("[%1] %2").arg(ts, line));
        // 第一次有日志出现说明作业已开始
        if (m_progress->value() == 0 && !m_cancelBtn->isEnabled())
            m_cancelBtn->setEnabled(true);
    }
}

void BurnJobPage::onProgress(int percent)
{
    if (percent < 0) {
        m_progress->setRange(0, 0);  // 不定进度
        m_progress->setTextVisible(false);
    } else {
        m_progress->setRange(0, 100);
        m_progress->setTextVisible(true);
        m_progress->setValue(percent);
        m_progress->setFormat(QStringLiteral("%p%"));
    }
}

void BurnJobPage::onDetail(const QString &text)
{
    m_detailLabel->setText(text);
}

void BurnJobPage::onState(const QString &text)
{
    m_detailLabel->setText(text);
}

void BurnJobPage::onFinished(bool ok, int code)
{
    setDone(ok);
    m_detailLabel->setText(
        ok ? QStringLiteral("作业成功完成 ✔")
           : QStringLiteral("作业失败（退出码 %1），请查看上方日志。").arg(code));
}

void BurnJobPage::setDone(bool ok)
{
    m_done = true;
    m_cancelBtn->setEnabled(false);
    m_backBtn->setEnabled(true);
    m_progress->setRange(0, 100);
    m_progress->setTextVisible(true);
    m_progress->setValue(ok ? 100 : 0);
}

void BurnJobPage::onCancel()
{
    m_cancelBtn->setEnabled(false);
    m_detailLabel->setText(QStringLiteral("正在取消…"));
    m_backend->cancel();
}

} // namespace Burn
