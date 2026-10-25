#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSet>
#include <QSettings>
#include <QStatusBar>
#include <QStyle>
#include <QToolBar>
#include <QUrl>

#include <algorithm>

#include "mainwindow.h"

using Playlist::Entry;
using Playlist::SortMode;

static const QStringList audioPatterns = { "*.mp3", "*.flac", "*.ogg", "*.opus", "*.m4a", "*.aac", "*.wav" };
static const QStringList playlistPatterns = { "*.m3u", "*.m3u8" };

static bool isPlaylist(const QString &path)
{
	return QDir::match(playlistPatterns, QFileInfo(path).fileName());
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	setCurrentFile(QString());
	setAcceptDrops(true);

	openAction = new QAction(QIcon::fromTheme("document-open"), tr("&Open…"), this);
	openAction->setShortcut(QKeySequence::Open);
	connect(openAction, &QAction::triggered, this, &MainWindow::open);

	addAction = new QAction(QIcon::fromTheme("list-add"), tr("&Add files…"), this);
	addAction->setShortcut(Qt::Key_Insert);
	connect(addAction, &QAction::triggered, this, &MainWindow::addFiles);

	removeAction = new QAction(QIcon::fromTheme("list-remove"), tr("&Remove selected"), this);
	removeAction->setShortcut(QKeySequence::Delete);
	connect(removeAction, &QAction::triggered, this, &MainWindow::removeSelected);

	clearAction = new QAction(QIcon::fromTheme("edit-clear"), tr("C&lear"), this);
	connect(clearAction, &QAction::triggered, this, &MainWindow::clear);

	saveAction = new QAction(QIcon::fromTheme("document-save"), tr("&Save"), this);
	saveAction->setShortcut(QKeySequence::Save);
	connect(saveAction, &QAction::triggered, this, &MainWindow::save);

	saveAsAction = new QAction(QIcon::fromTheme("document-save-as"), tr("Save &as…"), this);
	saveAsAction->setShortcut(QKeySequence::SaveAs);
	connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveAs);

	QAction *exitAction = new QAction(QIcon::fromTheme("application-exit"), tr("E&xit"), this);
	exitAction->setShortcut(QKeySequence::Quit);
	connect(exitAction, &QAction::triggered, this, &QWidget::close);

	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(openAction);
	fileMenu->addAction(addAction);
	fileMenu->addAction(removeAction);
	fileMenu->addAction(clearAction);
	fileMenu->addSeparator();
	fileMenu->addAction(saveAction);
	fileMenu->addAction(saveAsAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	QMenu *helpMenu = menuBar()->addMenu("&?");
	helpMenu->addAction(tr("&About…"), this, &MainWindow::about);
	helpMenu->addAction(tr("About &Qt…"), qApp, &QApplication::aboutQt);

	sortCombo = new QComboBox;
	sortCombo->addItem(tr("Path"), int(SortMode::Path));
	sortCombo->addItem(tr("File name"), int(SortMode::Name));
	const int sortIndex = sortCombo->findData(QSettings().value("sortMode", int(SortMode::Path)));
	sortCombo->setCurrentIndex(qMax(sortIndex, 0));
	connect(sortCombo, &QComboBox::currentIndexChanged, this, [this] {
		QSettings().setValue("sortMode", sortCombo->currentData());
		// the order is saved in the playlist
		if (!entries.isEmpty())
			setWindowModified(true);
		refreshView();
	});

	QLabel *sortLabel = new QLabel(tr("S&ort by:"));
	sortLabel->setBuddy(sortCombo);
	sortLabel->setContentsMargins(0, 0, 4, 0);

	QWidget *spacer = new QWidget;
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	// buttons that don't fit in a narrow window go into the toolbar extension menu
	toolBar = addToolBar(tr("Main toolbar"));
	toolBar->setMovable(false);
	toolBar->setToolButtonStyle(Qt::ToolButtonFollowStyle);
	toolBar->addAction(openAction);
	toolBar->addAction(saveAction);
	toolBar->addAction(saveAsAction);
	toolBar->addSeparator();
	toolBar->addAction(addAction);
	toolBar->addAction(removeAction);
	toolBar->addAction(clearAction);
	toolBar->addWidget(spacer);
	toolBar->addWidget(sortLabel);
	toolBar->addWidget(sortCombo);

	list = new QListWidget;
	list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	list->setUniformItemSizes(true);
	list->setFrameShape(QFrame::NoFrame);
	list->setAlternatingRowColors(true);
	connect(list, &QListWidget::itemSelectionChanged, this, &MainWindow::updateActions);
	setCentralWidget(list);

	countLabel = new QLabel;
	statusBar()->addPermanentWidget(countLabel);

	refreshView();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (maybeSave())
		event->accept();
	else
		event->ignore();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
	QMainWindow::resizeEvent(event);
	fitToolBar();
}

