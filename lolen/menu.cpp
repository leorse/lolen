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

#include "lolen/menu.h"

#include "engines/engine.h"
#include "engines/metaengine.h"
#include "base/version.h"

#include "common/config-manager.h"
#include "common/events.h"
#include "common/system.h"
#include "common/translation.h"

#include "gui/about.h"
#include "gui/dialog.h"
#include "gui/gui-manager.h"
#include "gui/message.h"
#include "gui/options.h"
#include "gui/ThemeEval.h"
#include "gui/widget.h"
#include "gui/widgets/popup.h"
#include "gui/widgets/scrollcontainer.h"
#include "gui/widgets/tab.h"

#include "backends/keymapper/keymap.h"

namespace LoLEN {

namespace {

Common::Array<Common::Language> s_languages;
bool s_restartRequested = false;

// Command of the "Apply" button, handled by GUI::OptionsDialog
const uint32 kApplyCmd = 'appl';

// Settings edited in the options dialog. The dialog edits the application
// domain; the values the engine stored in the game domain are moved there first.
const char *const kApplicationSettings[] = {
	"music_volume", "sfx_volume", "speech_volume", "mute",
	"music_driver", "opl_driver", "native_mt32", "enable_gs", "multi_midi", "midi_gain",
	"mt32_device", "gm_device", "soundfont",
	"fullscreen", "aspect_ratio", "filtering", "vsync", "gfx_mode", "stretch_mode",
	"scaler", "scale_factor", "render_mode", "shader",
	"kbdmouse_speed", "joystick_deadzone"
};

// Settings the engine only reads at startup: changing them restarts the game
const char *const kMusicSettings[] = {
	"music_driver", "opl_driver", "native_mt32", "enable_gs", "multi_midi", "midi_gain",
	"mt32_device", "gm_device", "soundfont"
};

int scaled(int value) {
	return (int)(value * g_gui.getScaleFactor());
}

void moveGameSettingsToApplicationDomain() {
	const Common::String &gameDomain = ConfMan.getActiveDomainName();
	if (gameDomain.empty())
		return;

	for (const char *key : kApplicationSettings) {
		if (!ConfMan.hasKey(key, gameDomain))
			continue;
		ConfMan.set(key, ConfMan.get(key, gameDomain), Common::ConfigManager::kApplicationDomain);
		ConfMan.removeKey(key, gameDomain);
	}
}

Common::String musicSettings() {
	Common::String result;
	for (const char *key : kMusicSettings)
		result += Common::String::format("%s=%s;", key, ConfMan.get(key).c_str());
	return result;
}

bool confirmRestart(const Common::U32String &message) {
	GUI::MessageDialog dialog(message, _("Restart"), _("Cancel"));
	return dialog.runModal() == GUI::kMessageOK;
}

//
// Options: sound, music, graphics and controls
//

class OptionsDialog : public GUI::OptionsDialog {
public:
	enum Page {
		kPageSound,
		kPageGraphics,
		kPageControls
	};

	explicit OptionsDialog(Page page);

	bool musicChanged() const { return _musicChanged; }

