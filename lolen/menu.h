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

#ifndef LOLEN_MENU_H
#define LOLEN_MENU_H

#include "common/array.h"
#include "common/language.h"

namespace LoLEN {

/**
 * Installs the LoLEN in-game menu in place of the ScummVM main menu.
 *
 * The menu is opened with Ctrl+F5, F10 or the Start button of a gamepad
 * (global keymap "MENU", which can be remapped in the menu). It offers:
 * Resume, Language, Sound, Display, Controls, Credits and Quit.
 *
 * @param languages  languages available for the installed version of the game
 */
void installMenu(const Common::Array<Common::Language> &languages);

/** Restores the ScummVM main menu. */
void uninstallMenu();

/**
 * Returns true when the player asked in the menu to restart the game
 * (new language or new music driver), and clears the request.
 */
bool takeRestartRequest();

} // End of namespace LoLEN

#endif
