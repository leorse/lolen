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

#ifndef LOLEN_CONFIG_H
#define LOLEN_CONFIG_H

#include "common/path.h"
#include "common/str.h"

namespace LoLEN {

/**
 * The LoLEN configuration file (lolen.ini).
 *
 * The file is organized in sections:
 *   [game]      game data path, language, savegame directory, game options
 *   [sound]     music driver and volumes
 *   [display]   fullscreen, aspect ratio, filtering...
 *   [controls]  remapped keys and gamepad buttons (keymapper)
 *   [debug]     debug level and channels
 *   [internal]  any other setting stored by the engine or the backend
 *
 * The engine and the backend still use the ScummVM ConfigManager (ConfMan).
 * load() fills ConfMan from the file and save() writes the ConfMan values back
 * into the file. Once install() has been called, ConfMan.flushToDisk() saves
 * the LoLEN file instead of a ScummVM configuration file.
 *
 * The old format written by ScummVM ([scummvm] and game sections) is still read,
 * the file is converted to the LoLEN format the next time it is saved.
 */
class Config {
public:
	/**
	 * @param filename    path of lolen.ini
	 * @param gameDomain  name of the ConfMan game domain which holds the game settings
	 */
	Config(const Common::Path &filename, const Common::String &gameDomain);
	~Config();

	/** Reads the file into ConfMan. Returns false if the file does not exist yet. */
	bool load();

	/** Writes the ConfMan settings into the file. */
	bool save() const;

	/** Makes ConfMan.flushToDisk() call save(). */
	void install();

	const Common::Path &getFilename() const { return _filename; }

private:
	static void flushHandler();

	const Common::Path _filename;
	const Common::String _gameDomain;
};

} // End of namespace LoLEN

#endif
