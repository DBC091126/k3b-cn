// AudioProjectPage.h — 音乐 CD 项目页
#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QUrl>
#include <QVector>

class QTreeWidgetItem;
class QLabel;
class QComboBox;

namespace Burn {

class Backend;
class AudioProject;

class AudioProjectPage : public QWidget
{
    Q_OBJECT
public:
    AudioProjectPage(Backend *backend, AudioProject *project, QWidget *parent = nullptr);

    void setProject(AudioProject *project);

public slots:
    void addTracks();
    void removeSelected();
    void clearProject();
    void requestBurn();

signals:
    void burnRequested(const QString &title);

private slots:
    void onFilesDropped(const QVector<QUrl> &urls);
    void onMediaChanged();

private:
    void addUrls(const QVector<QUrl> &urls);
    void syncFromTree();
    void rebuildTree();
    void refreshSummary();

    Backend *m_backend;
    AudioProject *m_project;
    QTreeWidget *m_tree;
    QLabel *m_durationLabel;
    QLabel *m_capacityLabel;
    QComboBox *m_speedCombo;
};

} // namespace Burn