// with text beside icons, the less important buttons lose their text until the toolbar fits
void MainWindow::fitToolBar()
{
	const QList<QAction *> byImportance = { saveAsAction, clearAction, removeAction, addAction, saveAction, openAction };
	for (QAction *action : byImportance)
		action->setPriority(QAction::NormalPriority);
	for (QAction *action : byImportance) {
		if (toolBar->sizeHint().width() <= width())
			break;
		action->setPriority(QAction::LowPriority);
	}
}

void MainWindow::open()
{
	if (!maybeSave())
		return;

	const QString path = QFileDialog::getOpenFileName(this, tr("Open playlist"), lastDir(),
		tr("Playlists (%1);;All files (*)").arg(playlistPatterns.join(' ')));
	if (path.isEmpty())
		return;
	setLastDir(path);
	openPlaylist(path);
}

void MainWindow::addFiles()
{
	const QStringList files = QFileDialog::getOpenFileNames(this, tr("Add files"), lastDir(),
		tr("Audio files and playlists (%1);;All files (*)").arg((audioPatterns + playlistPatterns).join(' ')));
	if (files.isEmpty())
		return;
	setLastDir(files.first());

	QList<Entry> newEntries;
	for (const QString &file : files)
		collect(file, newEntries);
	merge(newEntries);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	const QList<QUrl> urls = event->mimeData()->urls();
	if (std::any_of(urls.cbegin(), urls.cend(), [](const QUrl &url) { return url.isLocalFile(); }))
		event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
	QStringList paths;
	for (const QUrl &url : event->mimeData()->urls())
		if (url.isLocalFile())
			paths.append(url.toLocalFile());
	event->acceptProposedAction();
	openFiles(paths);
}

// used for dropped files and command line arguments
void MainWindow::openFiles(const QStringList &paths)
{
	// a single playlist on an empty window is opened, anything else is merged
	if (paths.size() == 1 && isPlaylist(paths.first()) && entries.isEmpty() && currentFile.isEmpty()) {
		openPlaylist(paths.first());
		return;
	}

	QList<Entry> newEntries;
	for (const QString &path : paths)
		collect(path, newEntries);
	merge(newEntries);
}

void MainWindow::removeSelected()
{
	QSet<QString> selected;
	for (const QListWidgetItem *item : list->selectedItems())
		selected.insert(item->data(Qt::UserRole).toString());
	if (selected.isEmpty())
		return;

	entries.removeIf([&](const Entry &e) { return selected.contains(e.absPath); });
	setWindowModified(true);
	refreshView();
}

void MainWindow::clear()
{
	if (!maybeSave())
		return;

	entries.clear();
	setCurrentFile(QString());
	setWindowModified(false);
	refreshView();
}

bool MainWindow::save()
{
	if (currentFile.isEmpty())
		return saveAs();
	return writePlaylist(currentFile);
}

bool MainWindow::saveAs()
{
	QFileDialog dialog(this, tr("Save playlist"), currentFile.isEmpty() ? lastDir() : currentFile,
		tr("Playlists (%1)").arg(playlistPatterns.join(' ')));
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setDefaultSuffix("m3u");
	if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty())
		return false;

	const QString path = dialog.selectedFiles().first();
	setLastDir(path);
	if (!writePlaylist(path))
		return false;
	setCurrentFile(path);
	return true;
}

void MainWindow::about()
{
	//: %1 is the author name
	const QString author = tr("by %1").arg("Matteo Croce");
	QMessageBox::about(this, tr("About Meu"),
		QString("Meu - M3U Editor Utility<br>%1 <a href=\"https://matteocroce.it/\">https://matteocroce.it/</a>").arg(author));
}

void MainWindow::openPlaylist(const QString &path)
{
	QList<Entry> newEntries;
	if (!readPlaylist(path, newEntries))
		return;

	entries.clear();
	merge(newEntries);
	setCurrentFile(path);
	setWindowModified(false);
	updateActions();
}

bool MainWindow::readPlaylist(const QString &path, QList<Entry> &newEntries)
{
	QString error;
	if (!Playlist::readM3u(path, newEntries, &error)) {
		QMessageBox::warning(this, tr("Error"), tr("Cannot read %1:\n%2").arg(path, error));
		return false;
	}
	return true;
}

