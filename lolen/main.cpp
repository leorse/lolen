/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// Entry point of LolEN on Windows. It replaces the ScummVM entry point
// (backends/platform/sdl/win32/win32-main.cpp) and starts the game directly.

// Disable symbol overrides so that we can use system headers.
#define FORBIDDEN_SYMBOL_ALLOW_ALL

// LolEN provides its own main(), SDL must not rename it to SDL_main.
#define SDL_MAIN_HANDLED

#include "common/scummsys.h"

#ifdef WIN32

// Keep this after "common/scummsys.h", otherwise the windows headers
// redefine "ARRAYSIZE" for example.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "backends/platform/sdl/win32/win32.h"
#include "backends/platform/sdl/win32/win32_wrapper.h"
#include "backends/platform/sdl/sdl-sys.h"

#include "lolen/application.h"

int main(int argc, char *argv[]);

int __stdcall WinMain(HINSTANCE /*hInst*/, HINSTANCE /*hPrevInst*/,  LPSTR /*lpCmdLine*/, int /*iShowCmd*/) {
// MinGW-w64 based toolchains define __MINGW64_VERSION_MAJOR and provide __argc/__argv,
// the old mingwrt toolchains only provide _argc/_argv.
#if defined(__GNUC__) && defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
	return main(_argc, _argv);
#else
	return main(__argc, __argv);
#endif
}

int main(int argc, char *argv[]) {
	SDL_SetMainReady();

#ifdef UNICODE
	argv = Win32::getArgvUtf8(&argc);
#endif

	// Directory of the executable, which holds lolen.ini and the savegames
	TCHAR processDirectory[MAX_PATH];
	Win32::getProcessDirectory(processDirectory, MAX_PATH);
	const Common::Path baseDir(Win32::tcharToString(processDirectory), Common::Path::kNativeSeparator);

	// Create and pre-initialize the backend
	g_system = new OSystem_Win32();
	assert(g_system);
	g_system->init();

	int res = LolEN::Application(*g_system, baseDir).run(argc, argv);

	g_system->destroy();

#ifdef UNICODE
	Win32::freeArgvUtf8(argc, argv);
#endif

	return res;
}

#endif
