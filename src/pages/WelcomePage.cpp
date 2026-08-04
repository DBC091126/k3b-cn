// WelcomePage.cpp — 欢迎页实现
#include "WelcomePage.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <functional>

#include "../Icons.h"
#include "../Backend.h"

namespace Burn {

// 可点击的项目卡片
class CardButton : public QWidget
{
    Q_OBJECT
public:
    CardButton(const QIcon &icon, const QString &title, const QString &desc,
               QWidget *parent = nullptr)
        : QWidget(parent), m_icon(icon), m_title(title), m_desc(desc)
    {
        setFixedSize(252, 168);
        setCursor(Qt::PointingHandCursor);
        setAttribute(Qt::WA_Hover);
    }

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRectF r = rect().adjusted(1, 1, -1, -1);
        p.setBrush(m_hover ? QColor(236, 242, 255) : QColor(248, 249, 251));
        p.setPen(QPen(m_hover ? QColor(41, 121, 255) : QColor(205, 208, 215), 1.2));
        p.drawRoundedRect(r, 12, 12);

        const int s = 56;
        p.drawPixmap((width() - s) / 2, 18, m_icon.pixmap(s, s));

        QFont f = font();
        f.setPointSize(11);
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(40, 42, 48));
        p.drawText(QRect(6, 84, width() - 12, 26), Qt::AlignCenter, m_title);

        f.setPointSize(9);
        f.setBold(false);
        p.setFont(f);
        p.setPen(QColor(120, 122, 130));
        p.drawText(QRect(12, 110, width() - 24, 48),
                   Qt::AlignHCenter | Qt::TextWordWrap, m_desc);
    }

    void enterEvent(QEnterEvent *) override
    {
        m_hover = true;
        update();
    }
    void leaveEvent(QEvent *) override
    {
        m_hover = false;
        update();
    }
    void mouseReleaseEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && rect().contains(e->pos()))
            emit clicked();
    }

private:
    QIcon m_icon;
    QString m_title;
    QString m_desc;
    bool m_hover = false;
};

WelcomePage::WelcomePage(Backend *backend, QWidget *parent)
    : QWidget(parent), m_backend(backend)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(36, 30, 36, 24);
    layout->setSpacing(6);

    auto *title = new QLabel(QStringLiteral("欢迎使用 K3b 刻录软件（中文版）"), this);
    QFont tf = title->font();
    tf.setPointSize(20);
    tf.setBold(true);
    title->setFont(tf);
    layout->addWidget(title);

    auto *sub = new QLabel(
        QStringLiteral("选择一项操作开始。把文件或文件夹拖入数据项目即可快速刻录。"),
        this);
    sub->setStyleSheet(QStringLiteral("color:#7a7d85;"));
    layout->addWidget(sub);

    layout->addSpacing(22);

    auto *grid = new QGridLayout;
    grid->setSpacing(16);

    const auto addCard = [this, grid](const QIcon &icon, const QString &title,
                                      const QString &desc, int row, int col,
                                      const std::function<void()> &action) {
        auto *card = new CardButton(icon, title, desc, this);
        connect(card, &CardButton::clicked, this, [action]() { action(); });
        grid->addWidget(card, row, col);
    };

    addCard(Icons::dataDisc(), QStringLiteral("新建数据光盘项目"),
            QStringLiteral("把文件或文件夹刻录成数据光盘，或生成 ISO 镜像。"),
            0, 0, [this]() { emit newDataProject(); });
    addCard(Icons::musicDisc(), QStringLiteral("新建音乐 CD 项目"),
            QStringLiteral("把音乐文件刻录成可在 CD 播放器播放的音乐 CD。"),
            0, 1, [this]() { emit newAudioProject(); });
    addCard(Icons::copyDisc(), QStringLiteral("复制光盘"),
            QStringLiteral("整盘复制源光盘到另一张可写光盘。"),
            0, 2, [this]() { emit openCopy(); });
    addCard(Icons::isoImage(), QStringLiteral("ISO 镜像工具"),
            QStringLiteral("制作 ISO 镜像，或把镜像烧录到光盘。"),
            1, 0, [this]() { emit openIso(); });
    addCard(Icons::mediaInfo(), QStringLiteral("介质信息"),
            QStringLiteral("查看光驱与光盘介质的详细信息。"),
            1, 1, [this]() { emit openMedia(); });
    addCard(Icons::erase(), QStringLiteral("擦除可重写光盘"),
            QStringLiteral("擦除 CD-RW、DVD±RW、BD-RE 等可重写介质。"),
            1, 2, [this]() { emit openErase(); });

    layout->addLayout(grid);
    layout->addStretch();
}

} // namespace Burn

#include "WelcomePage.moc"
