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

#ifndef KYRA_DEBUGGER_H
#define KYRA_DEBUGGER_H

#include "gui/debugger.h"

namespace Kyra {

class KyraEngine_v1;

class Debugger : public ::GUI::Debugger {
public:
	Debugger(KyraEngine_v1 *vm);

	virtual void initialize();

protected:
	KyraEngine_v1 *_vm;

	bool cmdSetScreenDebug(int argc, const char **argv);
	bool cmdLoadPalette(int argc, const char **argv);
	bool cmdShowFacings(int argc, const char **argv);
	bool cmdGameSpeed(int argc, const char **argv);
	bool cmdListFlags(int argc, const char **argv);
	bool cmdToggleFlag(int argc, const char **argv);
	bool cmdQueryFlag(int argc, const char **argv);
	bool cmdListTimers(int argc, const char **argv);
	bool cmdSetTimerCountdown(int argc, const char **argv);
};

class LoLEngine;

class Debugger_LoL : public Debugger {
public:
	Debugger_LoL(LoLEngine *vm);

protected:
	LoLEngine *_vm;
};

} // End of namespace Kyra

#endif
