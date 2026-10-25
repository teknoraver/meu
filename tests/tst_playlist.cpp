#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include "playlist.h"

using Playlist::Entry;
using Playlist::SortMode;

class TestPlaylist : public QObject
{
	Q_OBJECT
private slots:
	void init();
	void read();
	void readMissingFile();
	void write();
	void writeSameDir();
	void roundTrip();
	void sortByName();
	void sortByPath();
	void dedup();

private:
	QString path(const QString &rel) const { return dir->filePath(rel); }
	void touch(const QString &rel);
	void writeText(const QString &rel, const QByteArray &data);
	QByteArray readText(const QString &rel);

	std::unique_ptr<QTemporaryDir> dir;
};

static QStringList paths(const QList<Entry> &entries)
{
	QStringList result;
	for (const Entry &entry : entries)
		result.append(entry.absPath);
	return result;
}

static QList<Entry> fakeEntries(const QStringList &paths)
{
	QList<Entry> result;
	for (const QString &path : paths)
		result.append({ path, false });
	return result;
}

void TestPlaylist::init()
{
	dir = std::make_unique<QTemporaryDir>();
	QVERIFY(dir->isValid());
	touch("music/a.mp3");
	touch("music/sub/b.mp3");
	touch("other/c.mp3");
	QVERIFY(QDir().mkpath(path("lists")));
}

void TestPlaylist::touch(const QString &rel)
{
	writeText(rel, QByteArray());
}

void TestPlaylist::writeText(const QString &rel, const QByteArray &data)
{
	QVERIFY(QDir().mkpath(QFileInfo(path(rel)).absolutePath()));
	QFile file(path(rel));
	QVERIFY(file.open(QIODevice::WriteOnly));
	file.write(data);
}

QByteArray TestPlaylist::readText(const QString &rel)
{
	QFile file(path(rel));
	if (!file.open(QIODevice::ReadOnly))
		return QByteArray();
	return file.readAll();
}

void TestPlaylist::read()
{
	const QByteArray m3u = "#EXTM3U\n"
		"\n"
		"#EXTINF:123,Artist - Title\n"
		"../music/a.mp3\r\n"
		+ path("other/c.mp3").toUtf8() + "\n"
		+ QUrl::fromLocalFile(path("music/sub/b.mp3")).toEncoded() + "\n"
		"  ../music/missing.mp3  \n";
	writeText("lists/p.m3u", m3u);

	QList<Entry> entries;
	QVERIFY(Playlist::readM3u(path("lists/p.m3u"), entries));
	QCOMPARE(paths(entries), QStringList({
		path("music/a.mp3"),
		path("other/c.mp3"),
		path("music/sub/b.mp3"),
		path("music/missing.mp3"),
	}));
	QCOMPARE(entries[0].missing, false);
	QCOMPARE(entries[1].missing, false);
	QCOMPARE(entries[2].missing, false);
	QCOMPARE(entries[3].missing, true);
}

void TestPlaylist::readMissingFile()
{
	QList<Entry> entries;
	QString error;
	QVERIFY(!Playlist::readM3u(path("lists/nope.m3u"), entries, &error));
	QVERIFY(!error.isEmpty());
}

void TestPlaylist::write()
{
	const QList<Entry> entries = {
		Playlist::makeEntry(path("music/a.mp3")),
		Playlist::makeEntry(path("music/sub/b.mp3")),
		Playlist::makeEntry(path("music/gone.mp3")),
		Playlist::makeEntry(path("other/c.mp3")),
	};
	int skipped = -1;
	QVERIFY(Playlist::writeM3u(path("lists/out.m3u"), entries, &skipped));
	QCOMPARE(skipped, 1);
	QCOMPARE(readText("lists/out.m3u"), QByteArray("#EXTM3U\n../music/a.mp3\n../music/sub/b.mp3\n../other/c.mp3\n"));
}

void TestPlaylist::writeSameDir()
{
	const QList<Entry> entries = {
		Playlist::makeEntry(path("music/a.mp3")),
		Playlist::makeEntry(path("music/sub/b.mp3")),
		Playlist::makeEntry(path("other/c.mp3")),
	};
	QVERIFY(Playlist::writeM3u(path("music/out.m3u"), entries));
	QCOMPARE(readText("music/out.m3u"), QByteArray("#EXTM3U\na.mp3\nsub/b.mp3\n../other/c.mp3\n"));
}

void TestPlaylist::roundTrip()
{
	const QList<Entry> entries = {
		Playlist::makeEntry(path("music/a.mp3")),
		Playlist::makeEntry(path("other/c.mp3")),
	};
	QVERIFY(Playlist::writeM3u(path("lists/rt.m3u"), entries));

	QList<Entry> read;
	QVERIFY(Playlist::readM3u(path("lists/rt.m3u"), read));
	QCOMPARE(paths(read), paths(entries));
}

void TestPlaylist::sortByName()
{
	QList<Entry> entries = fakeEntries({
		"/z/track10.mp3",
		"/a/Track2.mp3",
		"/m/track1.mp3",
		"/b/track2.mp3",
	});
	Playlist::sortEntries(entries, SortMode::Name);
	QCOMPARE(paths(entries), QStringList({
		"/m/track1.mp3",
		"/a/Track2.mp3",
		"/b/track2.mp3",
		"/z/track10.mp3",
	}));
}

void TestPlaylist::sortByPath()
{
	QList<Entry> entries = fakeEntries({
		"/x/0.mp3",
		"/x/b/1.mp3",
		"/x/a b/2.mp3",
		"/x/a/10.mp3",
		"/x/A/9.mp3",
		"/x/a/sub/1.mp3",
	});
	Playlist::sortEntries(entries, SortMode::Path);
	QCOMPARE(paths(entries), QStringList({
		"/x/a/sub/1.mp3",
		"/x/A/9.mp3",
		"/x/a/10.mp3",
		"/x/a b/2.mp3",
		"/x/b/1.mp3",
		"/x/0.mp3",
	}));
}

void TestPlaylist::dedup()
{
	QList<Entry> entries = fakeEntries({ "/a.mp3", "/b.mp3" });
	QCOMPARE(Playlist::addEntries(entries, fakeEntries({ "/b.mp3", "/c.mp3", "/c.mp3" })), 1);
	QCOMPARE(paths(entries), QStringList({ "/a.mp3", "/b.mp3", "/c.mp3" }));
}

QTEST_GUILESS_MAIN(TestPlaylist)
#include "tst_playlist.moc"
