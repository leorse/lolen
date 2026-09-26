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

#include "kyra/gui/debugger.h"
#include "kyra/engine/timer.h"
#include "kyra/resource/resource.h"
#include "kyra/engine/lol.h"

#include "common/system.h"
#include "common/config-manager.h"

namespace Kyra {

Debugger::Debugger(KyraEngine_v1 *vm)
	: ::GUI::Debugger(), _vm(vm) {
}

void Debugger::initialize() {
	registerCmd("continue",           WRAP_METHOD(Debugger, cmdExit));
	registerCmd("screen_debug_mode",  WRAP_METHOD(Debugger, cmdSetScreenDebug));
	registerCmd("load_palette",       WRAP_METHOD(Debugger, cmdLoadPalette));
	registerCmd("facings",            WRAP_METHOD(Debugger, cmdShowFacings));
	registerCmd("gamespeed",          WRAP_METHOD(Debugger, cmdGameSpeed));
	registerCmd("flags",              WRAP_METHOD(Debugger, cmdListFlags));
	registerCmd("toggleflag",         WRAP_METHOD(Debugger, cmdToggleFlag));
	registerCmd("queryflag",          WRAP_METHOD(Debugger, cmdQueryFlag));
	registerCmd("timers",             WRAP_METHOD(Debugger, cmdListTimers));
	registerCmd("settimercountdown",  WRAP_METHOD(Debugger, cmdSetTimerCountdown));
}

bool Debugger::cmdSetScreenDebug(int argc, const char **argv) {
	if (argc > 1) {
		if (scumm_stricmp(argv[1], "enable") == 0)
			_vm->screen()->enableScreenDebug(true);
		else if (scumm_stricmp(argv[1], "disable") == 0)
			_vm->screen()->enableScreenDebug(false);
		else
			debugPrintf("Use screen_debug_mode <enable/disable> to enable or disable it.\n");
	} else {
		debugPrintf("Screen debug mode is %s.\n", (_vm->screen()->queryScreenDebug() ? "enabled" : "disabled"));
		debugPrintf("Use screen_debug_mode <enable/disable> to enable or disable it.\n");
	}
	return true;
}

bool Debugger::cmdLoadPalette(int argc, const char **argv) {
	Palette palette(_vm->screen()->getPalette(0).getNumColors());

	if (argc <= 1) {
		debugPrintf("Use load_palette <file> [start_col] [end_col]\n");
		return true;
	}

	if (_vm->resource()->getFileSize(argv[1]) != 768) {
		uint8 *buffer = new uint8[320 * 200 * sizeof(uint8)];
		if (!buffer) {
			debugPrintf("ERROR: Cannot allocate buffer for screen region!\n");
			return true;
		}

		_vm->screen()->copyRegionToBuffer(5, 0, 0, 320, 200, buffer);
		_vm->screen()->loadBitmap(argv[1], 5, 5, nullptr);
		palette.copy(_vm->screen()->getCPagePtr(5), 0, 256);
		_vm->screen()->copyBlockToPage(5, 0, 0, 320, 200, buffer);

		delete[] buffer;
	} else if (!_vm->screen()->loadPalette(argv[1], palette)) {
		debugPrintf("ERROR: Palette '%s' not found!\n", argv[1]);
		return true;
	}

	int startCol = 0;
	int endCol = palette.getNumColors();
	if (argc > 2)
		startCol = MIN(palette.getNumColors(), MAX(0, atoi(argv[2])));
	if (argc > 3)
		endCol = MIN(palette.getNumColors(), MAX(0, atoi(argv[3])));

	if (startCol > 0)
		palette.copy(_vm->screen()->getPalette(0), 0, startCol);
	if (endCol < palette.getNumColors())
		palette.copy(_vm->screen()->getPalette(0), endCol);

	_vm->screen()->setScreenPalette(palette);
	_vm->screen()->updateScreen();

	return true;
}

bool Debugger::cmdShowFacings(int argc, const char **argv) {
	debugPrintf("Facing directions:\n");
	debugPrintf("7  0  1\n");
	debugPrintf(" \\ | / \n");
	debugPrintf("6--*--2\n");
	debugPrintf(" / | \\\n");
	debugPrintf("5  4  3\n");
	return true;
}

bool Debugger::cmdGameSpeed(int argc, const char **argv) {
	if (argc == 2) {
		int val = atoi(argv[1]);

		if (val < 1 || val > 1000) {
			debugPrintf("speed must lie between 1 and 1000 (default: 60)\n");
			return true;
		}

		_vm->_tickLength = (uint8)(1000.0 / val);
	} else {
		debugPrintf("Syntax: gamespeed <value>\n");
	}

	return true;
}

bool Debugger::cmdListFlags(int argc, const char **argv) {
	for (int i = 0, p = 0; i < (int)sizeof(_vm->_flagsTable) * 8; i++, ++p) {
		debugPrintf("(%-3i): %-2i", i, _vm->queryGameFlag(i));
		if (p == 5) {
			debugPrintf("\n");
			p -= 6;
		}
	}
	debugPrintf("\n");
	return true;
}

bool Debugger::cmdToggleFlag(int argc, const char **argv) {
	if (argc > 1) {
		uint flag = atoi(argv[1]);
		if (_vm->queryGameFlag(flag))
			_vm->resetGameFlag(flag);
		else
			_vm->setGameFlag(flag);
		debugPrintf("Flag %i is now %i\n", flag, _vm->queryGameFlag(flag));
	} else {
		debugPrintf("Syntax: toggleflag <flag>\n");
	}

	return true;
}

bool Debugger::cmdQueryFlag(int argc, const char **argv) {
	if (argc > 1) {
		uint flag = atoi(argv[1]);
		debugPrintf("Flag %i is %i\n", flag, _vm->queryGameFlag(flag));
	} else {
		debugPrintf("Syntax: queryflag <flag>\n");
	}

	return true;
}

bool Debugger::cmdListTimers(int argc, const char **argv) {
	debugPrintf("Current time: %-8u\n", g_system->getMillis());
	for (int i = 0; i < _vm->timer()->count(); i++)
		debugPrintf("Timer %-2i: Active: %-3s Countdown: %-6i %-8u\n", i, _vm->timer()->isEnabled(i) ? "Yes" : "No", _vm->timer()->getDelay(i), _vm->timer()->getNextRun(i));

	return true;
}

bool Debugger::cmdSetTimerCountdown(int argc, const char **argv) {
	if (argc > 2) {
		uint timer = atoi(argv[1]);
		uint countdown = atoi(argv[2]);
		_vm->timer()->setCountdown(timer, countdown);
		debugPrintf("Timer %i now has countdown %i\n", timer, _vm->timer()->getDelay(timer));
	} else {
		debugPrintf("Syntax: settimercountdown <timer> <countdown>\n");
	}

	return true;
}

#pragma mark -

Debugger_LoL::Debugger_LoL(LoLEngine *vm) : Debugger(vm), _vm(vm) {
}

} // End of namespace Kyra
