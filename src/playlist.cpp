#include <QCollator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSet>
#include <QTextStream>
#include <QUrl>

#include <algorithm>

#include "playlist.h"

namespace Playlist {

Entry makeEntry(const QString &path)
{
	const QString absPath = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
	return { absPath, !QFileInfo::exists(absPath) };
}

bool readM3u(const QString &m3uPath, QList<Entry> &entries, QString *error)
{
	QFile file(m3uPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		if (error)
			*error = file.errorString();
		return false;
	}

	const QDir base = QFileInfo(m3uPath).absoluteDir();
	QTextStream in(&file);
	while (!in.atEnd()) {
		QString line = in.readLine().trimmed();
		if (line.isEmpty() || line.startsWith('#'))
			continue;
		if (line.startsWith("file:", Qt::CaseInsensitive))
			line = QUrl(line).toLocalFile();
		entries.append(makeEntry(base.absoluteFilePath(line)));
	}
	return true;
}

bool writeM3u(const QString &m3uPath, const QList<Entry> &entries, int *skipped, QString *error)
{
	QSaveFile file(m3uPath);
	if (!file.open(QIODevice::WriteOnly)) {
		if (error)
			*error = file.errorString();
		return false;
	}

	const QDir base = QFileInfo(m3uPath).absoluteDir();
	int missing = 0;
	QTextStream out(&file);
	// some players, like car stereos, recognize the playlist only with this header
	out << "#EXTM3U\n";
	for (const Entry &entry : entries) {
		if (!QFileInfo::exists(entry.absPath)) {
			missing++;
			continue;
		}
		out << base.relativeFilePath(entry.absPath) << '\n';
	}
	out.flush();

	if (skipped)
		*skipped = missing;
	if (!file.commit()) {
		if (error)
			*error = file.errorString();
		return false;
	}
	return true;
}

void sortEntries(QList<Entry> &entries, SortMode mode)
{
	// the C locale collation ignores numeric mode and case sensitivity
	QLocale locale;
	if (locale.language() == QLocale::C)
		locale = QLocale(QLocale::English, QLocale::UnitedStates);

	QCollator collator(locale);
	collator.setNumericMode(true);
	collator.setCaseSensitivity(Qt::CaseInsensitive);

	// compare paths one component at a time, so that a directory content stays together,
	// and put subdirectories before files like file managers do
	auto comparePaths = [&](const QString &a, const QString &b) {
		const QStringList pa = a.split('/');
		const QStringList pb = b.split('/');
		for (qsizetype i = 0; i < pa.size() && i < pb.size(); i++) {
			const bool dirA = i < pa.size() - 1;
			const bool dirB = i < pb.size() - 1;
			if (dirA != dirB)
				return dirA ? -1 : 1;
			const int c = collator.compare(pa[i], pb[i]);
			if (c)
				return c;
		}
		if (pa.size() != pb.size())
			return pa.size() < pb.size() ? -1 : 1;
		return a.compare(b);
	};

	std::stable_sort(entries.begin(), entries.end(), [&](const Entry &a, const Entry &b) {
		if (mode == SortMode::Name) {
			const int c = collator.compare(QFileInfo(a.absPath).fileName(), QFileInfo(b.absPath).fileName());
			if (c)
				return c < 0;
		}
		return comparePaths(a.absPath, b.absPath) < 0;
	});
}

int addEntries(QList<Entry> &entries, const QList<Entry> &newEntries)
{
	QSet<QString> known;
	for (const Entry &entry : entries)
		known.insert(entry.absPath);

	int added = 0;
	for (const Entry &entry : newEntries) {
		if (known.contains(entry.absPath))
			continue;
		known.insert(entry.absPath);
		entries.append(entry);
		added++;
	}
	return added;
}

}