// playlists are read, directories are scanned recursively for audio files
void MainWindow::collect(const QString &path, QList<Entry> &newEntries)
{
	if (QFileInfo(path).isDir()) {
		QDirIterator it(path, audioPatterns, QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext())
			newEntries.append(Playlist::makeEntry(it.next()));
	} else if (isPlaylist(path)) {
		readPlaylist(path, newEntries);
	} else {
		newEntries.append(Playlist::makeEntry(path));
	}
}

bool MainWindow::writePlaylist(const QString &path)
{
	int skipped = 0;
	QString error;
	if (!Playlist::writeM3u(path, entries, &skipped, &error)) {
		QMessageBox::warning(this, tr("Error"), tr("Cannot save %1:\n%2").arg(path, error));
		return false;
	}

	setWindowModified(false);
	statusBar()->showMessage(tr("Playlist saved to %1").arg(path), 5000);
	if (skipped)
		QMessageBox::information(this, tr("Missing files"),
			tr("%n missing file(s) were not written to the playlist.", nullptr, skipped));

	// files may have disappeared or reappeared since they were added
	for (Entry &entry : entries)
		entry.missing = !QFileInfo::exists(entry.absPath);
	refreshView();
	return true;
}

bool MainWindow::maybeSave()
{
	if (!isWindowModified())
		return true;

	const QMessageBox::StandardButton answer = QMessageBox::warning(this, QApplication::applicationDisplayName(),
		tr("The playlist has been modified.\nDo you want to save your changes?"),
		QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
	if (answer == QMessageBox::Save)
		return save();
	return answer == QMessageBox::Discard;
}

void MainWindow::merge(const QList<Entry> &newEntries)
{
	const int added = Playlist::addEntries(entries, newEntries);
	const int duplicates = newEntries.size() - added;
	const int missing = std::count_if(newEntries.cbegin(), newEntries.cend(), [](const Entry &e) { return e.missing; });
	if (added)
		setWindowModified(true);
	refreshView();

	QStringList messages;
	if (missing)
		messages.append(tr("%n missing file(s) in the playlist", nullptr, missing));
	if (duplicates)
		messages.append(tr("Ignored %n file(s) already in the playlist", nullptr, duplicates));
	statusBar()->showMessage(messages.join(" - "), 5000);
}

void MainWindow::refreshView()
{
	Playlist::sortEntries(entries, SortMode(sortCombo->currentData().toInt()));

	// an empty list not tied to a file has nothing to lose
	if (entries.isEmpty() && currentFile.isEmpty())
		setWindowModified(false);

	const QIcon warning = style()->standardIcon(QStyle::SP_MessageBoxWarning);
	int missing = 0;

	list->clear();
	for (const Entry &entry : entries) {
		QListWidgetItem *item = new QListWidgetItem(QFileInfo(entry.absPath).fileName(), list);
		item->setData(Qt::UserRole, entry.absPath);
		if (entry.missing) {
			missing++;
			item->setIcon(warning);
			item->setForeground(Qt::red);
			item->setToolTip(tr("Missing file: %1").arg(entry.absPath));
		} else {
			item->setToolTip(entry.absPath);
		}
	}

	QString count = tr("%n song(s)", nullptr, entries.size());
	if (missing)
		count += ' ' + tr("(%n missing)", nullptr, missing);
	countLabel->setText(count);
	updateActions();
}

void MainWindow::updateActions()
{
	// an emptied playlist can still be saved over its file
	const bool canSave = !entries.isEmpty() || isWindowModified();
	removeAction->setEnabled(!list->selectedItems().isEmpty());
	clearAction->setEnabled(!entries.isEmpty() || !currentFile.isEmpty());
	saveAction->setEnabled(canSave);
	saveAsAction->setEnabled(canSave);
}

void MainWindow::setCurrentFile(const QString &path)
{
	currentFile = path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
	if (currentFile.isEmpty())
		setWindowTitle(QString("%1[*]").arg(QApplication::applicationDisplayName()));
	else
		setWindowTitle(QString("%1[*] - %2").arg(QFileInfo(currentFile).fileName(), QApplication::applicationDisplayName()));
}

QString MainWindow::lastDir() const
{
	return QSettings().value("lastDir", QDir::homePath()).toString();
}

void MainWindow::setLastDir(const QString &path)
{
	QSettings().setValue("lastDir", QFileInfo(path).absolutePath());
}
