# Meu

**M3U Editor Utility**: a simple Qt application to create and edit `.m3u` playlists.

"Meu" means "mine" in Calabrian dialect, and it's how M3U reads with the 3 as an E.

<p align="center"><img src="https://github.com/user-attachments/assets/2f6f1e67-46db-4c57-9b24-cf9d9ff3a263" alt="Meu main window" width="576"></p>

## Features

- Add audio files with a file dialog or by dragging files and directories onto the window
  (directories are scanned recursively)
- Sort the playlist by path (subdirectories first) or by file name
  (case insensitive, natural order: `2` before `10`)
- Remove one or more songs, duplicates are ignored
- Open an existing `.m3u`/`.m3u8` playlist: missing files are shown in red with a warning icon
- Merge playlists by adding a playlist file to the open one
- Open files from the command line or with "Open with" in the file manager:
  a single playlist is opened, more files are merged into a new playlist
- Save with paths relative to the playlist directory, missing files are not written
- Warns about unsaved changes
- English and Italian user interface, chosen from the system locale

## Building

Requirements: CMake 3.16 or later, a C++17 compiler and Qt 6 (Widgets, Test).  
Qt Linguist tools are optional: without them the translations are not built
and the interface is English only.

On Fedora:

```sh
sudo dnf install cmake gcc-c++ qt6-qtbase-devel qt6-qttools-devel qt6-linguist qt6-qtsvg
```

`qt6-qtsvg` is only needed at runtime, to show the SVG application icon.

Build, test and run:

```sh
cmake -B build -S .
cmake --build build -j
ctest --test-dir build
./build/meu
```

To force a language, set the locale:

```sh
LANG=it_IT.UTF-8 ./build/meu
```

To install it with its icon and menu entry, for example in your home directory:

```sh
cmake --install build --prefix ~/.local
```

## Translations

User interface strings are written in English in the sources. The Italian
translation is in `translations/meu_it.ts`, while `translations/meu_en.ts`
only holds the English plural forms.  
After changing strings in the sources, refresh the `.ts` files with:

```sh
cmake --build build --target update_translations
```

then edit them with Qt Linguist or by hand.

## Playlist format

Playlists are plain `.m3u` files: an `#EXTM3U` header, then one path per line.  
When reading, empty lines and lines starting with `#` (like `#EXTM3U` and `#EXTINF`)
are ignored, and `file://` URLs are accepted.  
When saving, paths are written relative to the directory of the playlist:
it's up to the user to keep the files reachable from there.

## License

Copyright (C) 2026 Matteo Croce

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.  
See [LICENSE](LICENSE) for the full text.
