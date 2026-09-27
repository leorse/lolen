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

#include "kyra/kyra_v1.h"
#include "kyra/sound/sound_intern.h"
#include "kyra/resource/resource.h"
#include "kyra/engine/timer.h"
#include "kyra/gui/debugger.h"
#include "kyra/text/text.h"

#include "common/error.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"

namespace Kyra {

KyraEngine_v1::KyraEngine_v1(OSystem *system, const GameFlags &flags)
	: Engine(system), _flags(flags), _rnd("kyra") {
	_res = nullptr;
	_sound = nullptr;
	_text = nullptr;
	_staticres = nullptr;
	_timer = nullptr;
	_emc = nullptr;

	_configRenderMode = Common::kRenderDefault;
	_configNullSound = false;

	_gameSpeed = 60;
	_tickLength = (uint8)(1000.0 / _gameSpeed);

	_trackMap = nullptr;
	_trackMapSize = 0;
	_lastMusicCommand = -1;
	_curSfxFile = _curMusicTheme = -1;
	_preventScriptSfx = false;

	_gameToLoad = -1;

	_mouseState = -1;
	_deathHandler = -1;

	memset(_flagsTable, 0, sizeof(_flagsTable));

	_isSaveAllowed = false;
	_totalPlaySecs = _lastSecTick = _lastSecTickAtPauseStart = 0;

	_mouseX = _mouseY = 0;
	_transOffsY = 0;
	_asciiCodeEvents = _kbEventSkip = false;

	_configMusic = 0;
	_configSounds = false;
	_configVoice = _configWalkspeed = 0;
}

void KyraEngine_v1::pauseEngineIntern(bool pause) {
	Engine::pauseEngineIntern(pause);
	if (_sound)
		_sound->pause(pause);
	if (_timer)
		_timer->pause(pause);
	pausePlayTimer(pause);
}

Common::Error KyraEngine_v1::init() {
	// Setup mixer
	syncSoundSettings();

	// Users who have specified a default GM device in the launcher will get GM music,
	// otherwise AdLib. Users who want MT-32 music have to select this individually
	// (since we assume that most users rather have a GM device than a MT-32 device).
	// Users who want PC speaker sound always have to select this individually.
	MidiDriver::DeviceHandle dev = MidiDriver::detectDevice(MDT_PCSPK | MDT_MIDI | MDT_ADLIB | MDT_PREFER_GM);
	const MusicType musicType = MidiDriver::getMusicType(dev);
	if (musicType == MT_ADLIB) {
		_sound = new SoundPC_v1(this, _mixer, Sound::kAdLib);
	} else {
		Sound::kType type;

		if (musicType == MT_PCSPK || musicType == MT_NULL)
			type = Sound::kPCSpkr;
		else if (musicType == MT_MT32 || ConfMan.getBool("native_mt32"))
			type = Sound::kMidiMT32;
		else
			type = Sound::kMidiGM;

		MidiDriver *driver = nullptr;

		if (musicType == MT_PCSPK) {
			driver = new MidiDriver_PCSpeaker(_mixer);
		} else {
			driver = MidiDriver::createMidi(dev);
			if (type == Sound::kMidiMT32)
				driver->property(MidiDriver::PROP_CHANNEL_MASK, 0x03FE);
		}

		assert(driver);

		SoundMidiPC *soundMidiPc = new SoundMidiPC(this, _mixer, driver, type);
		_sound = soundMidiPc;
		assert(_sound);

		// Unlike some SCUMM games, it's not that the MIDI sounds are
		// missing. It's just that at least at the time of writing they
		// are decidedly inferior to the AdLib ones.
		if (ConfMan.getBool("multi_midi")) {
			SoundPC_v1 *adlib = new SoundPC_v1(this, _mixer, Sound::kAdLib);
			assert(adlib);

			_sound = new MixedSoundDriver(this, _mixer, soundMidiPc, adlib);
		}
	}

	assert(_sound);

	if (_sound)
		_sound->updateVolumeSettings();

	if (ConfMan.hasKey("render_mode"))
		_configRenderMode = Common::parseRenderMode(ConfMan.get("render_mode"));

	_res = new Resource(this);
	assert(_res);
	_res->reset();

	_staticres = new StaticResource(this);
	assert(_staticres);
	if (!_staticres->init())
		error("_staticres->init() failed");
	assert(screen());
	if (!screen()->init())
		error("screen()->init() failed");
	_timer = new TimerManager(this, _system);
	assert(_timer);
	setupTimers();
	_emc = new EMCInterpreter(this);
	assert(_emc);

	setupOpcodeTable();
	readSettings();

	if (ConfMan.hasKey("save_slot")) {
		_gameToLoad = ConfMan.getInt("save_slot");
		if (!saveFileLoadable(_gameToLoad))
			_gameToLoad = -1;
	}

	setupKeyMap();

	return Common::kNoError;
}

KyraEngine_v1::~KyraEngine_v1() {
	for (Common::Array<const Opcode *>::iterator i = _opcodes.begin(); i != _opcodes.end(); ++i)
		delete *i;
	_opcodes.clear();
	_keyMap.clear();

	delete _res;
	delete _staticres;
	delete _sound;
	delete _text;
	delete _timer;
	delete _emc;
}

Common::Point KyraEngine_v1::getMousePos() {
	Common::Point mouse = _eventMan->getMousePos();
	mouse.y -= -_transOffsY;

	return mouse;
}

void KyraEngine_v1::setMousePos(int x, int y) {
	_system->warpMouse(x, y);

	// Feed the event manager an artficial mouse move event, since warpMouse() won't generate one.
	// From the warpMouse comments I gather that this behavior is intentional due to requirements of
	// the SCUMM engine. In KYRA we need to get the same coordinates from _eventMan->getMousePos()
	// that we send via warpMouse(). We have script situations in Kyra (like the Alchemists' crystals
	// scene) where a new mouse cursor position is set and then immediately read. This function would
	// then get wrong coordinates.
	Common::Event event;
	event.type = Common::EVENT_MOUSEMOVE;
	event.mouse.x = x;
	event.mouse.y = y;
	_eventMan->pushEvent(event);
	updateInput();
}

int KyraEngine_v1::checkInput(Button *buttonList, bool mainLoop, int eventFlag) {
	_isSaveAllowed = mainLoop;
	updateInput();
	_isSaveAllowed = false;

	int keys = 0;
	int8 mouseWheel = 0;

	while (!_eventList.empty()) {
		Common::Event event = *_eventList.begin();
		bool breakLoop = false;

		switch (event.type) {
		case Common::EVENT_KEYDOWN:
			if (event.kbd.keycode >= Common::KEYCODE_1 && event.kbd.keycode <= Common::KEYCODE_9 &&
			        (event.kbd.hasFlags(Common::KBD_CTRL) || event.kbd.hasFlags(Common::KBD_ALT)) && mainLoop) {
				int saveLoadSlot = 9 - (event.kbd.keycode - Common::KEYCODE_0) + 990;

				if (event.kbd.hasFlags(Common::KBD_CTRL)) {
					if (saveFileLoadable(saveLoadSlot))
						loadGameStateCheck(saveLoadSlot);
					_eventList.clear();
					breakLoop = true;
				} else {
					char savegameName[14];
					Common::sprintf_s(savegameName, "Quicksave %d", event.kbd.keycode - Common::KEYCODE_0);
					saveGameStateIntern(saveLoadSlot, savegameName, nullptr);
				}
			} else if (event.kbd.hasFlags(Common::KBD_CTRL)) {
				if (event.kbd.keycode == Common::KEYCODE_q) {
					quitGame();
				}
			} else {
				KeyMap::const_iterator keycode = _keyMap.find(event.kbd.keycode);
				if (_asciiCodeEvents) {
					keys = event.kbd.ascii;
				} else if (keycode != _keyMap.end()) {
					keys = keycode->_value;
					if (event.kbd.flags & Common::KBD_SHIFT)
						keys |= 0x100;
				} else {
					keys = 0;
				}

				// When we got a keypress, which we might need to handle,
				// break the event loop and pass it to GUI code.
				if (keys)
					breakLoop = true;
			}
			break;

		case Common::EVENT_LBUTTONDOWN:
		case Common::EVENT_LBUTTONUP: {
			_mouseX = event.mouse.x;
			_mouseY = event.mouse.y - _transOffsY;
			keys = (event.type == Common::EVENT_LBUTTONDOWN ? 199 : (200 | 0x800));
			breakLoop = true;
			} break;

		case Common::EVENT_RBUTTONDOWN:
		case Common::EVENT_RBUTTONUP: {
			_mouseX = event.mouse.x;
			_mouseY = event.mouse.y - _transOffsY;
			keys = (event.type == Common::EVENT_RBUTTONDOWN ? 201 : (202 | 0x800));
			breakLoop = true;
			} break;

		case Common::EVENT_WHEELUP:
			mouseWheel = -1;
			break;

		case Common::EVENT_WHEELDOWN:
			mouseWheel = 1;
			break;

		default:
			break;
		}

		if (breakLoop)
			break;

		_eventList.erase(_eventList.begin());
	}

	GUI *guiInstance = gui();
	if (guiInstance) {
		if (keys)
			return guiInstance->processButtonList(buttonList, keys | eventFlag, mouseWheel);
		else
			return guiInstance->processButtonList(buttonList, 0, mouseWheel);
	} else {
		return keys;
	}
}

void KyraEngine_v1::setupKeyMap() {
	struct KeyCodeMapEntry {
		Common::KeyCode kcScummVM;
		int16 kcDOS;
	};

#define UNKNOWN_KEYCODE -1
#define KC(x) Common::KEYCODE_##x
	static const KeyCodeMapEntry keys[] = {
		{ KC(SPACE), 61 },
		{ KC(RETURN), 43 },
		{ KC(UP), 96 },
		{ KC(KP8), 96 },
		{ KC(RIGHT), 102 },
		{ KC(KP6), 102 },
		{ KC(DOWN), 98 },
		{ KC(KP2), 98 },
		{ KC(KP5), 97 },
		{ KC(LEFT), 92 },
		{ KC(KP4), 92 },
		{ KC(HOME), 91 },
		{ KC(KP7), 91 },
		{ KC(PAGEUP), 101 },
		{ KC(KP9), 101 },
		{ KC(END), 93 },
		{ KC(KP1), 93 },
		{ KC(PAGEDOWN), 103 },
		{ KC(KP3), 103 },
		{ KC(F1), 112 },
		{ KC(F2), 113 },
		{ KC(F3), 114 },
		{ KC(F4), 115 },
		{ KC(F5), 116 },
		{ KC(F6), 117 },
		{ KC(TAB), 16 },
		{ KC(a), 31 },
		{ KC(b), 50 },
		{ KC(c), 48 },
		{ KC(d), 33 },
		{ KC(e), 19 },
		{ KC(f), 34 },
		{ KC(i), 24 },
		{ KC(k), 38 },
		{ KC(m), 52 },
		{ KC(n), 51 },
		{ KC(o), 25 },
		{ KC(p), 26 },
		{ KC(r), 20 },
		{ KC(s), 32 },
		{ KC(w), 18 },
		{ KC(y), 22 },
		{ KC(z), 46 },
		{ KC(0), UNKNOWN_KEYCODE },
		{ KC(1), 2 },
		{ KC(2), 3 },
		{ KC(3), 4 },
		{ KC(4), 5 },
		{ KC(5), 6 },
		{ KC(6), 7 },
		{ KC(7), 8 },
		{ KC(SLASH), 55 },
		{ KC(ESCAPE), 110 },
		{ KC(MINUS), 12 },
		{ KC(KP_MINUS), 105 },
		{ KC(PLUS), 13 },
		{ KC(KP_PLUS), 106 },

		// Multiple mappings for the keys to the right of the 'M' key,
		// since these are different for QWERTZ, QWERTY and AZERTY keyboards.
		// QWERTZ
		{ KC(COMMA), 53 },
		{ KC(PERIOD), 54 },
		// AZERTY
		{ KC(SEMICOLON), 53 },
		{ KC(COLON), 54 },
		// QWERTY
		{ KC(LESS), 53 },
		{ KC(GREATER), 54 }
	};
#undef KC
#undef UNKNOWN_KEYCODE

	_keyMap.clear();

	// If we have an engine that wants ASCII codes instead of key codes, we can skip the setup of the key map.
	// In that case we simply return the ASCII codes from the event manager. At least until I know better I
	// trust that the ASCII codes we get from our event manager are the same identical codes. If that assumption
	// turns out to be wrong I can still implement the original conversion method...
	if (_asciiCodeEvents)
		return;

	for (int i = 0; i < ARRAYSIZE(keys); i++)
		_keyMap[keys[i].kcScummVM] = keys[i].kcDOS;
}

void KyraEngine_v1::updateInput() {
	Common::Event event;

	bool updateScreen = false;

	while (_eventMan->pollEvent(event)) {
		switch (event.type) {
		case Common::EVENT_KEYDOWN:
			if (event.kbd.keycode == Common::KEYCODE_PERIOD || event.kbd.keycode == Common::KEYCODE_ESCAPE ||
			        event.kbd.keycode == Common::KEYCODE_SPACE || event.kbd.keycode == Common::KEYCODE_RETURN ||
			        event.kbd.keycode == Common::KEYCODE_UP || event.kbd.keycode == Common::KEYCODE_RIGHT ||
			        event.kbd.keycode == Common::KEYCODE_DOWN || event.kbd.keycode == Common::KEYCODE_LEFT)
				_eventList.push_back(Event(event, true));
			else if (event.kbd.keycode == Common::KEYCODE_q && event.kbd.hasFlags(Common::KBD_CTRL))
				quitGame();
			else
				_eventList.push_back(Event(event, _kbEventSkip));
			break;

		case Common::EVENT_LBUTTONDOWN:
		case Common::EVENT_RBUTTONDOWN:
			_eventList.push_back(Event(event, true));
			break;

		case Common::EVENT_MOUSEMOVE:
			if (screen()->isMouseVisible())
				updateScreen = true;
			break;

		case Common::EVENT_LBUTTONUP:
		case Common::EVENT_RBUTTONUP:
		case Common::EVENT_WHEELUP:
		case Common::EVENT_WHEELDOWN:
			_eventList.push_back(event);
			break;

		default:
			break;
		}
	}

	screen()->updateBackendScreen(updateScreen);
}

void KyraEngine_v1::removeInputTop() {
	if (!_eventList.empty())
		_eventList.erase(_eventList.begin());
}

bool KyraEngine_v1::skipFlag() const {
	for (Common::List<Event>::const_iterator i = _eventList.begin(); i != _eventList.end(); ++i) {
		if (i->causedSkip)
			return true;
	}
	return false;
}

void KyraEngine_v1::resetSkipFlag(bool removeEvent) {
	for (Common::List<Event>::iterator i = _eventList.begin(); i != _eventList.end(); ++i) {
		if (i->causedSkip) {
			if (removeEvent)
				_eventList.erase(i);
			else
				i->causedSkip = false;
			return;
		}
	}
}

int KyraEngine_v1::setGameFlag(int flag) {
	assert((flag >> 3) >= 0 && (flag >> 3) <= ARRAYSIZE(_flagsTable));
	_flagsTable[flag >> 3] |= (1 << (flag & 7));
	return 1;
}

int KyraEngine_v1::queryGameFlag(int flag) const {
	assert((flag >> 3) >= 0 && (flag >> 3) <= ARRAYSIZE(_flagsTable));
	return ((_flagsTable[flag >> 3] >> (flag & 7)) & 1);
}

int KyraEngine_v1::resetGameFlag(int flag) {
	assert((flag >> 3) >= 0 && (flag >> 3) <= ARRAYSIZE(_flagsTable));
	_flagsTable[flag >> 3] &= ~(1 << (flag & 7));
	return 0;
}

void KyraEngine_v1::delayUntil(uint32 timestamp, bool updateTimers, bool update, bool isMainLoop) {
	const uint32 curTime = _system->getMillis();
	if (curTime > timestamp)
		return;

	uint32 del = timestamp - curTime;
	while (del && !shouldQuit()) {
		uint32 step = MIN<uint32>(del, _tickLength);
		delay(step, update, isMainLoop);
		del -= step;
	}
}

void KyraEngine_v1::delay(uint32 amount, bool update, bool isMainLoop) {
	_system->delayMillis(amount);
}

void KyraEngine_v1::delayWithTicks(int ticks) {
	delay(ticks * _tickLength);
}

void KyraEngine_v1::registerDefaultSettings() {
	if (_flags.fanLang != Common::UNK_LANG) {
		// HACK/WORKAROUND: Since we can't use registerDefault here to overwrite
		// the global subtitles settings, we're using this hack to enable subtitles
		// for fan translations
		const Common::ConfigManager::Domain *cur = ConfMan.getActiveDomain();
		if (!cur || (cur && !cur->contains("subtitles")))
			ConfMan.setBool("subtitles", true);
	}
}

void KyraEngine_v1::readSettings() {
	_configWalkspeed = ConfMan.getInt("walkspeed");
	_configMusic = 0;

	if (!ConfMan.getBool("music_mute")) {
		_configMusic = 1;
	}
	_configSounds = ConfMan.getBool("sfx_mute") ? 0 : 1;

	if (_sound) {
		_sound->enableMusic(_configNullSound ? false : _configMusic);
		_sound->enableSFX(_configNullSound ? false : _configSounds);
	}

	bool speechMute = ConfMan.getBool("speech_mute");
	bool subtitles = ConfMan.getBool("subtitles");

	if (!_configNullSound && !speechMute && subtitles)
		_configVoice = 2;   // Voice & Text
	else if (!_configNullSound && !speechMute && !subtitles)
		_configVoice = 1;   // Voice only
	else
		_configVoice = 0;   // Text only

	setWalkspeed(_configWalkspeed);
}

void KyraEngine_v1::writeSettings() {
	bool speechMute, subtitles;

	ConfMan.setInt("walkspeed", _configWalkspeed);
	ConfMan.setBool("music_mute", _configMusic == 0);
	ConfMan.setBool("sfx_mute", _configSounds == 0);

	switch (_configVoice) {
	case 0:     // Text only
		speechMute = true;
		subtitles = true;
		break;
	case 1:     // Voice only
		speechMute = false;
		subtitles = false;
		break;
	default:    // Voice & Text
		speechMute = false;
		subtitles = true;
	}

	if (_sound) {
		if (!_configMusic)
			_sound->beginFadeOut();
		_sound->enableMusic(_configNullSound ? false : _configMusic);
		_sound->enableSFX(_configNullSound ? false : _configSounds);
	}

	ConfMan.setBool("speech_mute", speechMute);
	ConfMan.setBool("subtitles", subtitles);

	ConfMan.flushToDisk();
}

bool KyraEngine_v1::speechEnabled() {
	return _flags.isTalkie && (_configVoice == 1 || _configVoice == 2);
}

bool KyraEngine_v1::textEnabled() {
	return !_flags.isTalkie || (_configVoice == 0 || _configVoice == 2);
}

int KyraEngine_v1::convertVolumeToMixer(int value) {
	value -= 2;
	return (value * Audio::Mixer::kMaxMixerVolume) / 95;
}

int KyraEngine_v1::convertVolumeFromMixer(int value) {
	return (value * 95) / Audio::Mixer::kMaxMixerVolume + 2;
}

void KyraEngine_v1::setVolume(kVolumeEntry vol, uint8 value) {
	switch (vol) {
	case kVolumeMusic:
		ConfMan.setInt("music_volume", convertVolumeToMixer(value));
		break;

	case kVolumeSfx:
		ConfMan.setInt("sfx_volume", convertVolumeToMixer(value));
		break;

	case kVolumeSpeech:
		ConfMan.setInt("speech_volume", convertVolumeToMixer(value));
		break;

	default:
		break;
	}

	// Resetup mixer
	_mixer->setVolumeForSoundType(Audio::Mixer::kSFXSoundType, ConfMan.getInt("sfx_volume"));
	_mixer->setVolumeForSoundType(Audio::Mixer::kMusicSoundType, ConfMan.getInt("music_volume"));
	_mixer->setVolumeForSoundType(Audio::Mixer::kSpeechSoundType, ConfMan.getInt("speech_volume"));
	if (_sound)
		_sound->updateVolumeSettings();
}

uint8 KyraEngine_v1::getVolume(kVolumeEntry vol) {
	switch (vol) {
	case kVolumeMusic:
		return convertVolumeFromMixer(ConfMan.getInt("music_volume"));

	case kVolumeSfx:
		return convertVolumeFromMixer(ConfMan.getInt("sfx_volume"));

	case kVolumeSpeech:
		if (speechEnabled())
			return convertVolumeFromMixer(ConfMan.getInt("speech_volume"));
		else
			return 2;
		break;

	default:
		break;
	}

	return 2;
}

void KyraEngine_v1::syncSoundSettings() {
	Engine::syncSoundSettings();

	// We need to use this here to allow the subtitle options to be changed
	// through the GMM's options dialog.
	readSettings();

	if (_sound)
		_sound->updateVolumeSettings();
}

void KyraEngine_v1::updatePlayTimer() {
	uint32 curTime = _system->getMillis();
	while (_lastSecTick + 1000 <= curTime) {
		_lastSecTick += 1000;
		_totalPlaySecs++;
	}
}

void KyraEngine_v1::restartPlayTimerAt(uint32 totalPlaySecs) {
	_lastSecTick = _system->getMillis();
	_totalPlaySecs = totalPlaySecs;
}

void KyraEngine_v1::pausePlayTimer(bool pause) {
	if (pause)
		_lastSecTickAtPauseStart = _lastSecTick;
	else
		_lastSecTick += (_system->getMillis() - _lastSecTickAtPauseStart);
}

} // End of namespace Kyra
