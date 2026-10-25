cmake_minimum_required(VERSION 3.16)

project(meu VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test OPTIONAL_COMPONENTS LinguistTools)
qt_standard_project_setup()

add_library(meu_core STATIC src/playlist.cpp src/playlist.h)
target_include_directories(meu_core PUBLIC src)
target_link_libraries(meu_core PUBLIC Qt6::Core)

qt_add_executable(meu WIN32 MACOSX_BUNDLE
	src/main.cpp
	src/mainwindow.cpp
	src/mainwindow.h
)
target_link_libraries(meu PRIVATE meu_core Qt6::Widgets)
target_compile_definitions(meu PRIVATE MEU_VERSION="${PROJECT_VERSION}")

# window icon fallback, used when the icon is not installed in the theme
qt_add_resources(meu "icon" PREFIX "/" BASE data FILES data/meu.svg)

# the .qm files are embedded in the executable under :/i18n
# run "cmake --build build --target update_translations" to refresh the .ts files
if(Qt6LinguistTools_FOUND)
	qt_add_translations(meu
		TS_FILES translations/meu_it.ts
		PLURALS_TS_FILE translations/meu_en.ts
		LUPDATE_OPTIONS -locations none
	)
else()
	message(WARNING "Qt6 LinguistTools not found, building without translations")
endif()

include(GNUInstallDirs)
install(TARGETS meu
	BUNDLE DESTINATION .
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(UNIX AND NOT APPLE)
	install(FILES data/meu.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
	install(FILES data/meu.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()

enable_testing()
qt_add_executable(tst_playlist tests/tst_playlist.cpp)
target_link_libraries(tst_playlist PRIVATE meu_core Qt6::Test)
add_test(NAME tst_playlist COMMAND tst_playlist)
