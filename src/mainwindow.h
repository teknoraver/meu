#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "playlist.h"

class QAction;
class QComboBox;
class QLabel;
class QListWidget;
class QToolBar;

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget *parent = nullptr);

	void openFiles(const QStringList &paths);

protected:
	void closeEvent(QCloseEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

private:
	void open();
	void addFiles();
	void removeSelected();
	void clear();
	bool save();
	bool saveAs();
	void about();

	void openPlaylist(const QString &path);
	bool readPlaylist(const QString &path, QList<Playlist::Entry> &newEntries);
	void collect(const QString &path, QList<Playlist::Entry> &newEntries);
	bool writePlaylist(const QString &path);
	bool maybeSave();

	void merge(const QList<Playlist::Entry> &newEntries);
	void refreshView();
	void updateActions();
	QString lastDir() const;
	void setLastDir(const QString &path);
	void setCurrentFile(const QString &path);
	void fitToolBar();

	QList<Playlist::Entry> entries;
	QString currentFile;

	QListWidget *list;
	QComboBox *sortCombo;
	QLabel *countLabel;
	QToolBar *toolBar;
	QAction *openAction;
	QAction *addAction;
	QAction *removeAction;
	QAction *clearAction;
	QAction *saveAction;
	QAction *saveAsAction;
};

#endif
