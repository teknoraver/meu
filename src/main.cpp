#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	app.setOrganizationName("meu");
	app.setApplicationName("meu");
	app.setApplicationDisplayName("Meu");
	app.setApplicationVersion(MEU_VERSION);
	app.setDesktopFileName("meu");
	app.setWindowIcon(QIcon::fromTheme("meu", QIcon(":/meu.svg")));

	// Qt's own strings, e.g. standard dialog buttons
	QTranslator qtTranslator;
	if (qtTranslator.load(QLocale(), "qtbase", "_", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
		app.installTranslator(&qtTranslator);

	// sources are in English, but the English translation is still needed for plural forms
	QTranslator appTranslator;
	if (appTranslator.load(QLocale(), "meu", "_", ":/i18n") || appTranslator.load("meu_en", ":/i18n"))
		app.installTranslator(&appTranslator);

	QCommandLineParser parser;
	parser.setApplicationDescription(QCoreApplication::translate("main", "Create and edit M3U playlists."));
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addPositionalArgument(QCoreApplication::translate("main", "files"), QCoreApplication::translate("main",
		"Playlists, audio files or directories to open. More than one are merged into a new playlist."),
		QCoreApplication::translate("main", "[files...]"));
	parser.process(app);

	MainWindow window;
	window.resize(560, 640);
	window.show();
	if (!parser.positionalArguments().isEmpty())
		window.openFiles(parser.positionalArguments());
	return app.exec();
}
