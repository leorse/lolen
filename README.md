# LoLEN — Lands of Lore ENgine

LoLEN runs **Lands of Lore: The Throne of Chaos** (Westwood Studios, 1993)
natively on Windows, without DOSBox, installer or launcher. It is a single
executable, `LandsOfLore.exe`, which starts the game directly.

LoLEN is a fork of [ScummVM](https://github.com/scummvm/scummvm), stripped
down to the **Kyra** engine, which runs Lands of Lore. Everything else from
ScummVM (launcher, other engines, other platforms) has been removed. The
engine keeps its original name and code; the infrastructure which replaces
ScummVM's is called LoLEN.

The original game always stays playable as is. Music and sound are fully
emulated (AdLib, optionally MT-32): no specific hardware is needed.

> **The game files are not included.** You need an original copy of Lands of
> Lore (GOG, CD or floppy version).

## Supported versions

**DOS** versions only:

| Version | Languages |
|---|---|
| CD (the GOG release is the CD version) | English, French, German (all on the same CD) |
| Floppy | English, French, German, Spanish… depending on the release |
| Fan translations (CD and floppy) | Russian, Italian, Chinese… |

The PC-98, FM-Towns, Amiga and Mac versions and the demos are not supported.

## Installing the game files

Put the game files **in the same directory as `LandsOfLore.exe`**:

```
LandsOfLore/
├── LandsOfLore.exe
├── (DLLs, see "Building")
├── GENERAL.PAK, L01.PAK, ... ← game files
├── lolen.ini                  ← created on first start
└── saves/                     ← created on first start
```

To keep the game files in another directory, set that directory in
`lolen.ini` (section `[game]`, key `path`) or pass it on the command line
(see below).

Depending on your version:

- **CD**: copy **all** the content of the CD, including the `ENG`, `FRE` and
  `GER` subdirectories (texts and voices of each language).
- **GOG**: the GOG installation directory contains a `GAME.DAT` file, which is
  an ISO image of the CD. Extract its content (for example with 7-Zip) and
  copy it as for the CD version. `GAME.DAT` itself cannot be used directly.
- **Floppy**: copy the content of all the disks (`WESTWOOD.1`, `WESTWOOD.2`…
  files) into the same directory, or the files of a copy already installed
  under DOS (`GENERAL.PAK`, `CHAPTER7.PAK`…).

### MT-32 music (optional)

The game uses AdLib emulation by default. For emulated Roland MT-32 music, put
the MT-32 ROMs (`MT32_CONTROL.ROM` and `MT32_PCM.ROM`) next to the game files,
then select the MT-32 in the **Sound** menu. The ROMs are not included.

## Starting the game

Double-click `LandsOfLore.exe`. The game starts directly.

For versions with several languages (CD, GOG), the Windows language is used
when it is available. It can be changed later in the LoLEN menu.

If the game files are not found, a message says so: check the directory or
the `path` key in `lolen.ini`.

### Command line

```
LandsOfLore.exe [OPTIONS] [GAME_PATH]

  GAME_PATH              Directory with the game data files
  --path=DIR             Same as GAME_PATH
  --language=CODE        Game language (en, fr, de, es, it, ru, zh...)
  -f, --fullscreen       Start in fullscreen mode
  -w, --windowed         Start in windowed mode
  --debugflags=FLAGS     Enable the given debug channels (comma separated, or 'all')
  -d, --debuglevel=NUM   Set the debug level
  -h, --help             Show this text
```

### Configuration: `lolen.ini`

`lolen.ini` is created next to the executable on first start. Every setting
is documented with a comment. A setting written as a comment (`;key=value`)
keeps its default value: remove the `;` to change it.

| Section | Content |
|---|---|
| `[game]` | game directory, language, savegame directory, game options |
| `[sound]` | music driver, volumes |
| `[display]` | fullscreen, aspect ratio correction, filtering… |
| `[controls]` | remapped keys and gamepad buttons (written by the program) |
| `[debug]` | debugging |
| `[internal]` | other settings written by the program |

Savegames are stored in the `saves` directory.

## Controls

The game is played with the mouse: left click to act, right click for
secondary actions (use an item, cast a spell…).

### LoLEN menu

| Keyboard | Gamepad | Action |
|---|---|---|
| F10 or Ctrl+F5 | Start | Open the LoLEN menu |

The menu pauses the game and offers:

- **Resume**: go back to the game;
- **Language**: change the game language (the game restarts, the progress
  since the last save is lost);
- **Sound**: volumes, music driver (AdLib, MT-32…);
- **Display**: fullscreen, aspect ratio correction, filtering, stretch mode;
- **Controls**: remap the keyboard keys and the gamepad buttons;
- **Credits**: the authors of LoLEN, of the Kyra engine and of ScummVM;
- **Quit**: leave the game.

### In the game

| Keyboard | Gamepad | Action |
|---|---|---|
| Up / Down arrow | D-pad up / down | Move forward / backwards |
| Left / Right arrow | Left / right trigger | Slide left / right |
| Home / Page Up | D-pad left / right | Turn left / right |
| F1, F2, F3 | X, Y, LB | Attack with the 1st, 2nd, 3rd character |
| M | | Map |
| R | | Rest |
| O | | Game options (save, load, audio…) |
| / | | Choose a spell |
| | A / B | Left click / right click |

### General shortcuts

| Keyboard | Action |
|---|---|
| Alt+Enter | Toggle fullscreen |
| Ctrl+U | Mute |
| Ctrl+M | Capture the mouse in the window |
| Alt+S | Screenshot |
| Ctrl+Q or Alt+F4 | Quit |

All these controls can be remapped in the **Controls** menu.

## Building

LoLEN is built on Windows with [MSYS2](https://www.msys2.org/), in the
**UCRT64** environment.

1. Install MSYS2, open the **MSYS2 UCRT64** shell and install the tools and
   libraries:

   ```sh
   pacman -S make diffutils \
     mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf \
     mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-libvorbis mingw-w64-ucrt-x86_64-zlib
   ```

2. Still in the UCRT64 shell, from the repository directory, configure and
   build:

   ```sh
   ./configure --disable-all-engines --enable-engine=kyra,lol \
     --disable-detection-full --enable-release --disable-debug \
     --disable-cloud --disable-libcurl --disable-sdlnet --disable-enet \
     --disable-discord --disable-tts --disable-taskbar --disable-sparkle \
     --disable-freetype2 --disable-fribidi --disable-jpeg --disable-gif \
     --disable-theoradec --disable-vpx --disable-faad --disable-bink \
     --disable-tinygl --disable-lua --disable-fluidsynth --disable-fluidlite \
     --disable-sonivox --disable-hq-scalers --disable-edge-scalers
   make -j8
   ```

   This produces `LandsOfLore.exe` at the root of the repository. The engine
   data (`kyra.dat`), the icon and the splash screen are embedded in the
   executable.

   Build from the MSYS2 UCRT64 shell: from Git Bash or another environment,
   linking fails.

3. The executable needs the MSYS2 DLLs. Either start it from the UCRT64
   shell:

   ```sh
   ./LandsOfLore.exe /path/to/the/game
   ```

   or copy these DLLs from `C:\msys64\ucrt64\bin` next to `LandsOfLore.exe`:
   `SDL2.dll`, `libvorbisfile-3.dll`, `libvorbis-0.dll`, `libogg-0.dll`,
   `zlib1.dll`, `libwinpthread-1.dll`.

The project roadmap is in [spec/roadmap-lol.md](spec/roadmap-lol.md).

## License and credits

LoLEN is distributed under the **GNU GPL version 3 or later**, like ScummVM
(see [COPYING](COPYING)). The ScummVM license headers are kept in every
source file.

Many thanks to the whole ScummVM team, and especially to the authors of the
Kyra engine, without whom this project would not exist. The full list of
authors is in [AUTHORS](AUTHORS) and in the **Credits** screen of the menu.

LoLEN is not an official ScummVM product. *Lands of Lore: The Throne of
Chaos* is a trademark of its respective owners (Westwood Studios / Electronic
Arts).