	void open() override;
	void apply() override;

protected:
	void build() override;
	void clean() override;

private:
	const Page _page;
	int _soundTabId;
	int _controlsTabId;
	bool _musicChanged;
};

OptionsDialog::OptionsDialog(Page page)
	: GUI::OptionsDialog(Common::ConfigManager::kApplicationDomain, "GlobalOptions"),
	  _page(page), _soundTabId(-1), _controlsTabId(-1), _musicChanged(false) {
	moveGameSettingsToApplicationDomain();

	new GUI::ButtonWidget(this, "GlobalOptions.Cancel", _("~C~ancel"), Common::U32String(), GUI::kCloseCmd);
	new GUI::ButtonWidget(this, "GlobalOptions.Apply", _("~A~pply"), Common::U32String(), kApplyCmd);
	new GUI::ButtonWidget(this, "GlobalOptions.Ok", _("~O~K"), Common::U32String(), GUI::kOKCmd);
}

void OptionsDialog::build() {
	if (!_tabWidget) {
		GUI::TabWidget *tab = new GUI::TabWidget(this, "GlobalOptions.TabWidget");
		_tabWidget = tab;

		// Sound
		_soundTabId = tab->addTab(_("Sound"), "GlobalOptions_Volume");
		addVolumeControls(tab, "GlobalOptions_Volume.");

		// Music: AdLib and MT-32 are emulated, no specific hardware is needed
		tab->addTab(_("Music"), "GlobalOptions_Audio");
		addAudioControls(tab, "GlobalOptions_Audio.");

		_midiTabId = tab->addTab(_("MIDI"), "GlobalOptions_MIDI");
		addMIDIControls(tab, "GlobalOptions_MIDI.");

		tab->addTab(_("MT-32"), "GlobalOptions_MT32");
		addMT32Controls(tab, "GlobalOptions_MT32.");

		// Graphics
		_graphicsTabId = tab->addTab(_("Graphics"), "GlobalOptions_Graphics");
		GUI::ScrollContainerWidget *graphicsContainer = new GUI::ScrollContainerWidget(tab, "GlobalOptions_Graphics.Container", "GlobalOptions_Graphics_Container");
		graphicsContainer->setTarget(this);
		graphicsContainer->setBackgroundType(GUI::ThemeEngine::kWidgetBackgroundNo);
		addGraphicControls(graphicsContainer, "GlobalOptions_Graphics_Container.");

		// Mouse speed and gamepad dead zone
		if (g_system->hasFeature(OSystem::kFeatureTouchpadMode) ||
		    g_system->hasFeature(OSystem::kFeatureKbdMouseSpeed) ||
		    g_system->hasFeature(OSystem::kFeatureJoystickDeadzone)) {
			tab->addTab(_("Gamepad"), "GlobalOptions_Control");
			addControlControls(tab, "GlobalOptions_Control.");
		}

		// Keyboard and gamepad mappings: the global actions (menu...) and the game actions.
		// They are all stored in the keymapper domain ([controls] in lolen.ini).
		Common::KeymapArray keymaps;
		Common::Keymap *globalKeymap = g_system->getEventManager()->getGlobalKeymap();
		if (globalKeymap && !globalKeymap->getActions().empty())
			keymaps.push_back(globalKeymap);
		else
			delete globalKeymap;
		keymaps.push_back(g_system->getGlobalKeymaps());
		if (g_engine && g_engine->getMetaEngine())
			keymaps.push_back(g_engine->getMetaEngine()->initKeymaps(ConfMan.getActiveDomainName().c_str()));

		_controlsTabId = tab->addTab(_("Controls"), "GlobalOptions_KeyMapper");
		GUI::ScrollContainerWidget *keymapContainer = new GUI::ScrollContainerWidget(tab, "GlobalOptions_KeyMapper.Container", "GlobalOptions_KeyMapper_Container");
		keymapContainer->setBackgroundType(GUI::ThemeEngine::kWidgetBackgroundNo);
		keymapContainer->setTarget(this);
		addKeyMapperControls(keymapContainer, "GlobalOptions_KeyMapper_Container.", keymaps, Common::ConfigManager::kKeymapperDomain);
	}

	GUI::OptionsDialog::build();

	// The settings are always edited, there is no "override global settings" here
	setVolumeSettingsState(true);
	setAudioSettingsState(true);
	setMIDISettingsState(true);
	setMT32SettingsState(true);
	setGraphicSettingsState(true);
}

void OptionsDialog::clean() {
	GUI::OptionsDialog::clean();
	_tabWidget = nullptr;
}

void OptionsDialog::open() {
	GUI::OptionsDialog::open();

	switch (_page) {
	case kPageGraphics:
		_tabWidget->setActiveTab(_graphicsTabId);
		break;
	case kPageControls:
		_tabWidget->setActiveTab(_controlsTabId);
		break;
	default:
		_tabWidget->setActiveTab(_soundTabId);
		break;
	}
}

void OptionsDialog::apply() {
	const Common::String previousMusic = musicSettings();
	GUI::OptionsDialog::apply();
	if (musicSettings() != previousMusic)
		_musicChanged = true;
}

//
// Language of the game
//

class LanguageDialog : public GUI::Dialog {
public:
	LanguageDialog(Common::Language current);

	Common::Language getLanguage() const;

	void handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data) override;
	void reflowLayout() override;

private:
	GUI::PopUpWidget *_popUp;
};

LanguageDialog::LanguageDialog(Common::Language current) : GUI::Dialog("LoLENLanguage") {
	GUI::StaticTextWidget *title = new GUI::StaticTextWidget(this, "LoLENLanguage.Title", _("Game language"));
	title->setAlign(Graphics::kTextAlignCenter);

	_popUp = new GUI::PopUpWidget(this, "LoLENLanguage.List");
	for (uint i = 0; i < s_languages.size(); ++i)
		_popUp->appendEntry(Common::String(Common::getLanguageDescription(s_languages[i])), i);
	_popUp->setSelected(0);
	for (uint i = 0; i < s_languages.size(); ++i) {
		if (s_languages[i] == current)
			_popUp->setSelectedTag(i);
	}

	new GUI::ButtonWidget(this, "LoLENLanguage.Cancel", _("~C~ancel"), Common::U32String(), GUI::kCloseCmd);
	new GUI::ButtonWidget(this, "LoLENLanguage.Ok", _("~O~K"), Common::U32String(), GUI::kOKCmd);
}

Common::Language LanguageDialog::getLanguage() const {
	const uint32 index = _popUp->getSelectedTag();
	return index < s_languages.size() ? s_languages[index] : Common::UNK_LANG;
}

void LanguageDialog::handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data) {
	if (cmd == GUI::kOKCmd) {
		setResult(1);
		close();
	} else {
		GUI::Dialog::handleCommand(sender, cmd, data);
	}
}

