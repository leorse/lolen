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

#ifndef KYRA_DETECTION_H
#define KYRA_DETECTION_H

#include "engines/advancedDetector.h"

namespace Kyra {

enum {
	// The value is stored in savegame headers, so it must not change.
	GI_LOL = 4
};

// Only the DOS floppy and DOS CD versions of Lands of Lore are supported.
struct GameFlags {
	Common::Language lang;

	// language overwrites of fan translations (only needed for multilingual games)
	Common::Language fanLang;
	Common::Language replacedLang;

	bool isTalkie             : 1;    // CD version
	bool useInstallerPackage  : 1;    // floppy version with compressed installer files

	byte gameID;
};

} // End of namespace Kyra

namespace {

struct KYRAGameDescription {
	AD_GAME_DESCRIPTION_HELPERS(desc);

	ADGameDescription desc;

	Kyra::GameFlags flags;
};

#define GAMEOPTION_LOL_SCROLLING		GUIO_GAMEOPTIONS4
#define GAMEOPTION_LOL_CURSORS			GUIO_GAMEOPTIONS5
#define GAMEOPTION_LOL_SAVENAMES		GUIO_GAMEOPTIONS8

} // End of anonymous namespace

#endif // KYRA_DETECTION_H
