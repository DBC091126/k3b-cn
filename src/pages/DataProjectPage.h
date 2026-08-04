// DataProjectPage.h — 数据光盘项目页：拖拽添加、树视图、容量、烧录
#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QUrl>
#include <QVector>

class QTreeWidgetItem;
class QLabel;
class QProgressBar;
class QLineEdit;
class QComboBox;

namespace Burn {

class Backend;
class DataProject;

// 支持从外部拖拽 URL 的树视图
class EntryTree : public QTreeWidget
{
    Q_OBJECT
public:
    explicit EntryTree(QWidget *parent = nullptr);

signals:
    void filesDropped(const QVector<QUrl> &urls);

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *e) override;
};

class DataProjectPage : public QWidget
{
    Q_OBJECT
public:
    DataProjectPage(Backend *backend, DataProject *project, QWidget *parent = nullptr);

    void setProject(DataProject *project);

public slots:
    void addFiles();
    void addFolder();
    void removeSelected();
    void clearProject();
    void requestBurn();

signals:
    void burnRequested(const QString &title);

private slots:
    void onFilesDropped(const QVector<QUrl> &urls);
    void onMediaChanged();

private:
    void rebuildTree();
    void syncFromTree();
    void addUrls(const QVector<QUrl> &urls);
    void refreshCapacity();
    bool confirmOverwrite();

    Backend *m_backend;
    DataProject *m_project;
    EntryTree *m_tree;
    QLabel *m_sizeLabel;
    QLabel *m_capacityLabel;
    QProgressBar *m_capacityBar;
    QLineEdit *m_volumeEdit;
    QComboBox *m_speedCombo;
};

} // namespace Burn
