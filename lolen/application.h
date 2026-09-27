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

#ifndef LOLEN_APPLICATION_H
#define LOLEN_APPLICATION_H

#include "common/error.h"
#include "common/path.h"
#include "common/str.h"

class OSystem;
class Plugin;
struct DetectedGame;

/**
 * LolEN - the Lands of Lore engine.
 *
 * This namespace holds the code which replaces the ScummVM infrastructure
 * (entry point, launcher, configuration handling...). The game engine itself
 * is the ScummVM Kyra engine and keeps its original names.
 */
namespace LolEN {

/**
 * Starts Lands of Lore directly, without any launcher.
 *
 * The configuration is stored in "lolen.ini" and the savegames in the "saves"
 * directory, both next to the executable. The game data path is taken from the
 * command line, then from the configuration file, then defaults to the
 * directory of the executable.
 */
class Application {
public:
	/**
	 * @param system   the backend, already pre-initialized with OSystem::init()
	 * @param baseDir  the directory holding the executable
	 */
	Application(OSystem &system, const Common::Path &baseDir);

	/** Runs the game and returns the process exit code. */
	int run(int argc, const char *const argv[]);

private:
	bool parseCommandLine(int argc, const char *const argv[]);
	void printUsage() const;

	void registerDefaults();
	void loadConfig();
	void setupGameDomain();

	void setupGraphics();
	void setupKeymapper();

	Common::Error identifyGame(const Plugin *&detectionPlugin, DetectedGame &game, const void *&descriptor);
	Common::Error runGame(const Plugin *enginePlugin, const DetectedGame &game, const void *descriptor);

	void showError(const Common::String &message);
	void shutdown();

	OSystem &_system;
	const Common::Path _baseDir;

	// Command line settings
	Common::Path _cmdGamePath;
	Common::String _cmdLanguage;
	Common::String _debugFlags;
	int _debugLevel;
	bool _cmdFullscreen;
	bool _cmdWindowed;
};

} // End of namespace LolEN

#endif
