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

#ifndef LOLEN_SPLASH_H
#define LOLEN_SPLASH_H

namespace LolEN {

/**
 * Shows the LolEN splash screen.
 *
 * The image is the "lolen_splash.bmp" resource embedded in the executable
 * (see dists/scummvm.rc). It is scaled to fit the window, keeping its aspect
 * ratio, and shown for a short time or until a key or mouse button is pressed.
 * Nothing happens if the image is not available.
 */
void showSplashScreen();

} // End of namespace LolEN

#endif
