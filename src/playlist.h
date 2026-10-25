#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QList>
#include <QString>

namespace Playlist {

struct Entry {
	QString absPath;
	bool missing;
};

enum class SortMode {
	Path,
	Name,
};

// Builds an entry from a (possibly relative) path, checking if the file exists
Entry makeEntry(const QString &path);

// Reads a .m3u file; relative paths are resolved against the playlist directory
bool readM3u(const QString &m3uPath, QList<Entry> &entries, QString *error = nullptr);

// Writes a .m3u file with paths relative to its directory, skipping missing files
bool writeM3u(const QString &m3uPath, const QList<Entry> &entries, int *skipped = nullptr, QString *error = nullptr);

void sortEntries(QList<Entry> &entries, SortMode mode);

// Appends the entries not already in the list, returns how many were added
int addEntries(QList<Entry> &entries, const QList<Entry> &newEntries);

}

#endif
