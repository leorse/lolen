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

#include "lolen/config.h"

#include "common/algorithm.h"
#include "common/config-manager.h"
#include "common/file.h"
#include "common/fs.h"
#include "common/str-array.h"
#include "common/textconsole.h"

namespace LoLEN {

namespace {

// Where the settings of a section are stored in ConfMan
enum SectionDomain {
	kDomainGame,        // game domain (the active domain while the game runs)
	kDomainApplication, // ScummVM application domain
	kDomainKeymapper    // ScummVM keymapper domain
};

struct Section {
	const char *name;
	SectionDomain domain;
	const char *comment;
};

// Sections in the order they are written
static const Section kSections[] = {
	{ "game",     kDomainGame,        "Game data and game options" },
	{ "sound",    kDomainApplication, "Music and sound" },
	{ "display",  kDomainApplication, "Display" },
	{ "controls", kDomainKeymapper,   "Remapped keys and gamepad buttons (written by the program)" },
	{ "debug",    kDomainApplication, "Debugging" },
	{ "internal", kDomainApplication, "Other settings written by the program" }
};

struct Key {
	const char *section;
	const char *name;
	const char *comment;
};

// Documented settings. Any other setting is kept in [internal] (or in [game]
// for the settings stored in the game domain).
static const Key kKeys[] = {
	{ "game", "path",             "Directory with the game data files" },
	{ "game", "language",         "Game language: en, fr, de, es, it, ru, zh (the CD version has en, fr and de)" },
	{ "game", "savepath",         "Directory for the savegames" },
	{ "game", "subtitles",        "Show the text of the spoken dialogues: true or false" },
	{ "game", "smooth_scrolling", "Smooth scrolling when walking: true or false" },
	{ "game", "floating_cursors", "Directional arrow cursors at the screen edges: true or false" },
	{ "game", "auto_savenames",   "Suggest savegame names: true or false" },

	{ "sound", "music_driver",    "Music: auto, adlib, mt32 (emulated, needs the MT-32 ROM files), pcspk or a MIDI device" },
	{ "sound", "native_mt32",     "The MIDI device is a real MT-32 or compatible: true or false" },
	{ "sound", "enable_gs",       "Roland GS mode for MT-32 music on GS devices: true or false" },
	{ "sound", "multi_midi",      "Mix AdLib and MIDI music: true or false" },
	{ "sound", "music_volume",    "Music volume: 0 to 256" },
	{ "sound", "sfx_volume",      "Sound effects volume: 0 to 256" },
	{ "sound", "speech_volume",   "Speech volume: 0 to 256" },
	{ "sound", "mute",            "Mute all sounds: true or false" },

	{ "display", "fullscreen",    "Fullscreen mode: true or false (Alt+Enter while playing)" },
	{ "display", "aspect_ratio",  "Correct the aspect ratio to 4:3: true or false" },
	{ "display", "filtering",     "Bilinear filtering: true or false" },
	{ "display", "vsync",         "Vertical synchronization: true or false" },
	{ "display", "gfx_mode",      "Graphics mode: default, opengl or surfacesdl" },
	{ "display", "stretch_mode",  "Stretch mode: default, center, pixel-perfect, even-pixels, fit, stretch or fit_force_aspect" },

	{ "debug", "debuglevel",      "Debug level for the console output (0 = off)" },
	{ "debug", "debugflags",      "Comma separated debug channels, or all" }
};

// Settings which are set by the program at each start and are not stored
static const char *const kVolatileGameKeys[] = { "gameid", "engineid", "guioptions", "description" };

const Section *findSection(const Common::String &name) {
	for (const Section &section : kSections)
		if (name.equalsIgnoreCase(section.name))
			return &section;
	return nullptr;
}

const Key *findKey(const Common::String &name) {
	for (const Key &key : kKeys)
		if (name.equalsIgnoreCase(key.name))
			return &key;
	return nullptr;
}

bool isVolatileGameKey(const Common::String &name) {
	for (const char *key : kVolatileGameKeys)
		if (name.equalsIgnoreCase(key))
			return true;
	return false;
}

Common::StringArray sortedKeys(const Common::ConfigManager::Domain &domain) {
	Common::StringArray keys;
	for (const auto &entry : domain)
		keys.push_back(entry._key);
	Common::sort(keys.begin(), keys.end());
	return keys;
}

void writeComment(Common::WriteStream &stream, const char *comment) {
	stream.writeString(Common::String::format("; %s\n", comment));
}

void writeEntry(Common::WriteStream &stream, const Common::String &key, const Common::String &value) {
	stream.writeString(Common::String::format("%s=%s\n", key.c_str(), value.c_str()));
}

Config *s_installedConfig = nullptr;

} // End of anonymous namespace

Config::Config(const Common::Path &filename, const Common::String &gameDomain)
	: _filename(filename), _gameDomain(gameDomain) {
}

Config::~Config() {
	if (s_installedConfig == this) {
		ConfMan.setFlushHandler(nullptr);
		s_installedConfig = nullptr;
	}
}

bool Config::load() {
	if (!ConfMan.hasGameDomain(_gameDomain))
		ConfMan.addGameDomain(_gameDomain);

	Common::File file;
	if (!file.open(Common::FSNode(_filename)))
		return false;

	Common::String domain = Common::ConfigManager::kApplicationDomain;
	int lineNumber = 0;

	while (!file.eos() && !file.err()) {
		Common::String line = file.readLine();
		++lineNumber;

		// UTF-8 byte order mark written by some editors
		if (lineNumber == 1 && line.hasPrefix("\xEF\xBB\xBF"))
			line.erase(0, 3);

		line.trim();
		if (line.empty() || line.hasPrefix(";") || line.hasPrefix("#"))
			continue;

		if (line.hasPrefix("[")) {
			if (!line.hasSuffix("]")) {
				warning("%s:%d: invalid section '%s'", _filename.toString(Common::Path::kNativeSeparator).c_str(), lineNumber, line.c_str());
				continue;
			}
			const Common::String name = line.substr(1, line.size() - 2);
			const Section *section = findSection(name);

			if (section) {
				switch (section->domain) {
				case kDomainGame:
					domain = _gameDomain;
					break;
				case kDomainKeymapper:
					domain = Common::ConfigManager::kKeymapperDomain;
					break;
				default:
					domain = Common::ConfigManager::kApplicationDomain;
					break;
				}
			} else if (name.equalsIgnoreCase(_gameDomain)) {
				// Old format written by ScummVM: the game domain
				domain = _gameDomain;
			} else if (name.equalsIgnoreCase(Common::ConfigManager::kKeymapperDomain)) {
				domain = Common::ConfigManager::kKeymapperDomain;
			} else {
				// Old format written by ScummVM ([scummvm]) or unknown section
				if (!name.equalsIgnoreCase(Common::ConfigManager::kApplicationDomain))
					warning("%s:%d: unknown section '%s'", _filename.toString(Common::Path::kNativeSeparator).c_str(), lineNumber, name.c_str());
				domain = Common::ConfigManager::kApplicationDomain;
			}
			continue;
		}

		const size_t separator = line.findFirstOf('=');
		if (separator == Common::String::npos) {
			warning("%s:%d: invalid line '%s'", _filename.toString(Common::Path::kNativeSeparator).c_str(), lineNumber, line.c_str());
			continue;
		}

		Common::String key = line.substr(0, separator);
		Common::String value = line.substr(separator + 1);
		key.trim();
		value.trim();
		if (!key.empty())
			ConfMan.getDomain(domain)->setVal(key, value);
	}

	return true;
}

bool Config::save() const {
	Common::DumpFile file;
	if (!file.open(_filename)) {
		warning("Could not write the configuration file '%s'", _filename.toString(Common::Path::kNativeSeparator).c_str());
		return false;
	}

	const Common::ConfigManager::Domain *gameDomain = ConfMan.getDomain(_gameDomain);
	const Common::ConfigManager::Domain *appDomain = ConfMan.getDomain(Common::ConfigManager::kApplicationDomain);
	const Common::ConfigManager::Domain *keymapperDomain = ConfMan.getDomain(Common::ConfigManager::kKeymapperDomain);

	file.writeString("; LoLEN configuration file\n");
	file.writeString("; Lines starting with ';' are comments. A setting written as a comment uses its\n");
	file.writeString("; default value: remove the ';' to change it. The file is rewritten by the program.\n");

	for (const Section &section : kSections) {
		file.writeString("\n");
		writeComment(file, section.comment);
		file.writeString(Common::String::format("[%s]\n", section.name));

		if (section.domain == kDomainKeymapper) {
			if (keymapperDomain) {
				for (const Common::String &key : sortedKeys(*keymapperDomain))
					writeEntry(file, key, keymapperDomain->getVal(key));
			}
			continue;
		}

		// Documented settings of this section. The game domain overrides the
		// application domain, since the engine stores its changes there.
		for (const Key &key : kKeys) {
			if (strcmp(key.section, section.name))
				continue;

			writeComment(file, key.comment);
			if (gameDomain && gameDomain->contains(key.name))
				writeEntry(file, key.name, gameDomain->getVal(key.name));
			else if (appDomain && appDomain->contains(key.name))
				writeEntry(file, key.name, appDomain->getVal(key.name));
			else if (ConfMan.hasDefault(key.name))
				// Default values are shown as comments, so that the engine can still tell
				// whether a setting was chosen by the user.
				file.writeString(Common::String::format(";%s=%s\n", key.name, ConfMan.get(key.name, Common::ConfigManager::kApplicationDomain).c_str()));
			else
				file.writeString(Common::String::format(";%s=\n", key.name));
		}

		// Other settings of the game domain are kept in [game]
		if (section.domain == kDomainGame && gameDomain) {
			for (const Common::String &key : sortedKeys(*gameDomain)) {
				if (!findKey(key) && !isVolatileGameKey(key))
					writeEntry(file, key, gameDomain->getVal(key));
			}
		}

		// Other settings of the application domain are kept in [internal]
		if (!strcmp(section.name, "internal") && appDomain) {
			for (const Common::String &key : sortedKeys(*appDomain)) {
				if (!findKey(key))
					writeEntry(file, key, appDomain->getVal(key));
			}
		}
	}

	if (!file.flush() || file.err()) {
		warning("Error while writing the configuration file '%s'", _filename.toString(Common::Path::kNativeSeparator).c_str());
		return false;
	}

	return true;
}

void Config::install() {
	s_installedConfig = this;
	ConfMan.setFlushHandler(&Config::flushHandler);
}

void Config::flushHandler() {
	if (s_installedConfig)
		s_installedConfig->save();
}

} // End of namespace LoLEN
