// WelcomePage.h — 欢迎页：K3b 风格项目入口卡片
#pragma once

#include <QWidget>

namespace Burn {

class Backend;

class WelcomePage : public QWidget
{
    Q_OBJECT
public:
    explicit WelcomePage(Backend *backend, QWidget *parent = nullptr);

signals:
    void newDataProject();
    void newAudioProject();
    void openCopy();
    void openIso();
    void openMedia();
    void openErase();

private:
    void addCard(const QIcon &icon, const QString &title,
                 const QString &desc, const char *slot);
    Backend *m_backend;
};

} // namespace Burn