void LanguageDialog::reflowLayout() {
	// The layout is defined here since the built-in theme does not know it.
	// The theme drops it when it is reloaded (e.g. when the window size changes).
	GUI::ThemeEval &eval = *g_gui.xmlEval();
	if (!eval.hasDialog(_name)) {
		const int lineHeight = eval.getVar("Globals.Line.Height");
		const int popUpHeight = eval.getVar("Globals.PopUp.Height", lineHeight);

		eval.addDialog(_name, "screen_center");
		eval.addLayout(GUI::ThemeLayout::kLayoutVertical, -1, GUI::ThemeLayout::kItemAlignCenter).addPadding(16, 16, 16, 16);
		eval.addWidget("Title", "", scaled(240), lineHeight);
		eval.addSpace(scaled(8));
		eval.addWidget("List", "", scaled(240), popUpHeight);
		eval.addSpace(scaled(16));
		eval.addLayout(GUI::ThemeLayout::kLayoutHorizontal).addPadding(0, 0, 0, 0);
		eval.addSpace();
		eval.addWidget("Cancel", "Button");
		eval.addWidget("Ok", "Button");
		eval.closeLayout();
		eval.closeLayout().closeDialog();
	}

	GUI::Dialog::reflowLayout();
}

//
// Credits: LoLEN, the Kyra engine team, then the ScummVM credits (AUTHORS)
//

class CreditsDialog : public GUI::AboutDialog {
public:
	CreditsDialog() : GUI::AboutDialog(true) {
		// The base class constructor built the lines without the LoLEN header
		reflowLayout();
	}

protected:
	void addHeaderLines() override {
		addLine(Common::U32String("C1LoLEN"));
		addLine(Common::U32String("C0Lands of Lore Engine"));
		addLine(Common::U32String(Common::String("C2Based on ScummVM ") + gScummVMVersion));
		_lines.push_back(Common::U32String());
		addLine(Common::U32String("C0Lands of Lore: The Throne of Chaos (C) 1993 Westwood Studios"));
		_lines.push_back(Common::U32String());
		addLine(Common::U32String("C2The game runs on the Kyra engine of ScummVM, written by:"));
		addCreditsSection("Kyra");
		_lines.push_back(Common::U32String());
		_lines.push_back(Common::U32String());
	}
};

//
// Main menu
//

class MainMenuDialog : public GUI::Dialog {
public:
	MainMenuDialog();

	void handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data) override;
	void reflowLayout() override;

private:
	enum {
		kResumeCmd = 'RSUM',
		kLanguageCmd = 'LANG',
		kSoundCmd = 'SOND',
		kGraphicsCmd = 'GRAF',
		kControlsCmd = 'CTRL',
		kCreditsCmd = 'CRED',
		kQuitCmd = 'QUIT'
	};

	void chooseLanguage();
	void showOptions(OptionsDialog::Page page);
	void restart();
};

MainMenuDialog::MainMenuDialog() : GUI::Dialog("LoLENMenu") {
	GUI::StaticTextWidget *title = new GUI::StaticTextWidget(this, "LoLENMenu.Title", Common::U32String("Lands of Lore"));
	title->setAlign(Graphics::kTextAlignCenter);

	GUI::StaticTextWidget *version = new GUI::StaticTextWidget(this, "LoLENMenu.Version", Common::U32String("LoLEN"),
		Common::U32String(), GUI::ThemeEngine::kFontStyleNormal);
	version->setAlign(Graphics::kTextAlignCenter);

	new GUI::ButtonWidget(this, "LoLENMenu.Resume", _("~R~esume"), Common::U32String(), kResumeCmd, 'P');

	GUI::ButtonWidget *language = new GUI::ButtonWidget(this, "LoLENMenu.Language", _("~L~anguage"), Common::U32String(), kLanguageCmd);
	language->setEnabled(s_languages.size() > 1);

	new GUI::ButtonWidget(this, "LoLENMenu.Sound", _("~S~ound"), Common::U32String(), kSoundCmd);
	new GUI::ButtonWidget(this, "LoLENMenu.Graphics", _("~D~isplay"), Common::U32String(), kGraphicsCmd);
	new GUI::ButtonWidget(this, "LoLENMenu.Controls", _("C~o~ntrols"), Common::U32String(), kControlsCmd);
	new GUI::ButtonWidget(this, "LoLENMenu.Credits", _("~C~redits"), Common::U32String(), kCreditsCmd);
	new GUI::ButtonWidget(this, "LoLENMenu.Quit", _("~Q~uit"), Common::U32String(), kQuitCmd);
}

