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

// The usage text is printed on the console.
#define FORBIDDEN_SYMBOL_EXCEPTION_printf

#include "lolen/application.h"
#include "lolen/config.h"

#include "lolen/splash.h"

#include "engines/engine.h"
#include "engines/metaengine.h"
#include "engines/util.h"
#include "base/plugins.h"
#include "base/version.h"

#include "common/archive.h"
#include "common/config-manager.h"
#include "common/debug.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/fs.h"
#include "common/language.h"
#include "common/osd_message_queue.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/tokenizer.h"
#include "common/translation.h"

#include "audio/musicplugin.h"

#include "graphics/cursorman.h"
#include "graphics/fontman.h"
#include "graphics/yuv_to_rgb.h"

#include "gui/gui-manager.h"

#include "backends/keymapper/keymap.h"
#include "backends/keymapper/keymapper.h"

namespace LoLEN {

// Name of the game domain in lolen.ini. It is also the prefix of the savegame file names.
static const char *const kTarget = "lol";
// Engine and game ids of Lands of Lore in the Kyra engine.
static const char *const kEngineId = "kyra";
static const char *const kGameId = "lol";

static const char *const kConfigFileName = "lolen.ini";
static const char *const kSaveDirName = "saves";
static const char *const kWindowCaption = "LoLEN - Lands of Lore: The Throne of Chaos";

Application::Application(OSystem &system, const Common::Path &baseDir)
	: _system(system), _baseDir(baseDir), _debugLevel(-1), _cmdFullscreen(false), _cmdWindowed(false) {
}

Application::~Application() {
}

void Application::printUsage() const {
	printf("Usage: lolen [OPTIONS] [GAME_PATH]\n"
	       "\n"
	       "Starts Lands of Lore: The Throne of Chaos.\n"
	       "\n"
	       "  GAME_PATH              Directory with the game data files\n"
	       "  --path=DIR             Same as GAME_PATH\n"
	       "  --language=CODE        Game language (en, fr, de, es, it, ru, zh...)\n"
	       "  -f, --fullscreen       Start in fullscreen mode\n"
	       "  -w, --windowed         Start in windowed mode\n"
	       "  --debugflags=FLAGS     Enable the given debug channels (comma separated, or 'all')\n"
	       "  -d, --debuglevel=NUM   Set the debug level\n"
	       "  -h, --help             Show this text\n"
	       "\n"
	       "The settings are stored in lolen.ini next to the executable.\n"
	       "\n"
	       "LoLEN is based on %s.\n", gScummVMFullVersion);
}

bool Application::parseCommandLine(int argc, const char *const argv[]) {
	for (int i = 1; i < argc; ++i) {
		Common::String arg(argv[i]);

		if (arg == "-h" || arg == "--help") {
			printUsage();
			return false;
		} else if (arg.hasPrefix("--path=")) {
			_cmdGamePath = Common::Path(arg.substr(7), Common::Path::kNativeSeparator);
		} else if (arg.hasPrefix("--language=")) {
			_cmdLanguage = arg.substr(11);
		} else if (arg == "-f" || arg == "--fullscreen") {
			_cmdFullscreen = true;
		} else if (arg == "-w" || arg == "--windowed") {
			_cmdWindowed = true;
		} else if (arg.hasPrefix("--debugflags=")) {
			_debugFlags = arg.substr(13);
		} else if (arg.hasPrefix("--debuglevel=")) {
			_debugLevel = atoi(arg.substr(13).c_str());
		} else if (arg.hasPrefix("-d") && arg.size() > 2) {
			_debugLevel = atoi(arg.substr(2).c_str());
		} else if (!arg.hasPrefix("-")) {
			_cmdGamePath = Common::Path(arg, Common::Path::kNativeSeparator);
		} else {
			warning("Unknown command line option '%s'", arg.c_str());
		}
	}

	return true;
}

void Application::registerDefaults() {
	// Graphics
	ConfMan.registerDefault("fullscreen", false);
	ConfMan.registerDefault("filtering", false);
	ConfMan.registerDefault("aspect_ratio", true);
	ConfMan.registerDefault("gfx_mode", "normal");
	ConfMan.registerDefault("render_mode", "default");
	ConfMan.registerDefault("desired_screen_aspect_ratio", "auto");
	ConfMan.registerDefault("stretch_mode", "default");
	ConfMan.registerDefault("scaler", "default");
	ConfMan.registerDefault("scale_factor", -1);
	ConfMan.registerDefault("shader", Common::Path("default", Common::Path::kNoSeparator));
	ConfMan.registerDefault("show_fps", false);
	ConfMan.registerDefault("dirtyrects", true);
	ConfMan.registerDefault("vsync", true);
	ConfMan.registerDefault("rotation_mode", 0);

	// Sound & Music
	ConfMan.registerDefault("music_volume", 192);
	ConfMan.registerDefault("sfx_volume", 192);
	ConfMan.registerDefault("speech_volume", 192);

	ConfMan.registerDefault("music_mute", false);
	ConfMan.registerDefault("sfx_mute", false);
	ConfMan.registerDefault("speech_mute", false);
	ConfMan.registerDefault("mute", false);

	ConfMan.registerDefault("multi_midi", false);
	ConfMan.registerDefault("native_mt32", false);
	ConfMan.registerDefault("dump_midi", false);
	ConfMan.registerDefault("enable_gs", false);
	ConfMan.registerDefault("midi_gain", 100);

	ConfMan.registerDefault("music_driver", "auto");
	ConfMan.registerDefault("mt32_device", "null");
	ConfMan.registerDefault("gm_device", "auto");
	ConfMan.registerDefault("opl2lpt_parport", "null");

	ConfMan.registerDefault("cdrom", 0);

	// Game
	ConfMan.registerDefault("path", Common::Path());
	ConfMan.registerDefault("platform", Common::kPlatformDOS);
	ConfMan.registerDefault("language", "en");
	ConfMan.registerDefault("subtitles", false);
	ConfMan.registerDefault("save_slot", -1);
	ConfMan.registerDefault("autosave_period", 5 * 60);
	ConfMan.registerDefault("talkspeed", 60);
	ConfMan.registerDefault("copy_protection", false);

	// Miscellaneous
	ConfMan.registerDefault("joystick_num", 0);
	ConfMan.registerDefault("confirm_exit", false);
	ConfMan.registerDefault("disable_sdl_parachute", false);
	ConfMan.registerDefault("disable_sdl_audio", false);

	// The in-game menu of ScummVM (Ctrl+F5) still uses the ScummVM GUI
	ConfMan.registerDefault("gui_saveload_chooser", "grid");
	ConfMan.registerDefault("gui_saveload_last_pos", "0");
	ConfMan.registerDefault("gui_browser_show_hidden", false);
	ConfMan.registerDefault("gui_browser_native", true);
	ConfMan.registerDefault("gui_return_to_launcher_at_exit", false);
}

void Application::loadConfig() {
	_config.reset(new Config(_baseDir.join(kConfigFileName), kTarget));
	if (!_config->load())
		debug("Creating configuration file: %s", _config->getFilename().toString(Common::Path::kNativeSeparator).c_str());

	// From now on, ConfMan.flushToDisk() (called by the engine when its options change)
	// writes lolen.ini in the LoLEN format.
	_config->install();

	// Savegames are stored next to the executable, unless the user chose another place.
	if (!ConfMan.hasKey("savepath", kTarget) && !ConfMan.hasKey("savepath", Common::ConfigManager::kApplicationDomain)) {
		Common::FSNode saveDir(_baseDir.join(kSaveDirName));
		if (!saveDir.exists() && !saveDir.createDirectory())
			warning("Could not create the savegame directory '%s'", saveDir.getPath().toString(Common::Path::kNativeSeparator).c_str());
		ConfMan.setPath("savepath", saveDir.getPath(), kTarget);
	}

	if (_debugLevel == -1 && ConfMan.hasKey("debuglevel"))
		_debugLevel = ConfMan.getInt("debuglevel");
	if (_debugLevel != -1)
		gDebugLevel = _debugLevel;

	if (_debugFlags.empty() && ConfMan.hasKey("debugflags"))
		_debugFlags = ConfMan.get("debugflags");

	// The in-game menu of ScummVM (Ctrl+F5) uses the built-in GUI theme, the launcher themes are
	// not shipped. The built-in theme only has bitmap fonts, which the ScummVM GUI only accepts
	// for English, so this menu is shown in English. The game itself keeps its own language.
	ConfMan.set("gui_theme", "builtin", Common::ConfigManager::kSessionDomain);
	ConfMan.set("gui_language", "C", Common::ConfigManager::kSessionDomain);

	if (_cmdFullscreen)
		ConfMan.setBool("fullscreen", true, Common::ConfigManager::kTransientDomain);
	else if (_cmdWindowed)
		ConfMan.setBool("fullscreen", false, Common::ConfigManager::kTransientDomain);
}

void Application::setupGameDomain() {
	if (!ConfMan.hasGameDomain(kTarget))
		ConfMan.addGameDomain(kTarget);

	Common::ConfigManager::Domain *domain = ConfMan.getDomain(kTarget);
	domain->setVal("engineid", kEngineId);
	domain->setVal("gameid", kGameId);

	// Game path: command line, then configuration file, then the directory of the executable.
	if (!_cmdGamePath.empty())
		domain->setVal("path", _cmdGamePath.toConfig());
	else if (!domain->contains("path"))
		domain->setVal("path", _baseDir.toConfig());

	if (!_cmdLanguage.empty())
		domain->setVal("language", _cmdLanguage);

	ConfMan.setActiveDomain(kTarget);
}

void Application::setupGraphics() {
	_system.beginGFXTransaction();
		_system.setGraphicsMode(ConfMan.get("gfx_mode").c_str());
		_system.setStretchMode(ConfMan.get("stretch_mode").c_str());
		_system.setScaler(ConfMan.get("scaler").c_str(), ConfMan.getInt("scale_factor"));
		_system.setShader(ConfMan.getPath("shader"));
		_system.setRotationMode(ConfMan.getInt("rotation_mode"));
		_system.initSize(320, 200);

		_system.setFeatureState(OSystem::kFeatureAspectRatioCorrection, ConfMan.getBool("aspect_ratio"));
		_system.setFeatureState(OSystem::kFeatureFullscreenMode, ConfMan.getBool("fullscreen"));
		_system.setFeatureState(OSystem::kFeatureFilteringMode, ConfMan.getBool("filtering"));
		_system.setFeatureState(OSystem::kFeatureVSync, ConfMan.getBool("vsync"));
	_system.endGFXTransaction();

	_system.applyBackendSettings();
	_system.setWindowCaption(Common::U32String(kWindowCaption));
	_system.fillScreen(0);
}

void Application::setupKeymapper() {
	Common::Keymapper *mapper = _system.getEventManager()->getKeymapper();
	mapper->clear();

	// Query the backend for hardware keys and default bindings and register them
	mapper->registerHardwareInputSet(_system.getHardwareInputSet(), _system.getKeymapperDefaultBindings());

	Common::Keymap *primaryGlobalKeymap = _system.getEventManager()->getGlobalKeymap();
	if (primaryGlobalKeymap)
		mapper->addGlobalKeymap(primaryGlobalKeymap);

	// Get the platform-specific global keymaps (if any)
	Common::KeymapArray platformKeymaps = _system.getGlobalKeymaps();
	for (auto &platformKeymap : platformKeymaps)
		mapper->addGlobalKeymap(platformKeymap);
}

Common::Error Application::identifyGame(const Plugin *&detectionPlugin, DetectedGame &game, const void *&descriptor) {
	detectionPlugin = EngineMan.findDetectionPlugin(kEngineId);
	if (!detectionPlugin)
		return Common::kMetaEnginePluginNotFound;

	MetaEngineDetection &metaEngine = detectionPlugin->get<MetaEngineDetection>();

	DebugMan.addAllDebugChannels(metaEngine.getDebugChannels());
	Common::StringTokenizer tokenizer(_debugFlags, " ,");
	while (!tokenizer.empty()) {
		Common::String token = tokenizer.nextToken();
		if (token.equalsIgnoreCase("all"))
			DebugMan.enableAllDebugChannels();
		else if (!DebugMan.enableDebugChannel(token))
			warning("Unknown debug channel '%s'", token.c_str());
	}

	// Multi-language versions (e.g. the CD version) are detected once per language.
	// Without a language setting we try the language of the system first.
	const bool hasLanguage = ConfMan.hasKey("language", kTarget);
	if (!hasLanguage) {
		Common::Language systemLanguage = Common::parseLanguageFromLocale(_system.getSystemLanguage().c_str());
		if (systemLanguage != Common::UNK_LANG) {
			ConfMan.set("language", Common::getLanguageCode(systemLanguage), kTarget);
			if (metaEngine.identifyGame(game, &descriptor).getCode() == Common::kNoError)
				return Common::kNoError;
			ConfMan.removeKey("language", kTarget);
		}
	}

	Common::Error result = metaEngine.identifyGame(game, &descriptor);
	if (result.getCode() == Common::kNoError && !hasLanguage && game.language != Common::UNK_LANG)
		ConfMan.set("language", Common::getLanguageCode(game.language), kTarget);

	return result;
}

Common::Error Application::runGame(const Plugin *enginePlugin, const DetectedGame &game, const void *descriptor) {
	Common::FSNode dir(ConfMan.getPath("path"));
	Engine *engine = nullptr;

	MetaEngine &metaEngine = enginePlugin->get<MetaEngine>();

	// Some engines query their options in their constructor, thus the defaults
	// need to be registered before the instance is created.
	metaEngine.registerDefaultSettings(kTarget);
	Common::Error err = metaEngine.createInstance(&_system, &engine, game, descriptor);

	if (!engine || err.getCode() != Common::kNoError) {
		warning("Failed to instantiate the engine: %s (path '%s')", err.getDesc().c_str(),
		        dir.getPath().toString(Common::Path::kNativeSeparator).c_str());
		metaEngine.deleteInstance(engine, game, descriptor);
		return err;
	}

	engine->setMetaEngine(&metaEngine);
	_system.setWindowCaption(Common::U32String(kWindowCaption));

	// Add the game path to the directory search list
	engine->initializePath(dir);

	// Game specific keymaps
	Common::Keymapper *keymapper = _system.getEventManager()->getKeymapper();
	Common::KeymapArray gameKeymaps = metaEngine.initKeymaps(kTarget);
	for (auto &gameKeymap : gameKeymaps)
		keymapper->addGameKeymap(gameKeymap);

	_system.applyBackendSettings();
	_system.engineInit();

	_system.getEventManager()->purgeKeyboardEvents();
	_system.getEventManager()->purgeMouseEvents();

	Common::Error result = engine->run();

	_system.engineDone();
	keymapper->cleanupGameKeymaps();
	metaEngine.deleteInstance(engine, game, descriptor);
	SearchMan.clear();

	return result;
}

void Application::showError(const Common::String &message) {
	warning("%s", message.c_str());
	_system.messageBox(LogMessageType::kError, message.c_str());
}

void Application::shutdown() {
	PluginManager::destroy();
	GUI::GuiManager::destroy();
	Common::ConfigManager::destroy();
	Common::DebugManager::destroy();
	Common::OSDMessageQueue::destroy();
	Common::SearchManager::destroy();
#ifdef USE_TRANSLATION
	Common::MainTranslationManager::destroy();
#endif
	MusicManager::destroy();
	Graphics::CursorManager::destroy();
	Graphics::FontManager::destroy();
	EngineManager::destroy();
	Graphics::YUVToRGBManager::destroy();
}

int Application::run(int argc, const char *const argv[]) {
	if (!parseCommandLine(argc, argv))
		return 0;

	registerDefaults();
	_system.registerDefaultSettings(Common::ConfigManager::kApplicationDomain);
	loadConfig();
	setupGameDomain();

	PluginManager::instance().init();
	PluginManager::instance().loadAllPlugins();
	PluginManager::instance().loadDetectionPlugin();

	_system.initBackend();
	setupGraphics();

	// Init the managers used by the engine
	_system.getAudioCDManager();
	MusicManager::instance();
	Common::DebugManager::instance();

	_system.getEventManager()->init();
	Common::OSDMessageQueue::instance().registerEventSource();
	setupKeymapper();

	const Plugin *detectionPlugin = nullptr;
	DetectedGame game;
	const void *descriptor = nullptr;
	Common::Error result = identifyGame(detectionPlugin, game, descriptor);

	if (result.getCode() != Common::kNoError) {
		showError(Common::String::format(
			"Lands of Lore was not found in '%s'.\n\n"
			"Start LoLEN with the game directory as argument, or set 'path' in the [game] section of %s.",
			ConfMan.getPath("path").toString(Common::Path::kNativeSeparator).c_str(), kConfigFileName));
	} else {
		// Remember the settings (game path, detected language...) for the next start.
		ConfMan.flushToDisk();

		const Plugin *enginePlugin = PluginMan.findEnginePlugin(kEngineId);
		if (!enginePlugin) {
			result = Common::kEnginePluginNotFound;
			showError("The Lands of Lore engine is missing from this build.");
		} else {
			PluginManager::instance().unloadPluginsExcept(PLUGIN_TYPE_ENGINE, enginePlugin);

			// The ScummVM splash screen first, then the LoLEN one. The engine does not show
			// the ScummVM splash screen again once it has been displayed.
			splashScreen();
			showSplashScreen();

			result = runGame(enginePlugin, game, descriptor);
			if (result.getCode() != Common::kNoError && result.getCode() != Common::kUserCanceled)
				showError("Error running Lands of Lore: " + result.getDesc());
		}

		// Settings changed while playing (window size...)
		_config->save();
	}

	DebugMan.removeAllDebugChannels();
	_config.reset();
	shutdown();

	return result.getCode() == Common::kUserCanceled ? 0 : result.getCode();
}

} // End of namespace LoLEN