void MainMenuDialog::handleCommand(GUI::CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kResumeCmd:
		close();
		break;
	case kLanguageCmd:
		chooseLanguage();
		break;
	case kSoundCmd:
		showOptions(OptionsDialog::kPageSound);
		break;
	case kGraphicsCmd:
		showOptions(OptionsDialog::kPageGraphics);
		break;
	case kControlsCmd:
		showOptions(OptionsDialog::kPageControls);
		break;
	case kCreditsCmd: {
		CreditsDialog credits;
		credits.runModal();
		break;
	}
	case kQuitCmd:
		close();
		Engine::quitGame();
		break;
	default:
		GUI::Dialog::handleCommand(sender, cmd, data);
		break;
	}
}

void MainMenuDialog::chooseLanguage() {
	const Common::String &gameDomain = ConfMan.getActiveDomainName();
	const Common::Language current = Common::parseLanguage(ConfMan.get("language", gameDomain));

	LanguageDialog dialog(current);
	if (dialog.runModal() <= 0)
		return;

	const Common::Language language = dialog.getLanguage();
	if (language == Common::UNK_LANG || language == current)
		return;

	if (!confirmRestart(_("The game restarts in the new language.\nThe progress since the last save is lost.")))
		return;

	ConfMan.set("language", Common::getLanguageCode(language), gameDomain);
	ConfMan.flushToDisk();
	restart();
}

void MainMenuDialog::showOptions(OptionsDialog::Page page) {
	OptionsDialog options(page);
	options.runModal();

	if (options.musicChanged() &&
	    confirmRestart(_("The new music settings are used once the game restarts.\n"
	                     "Restart now? The progress since the last save is lost.")))
		restart();
}

void MainMenuDialog::restart() {
	s_restartRequested = true;
	close();
	Engine::quitGame();
}

void MainMenuDialog::reflowLayout() {
	// The layout is defined here since the built-in theme does not know it.
	// The theme drops it when it is reloaded (e.g. when the window size changes).
	GUI::ThemeEval &eval = *g_gui.xmlEval();
	if (!eval.hasDialog(_name)) {
		const int lineHeight = eval.getVar("Globals.Line.Height");
		const int buttonHeight = eval.getVar("Globals.Button.Height");
		const int textWidth = scaled(210);
		const int buttonWidth = scaled(150);
		const int space = scaled(10);

		eval.addDialog(_name, "screen_center");
		eval.addLayout(GUI::ThemeLayout::kLayoutVertical, -1, GUI::ThemeLayout::kItemAlignCenter).addPadding(16, 16, 16, 16);
		eval.addWidget("Title", "", textWidth, lineHeight);
		eval.addWidget("Version", "", textWidth, lineHeight);
		eval.addSpace(space);
		eval.addWidget("Resume", "", buttonWidth, buttonHeight);
		eval.addSpace(space);
		eval.addWidget("Language", "", buttonWidth, buttonHeight);
		eval.addWidget("Sound", "", buttonWidth, buttonHeight);
		eval.addWidget("Graphics", "", buttonWidth, buttonHeight);
		eval.addWidget("Controls", "", buttonWidth, buttonHeight);
		eval.addSpace(space);
		eval.addWidget("Credits", "", buttonWidth, buttonHeight);
		eval.addSpace(space);
		eval.addWidget("Quit", "", buttonWidth, buttonHeight);
		eval.closeLayout().closeDialog();
	}

	GUI::Dialog::reflowLayout();
}

GUI::Dialog *createMainMenu(Engine *) {
	return new MainMenuDialog();
}

} // End of anonymous namespace

void installMenu(const Common::Array<Common::Language> &languages) {
	s_languages = languages;
	s_restartRequested = false;
	Engine::setMainMenuFactory(&createMainMenu);
}

void uninstallMenu() {
	Engine::setMainMenuFactory(nullptr);
	s_languages.clear();
}

bool takeRestartRequest() {
	const bool requested = s_restartRequested;
	s_restartRequested = false;
	return requested;
}

} // End of namespace LoLEN
