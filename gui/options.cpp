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

#include "gui/browser.h"
#include "gui/shaderbrowser-dialog.h"
#include "gui/themebrowser.h"
#include "gui/message.h"
#include "gui/gui-manager.h"
#include "gui/options.h"
#include "gui/widgets/popup.h"
#include "gui/widgets/tab.h"
#include "gui/ThemeEval.h"
#include "gui/textviewer.h"

#include "backends/keymapper/keymapper.h"
#include "backends/keymapper/remap-widget.h"

#include "common/fs.h"
#include "common/config-manager.h"
#include "common/gui_options.h"
#include "common/rendermode.h"
#include "common/rotationmode.h"
#include "common/savefile.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "common/translation.h"
#include "common/updates.h"
#include "common/util.h"
#include "common/text-to-speech.h"

#include "engines/achievements.h"

#include "audio/mididrv.h"
#include "audio/musicplugin.h"
#include "audio/mixer.h"
#include "audio/fmopl.h"
#include "widgets/scrollcontainer.h"
#include "widgets/edittext.h"

#ifdef USE_CLOUD
#include "backends/cloud/cloudmanager.h"
#include "gui/cloudconnectionwizard.h"
#include "gui/downloaddialog.h"
#endif

#ifdef USE_HTTP
#include "gui/downloadpacksdialog.h"
#endif

#ifdef USE_SDL_NET
#include "backends/networking/sdl_net/localwebserver.h"
#endif

#include "graphics/paletteman.h"
#include "graphics/pm5544.h"
#include "graphics/renderer.h"
#include "graphics/scalerplugin.h"

namespace GUI {

enum {
	kMidiGainChanged		= 'mgch',
	kMusicVolumeChanged		= 'muvc',
	kSfxVolumeChanged		= 'sfvc',
	kMuteAllChanged			= 'mute',
	kSubtitleToggle			= 'sttg',
	kSubtitleSpeedChanged	= 'stsc',
	kSpeechVolumeChanged	= 'vcvc',
	kChooseSoundFontCmd		= 'chsf',
	kClearSoundFontCmd      = 'clsf',
	kChooseSaveDirCmd		= 'chos',
	kSavePathClearCmd		= 'clsp',
	kChooseThemeDirCmd		= 'chth',
	kChooseIconDirCmd		= 'chic',
	kChooseDLCDirCmd		= 'chdc',
	kThemePathClearCmd		= 'clth',
	kBrowserPathClearCmd	= 'clbr',
	kIconPathClearCmd		= 'clic',
	kDLCPathClearCmd		= 'cldc',
	kChooseExtraDirCmd		= 'chex',
	kExtraPathClearCmd		= 'clex',
	kChoosePluginsDirCmd	= 'chpl',
	kPluginsPathClearCmd	= 'clpl',
	kChooseThemeCmd			= 'chtf',
	kUpdateIconsCmd			= 'upic',
	kUpdateShadersCmd		= 'upsh',
	kChooseShaderCmd        = 'chsh',
	kClearShaderCmd         = 'clsh',
	kUpdatesCheckCmd		= 'updc',
	kKbdMouseSpeedChanged	= 'kmsc',
	kJoystickDeadzoneChanged= 'jodc',
	kGraphicsTabContainerReflowCmd = 'gtcr',
	kScalerPopUpCmd			= 'scPU',
	kFullscreenToggled		= 'oful',
	kRandomSeedClearCmd     = 'rndc',
	kViewLogCmd             = 'vwlg',
};

enum {
	kSubtitlesSpeech,
	kSubtitlesSubs,
	kSubtitlesBoth,
};

#ifdef USE_FLUIDSYNTH
enum {
	kFluidSynthSettingsCmd  = 'flst'
};
#endif

enum {
	kStoragePopUpCmd = 'sPup',
	kSyncSavesStorageCmd = 'ssst',
	kDownloadStorageCmd = 'dlst',
	kRunServerCmd = 'rnsv',
	kCloudTabContainerReflowCmd = 'ctcr',
	kOpenCloudConnectionWizardCmd = 'OpCW',
	kServerPortClearCmd = 'spcl',
	kChooseRootDirCmd = 'chrp',
	kRootPathClearCmd = 'clrp',
	kDisconnectStorageCmd = 'DcSt',
	kEnableStorageCmd = 'EnSt'
};

enum {
	kApplyCmd = 'appl'
};

static const char *const savePeriodLabels[] = { _s("Never"), _s("Every 5 mins"), _s("Every 10 mins"), _s("Every 15 mins"), _s("Every 30 mins"), nullptr };
static const int savePeriodValues[] = { 0, 5 * 60, 10 * 60, 15 * 60, 30 * 60, -1 };

static const char *const guiBaseLabels[] = {
	// I18N: Extremely large GUI scale
	_s("200% - Extremely large"),
	// I18N: Very very large GUI scale
	_s("175% - Very large"),
	// I18N: Very large GUI scale
	_s("150% - Larger"),
	// I18N: Large GUI scale
	_s("125% - Large"),
	// I18N: Medium GUI scale
	_s("100% - Medium"),
	// I18N: Small GUI scale
	_s("75% - Small"),
	// I18N: Smaller GUI scale
	_s("50% - Smaller"),
	// I18N: Smallest GUI scale
	_s("25% - Smallest"),
	nullptr
};
static const int guiBaseValues[] = { 200, 175, 150, 125, 100, 75, 50, 25, -1 };

// The keyboard mouse speed values range from 0 to 7 and correspond to speeds shown in the label
// "10" (value 3) is the default speed corresponding to the speed before introduction of this control
static const char *const kbdMouseSpeedLabels[] = { "3", "5", "8", "10", "13", "15", "18", "20", nullptr };

OptionsDialog::OptionsDialog(const Common::String &domain, int x, int y, int w, int h)
	: Dialog(x, y, w, h), _domain(domain), _graphicsTabId(-1), _midiTabId(-1), _pathsContainer(nullptr), _tabWidget(nullptr) {
	init();
}

OptionsDialog::OptionsDialog(const Common::String &domain, const Common::String &name)
	: Dialog(name), _domain(domain), _graphicsTabId(-1), _midiTabId(-1), _pathsContainer(nullptr), _tabWidget(nullptr) {
	init();
}

OptionsDialog::~OptionsDialog() {
	delete _subToggleGroup;
}

void OptionsDialog::init() {
	_enableControlSettings = false;
	_touchpadCheckbox = nullptr;
	_kbdMouseSpeedDesc = nullptr;
	_kbdMouseSpeedSlider = nullptr;
	_kbdMouseSpeedLabel = nullptr;
	_joystickDeadzoneDesc = nullptr;
	_joystickDeadzoneSlider = nullptr;
	_joystickDeadzoneLabel = nullptr;
	_keymapperWidget = nullptr;
	_backendOptions = nullptr;
	_enableGraphicSettings = false;
	_gfxPopUp = nullptr;
	_gfxPopUpDesc = nullptr;
	_renderModePopUp = nullptr;
	_renderModePopUpDesc = nullptr;
	_rotationModePopUp = nullptr;
	_rotationModePopUpDesc = nullptr;
	_stretchPopUp = nullptr;
	_stretchPopUpDesc = nullptr;
	_scalerPopUp = nullptr;
	_scalerPopUpDesc = nullptr;
	_scaleFactorPopUp = nullptr;
	_fullscreenCheckbox = nullptr;
	_filteringCheckbox = nullptr;
	_aspectCheckbox = nullptr;
	_shader = nullptr;
	_shaderButton = nullptr;
	_shaderClearButton = nullptr;
	_vsyncCheckbox = nullptr;
	_rendererTypePopUpDesc = nullptr;
	_rendererTypePopUp = nullptr;
	_antiAliasPopUpDesc = nullptr;
	_antiAliasPopUp = nullptr;
	_enableAudioSettings = false;
	_midiPopUp = nullptr;
	_midiPopUpDesc = nullptr;
	_oplPopUp = nullptr;
	_oplPopUpDesc = nullptr;
	_enableMIDISettings = false;
	_gmDevicePopUp = nullptr;
	_gmDevicePopUpDesc = nullptr;
	_soundFont = nullptr;
	_soundFontButton = nullptr;
	_soundFontClearButton = nullptr;
	_multiMidiCheckbox = nullptr;
	_midiGainDesc = nullptr;
	_midiGainSlider = nullptr;
	_midiGainLabel = nullptr;
	_enableMT32Settings = false;
	_mt32Checkbox = nullptr;
	_mt32DevicePopUp = nullptr;
	_mt32DevicePopUpDesc = nullptr;
	_enableGSCheckbox = nullptr;
	_enableVolumeSettings = false;
	_musicVolumeDesc = nullptr;
	_musicVolumeSlider = nullptr;
	_musicVolumeLabel = nullptr;
	_sfxVolumeDesc = nullptr;
	_sfxVolumeSlider = nullptr;
	_sfxVolumeLabel = nullptr;
	_speechVolumeDesc = nullptr;
	_speechVolumeSlider = nullptr;
	_speechVolumeLabel = nullptr;
	_muteCheckbox = nullptr;
	_enableSubtitleSettings = false;
	_enableSubtitleToggle = false;
	_subToggleDesc = nullptr;
	_subToggleGroup = nullptr;
	_subToggleSubOnly = nullptr;
	_subToggleSpeechOnly = nullptr;
	_subToggleSubBoth = nullptr;
	_subSpeedDesc = nullptr;
	_subSpeedSlider = nullptr;
	_subSpeedLabel = nullptr;

	// Retrieve game GUI options
	_guioptions.clear();
	if (ConfMan.hasKey("guioptions", _domain)) {
		_guioptionsString = ConfMan.get("guioptions", _domain);
		_guioptions = parseGameGUIOptions(_guioptionsString);
	}
}

void OptionsDialog::build() {
	// Retrieve game GUI options
	_guioptions.clear();
	if (ConfMan.hasKey("guioptions", _domain)) {
		_guioptionsString = ConfMan.get("guioptions", _domain);
		_guioptions = parseGameGUIOptions(_guioptionsString);
	}

	// Control options
	if (g_system->hasFeature(OSystem::kFeatureTouchpadMode)) {
		if (ConfMan.hasKey("touchpad_mouse_mode", _domain)) {
			bool touchpadState =  g_system->getFeatureState(OSystem::kFeatureTouchpadMode);
			if (_touchpadCheckbox != nullptr)
				_touchpadCheckbox->setState(touchpadState);
		}
	}
	if (g_system->hasFeature(OSystem::kFeatureKbdMouseSpeed)) {
		int value = ConfMan.getInt("kbdmouse_speed", _domain);
		if (_kbdMouseSpeedSlider && value < ARRAYSIZE(kbdMouseSpeedLabels) - 1 && value >= 0) {
			_kbdMouseSpeedSlider->setValue(value);
			_kbdMouseSpeedLabel->setLabel(_(kbdMouseSpeedLabels[value]));
		}
	}
	if (g_system->hasFeature(OSystem::kFeatureJoystickDeadzone)) {
		int value = ConfMan.getInt("joystick_deadzone", _domain);
		if (_joystickDeadzoneSlider != nullptr) {
			_joystickDeadzoneSlider->setValue(value);
			_joystickDeadzoneLabel->setValue(value);
		}
	}

	// Keymapper options
	if (_keymapperWidget) {
		_keymapperWidget->load();
	}

	// Backend options
	if (_backendOptions) {
		_backendOptions->load();
	}

	// Graphic options
	if (_fullscreenCheckbox) {
		_gfxPopUp->setSelected(0);

		if (ConfMan.hasKey("gfx_mode", _domain)) {
			const OSystem::GraphicsMode *gm = g_system->getSupportedGraphicsModes();
			Common::String gfxMode(ConfMan.get("gfx_mode", _domain));
			int gfxCount = 1;
			while (gm->name) {
				gfxCount++;

				if (scumm_stricmp(gm->name, gfxMode.c_str()) == 0)
					_gfxPopUp->setSelected(gfxCount);

				gm++;
			}
		}

		_renderModePopUp->setSelected(0);

		if (ConfMan.hasKey("render_mode", _domain)) {
			const Common::RenderModeDescription *p = Common::g_renderModes;
			const Common::RenderMode renderMode = Common::parseRenderMode(ConfMan.get("render_mode", _domain));
			int sel = 0;
			for (; p->code; ++p) {
				if (renderMode == p->id)
					sel = p->id;
			}
			_renderModePopUp->setSelectedTag(sel);
		}

		if (g_system->hasFeature(OSystem::kFeatureRotationMode)) {
			_rotationModePopUp->setSelected(0);

			if (ConfMan.hasKey("rotation_mode", _domain)) {
				const Common::RotationModeDescription *p = Common::g_rotationModes;
				const Common::RotationMode rotationMode = Common::parseRotationMode(ConfMan.getInt("rotation_mode", _domain));
				int sel = 0;
				for (; p->description; ++p) {
					if (rotationMode == p->id)
						sel = p->id;
				}
				_rotationModePopUp->setSelectedTag(sel);
			}
		}

		// Fullscreen setting
		if (g_system->hasFeature(OSystem::kFeatureFullscreenMode)) {
			_fullscreenCheckbox->setState(ConfMan.getBool("fullscreen", _domain));
			if (ConfMan.isKeyTemporary("fullscreen"))
				_fullscreenCheckbox->setOverride(true);
		} else {
			_fullscreenCheckbox->setState(true);
			_fullscreenCheckbox->setEnabled(false);
		}

		// Filtering setting
		if (g_system->hasFeature(OSystem::kFeatureFilteringMode)) {
			_filteringCheckbox->setState(ConfMan.getBool("filtering", _domain));
			if (ConfMan.isKeyTemporary("filtering"))
				_filteringCheckbox->setOverride(true);
		}

		// Aspect ratio setting
		if (g_system->hasFeature(OSystem::kFeatureAspectRatioCorrection)) {
			if (_guioptions.contains(GUIO_NOASPECT)) {
				_aspectCheckbox->setState(false);
				_aspectCheckbox->setEnabled(false);
			} else {
				_aspectCheckbox->setEnabled(true);
				_aspectCheckbox->setState(ConfMan.getBool("aspect_ratio", _domain));
			}
		}

		if (g_system->hasFeature(OSystem::kFeatureVSync)) {
			_vsyncCheckbox->setState(ConfMan.getBool("vsync", _domain));
			if (ConfMan.isKeyTemporary("vsync"))
				_vsyncCheckbox->setOverride(true);
		}
	}

	if (_stretchPopUp) {
		if (g_system->hasFeature(OSystem::kFeatureStretchMode)) {
			_stretchPopUp->setSelected(0);

			if (ConfMan.hasKey("stretch_mode", _domain)) {
				const OSystem::GraphicsMode *sm = g_system->getSupportedStretchModes();
				Common::String stretchMode(ConfMan.get("stretch_mode", _domain));
				int stretchCount = 1;
				while (sm->name) {
					stretchCount++;
					if (scumm_stricmp(sm->name, stretchMode.c_str()) == 0)
						_stretchPopUp->setSelected(stretchCount);
					sm++;
				}
			}
		} else {
			_stretchPopUpDesc->setVisible(false);
			_stretchPopUp->setVisible(false);
		}
	}

	if (_scalerPopUp) {
		if (g_system->hasFeature(OSystem::kFeatureScalers)) {
			_scalerPopUp->setSelected(0);
			_scaleFactorPopUp->setSelected(0);

			if (ConfMan.hasKey("scaler", _domain)) {
				const PluginList &scalerPlugins = ScalerMan.getPlugins();
				Common::String scaler(ConfMan.get("scaler", _domain));

				for (uint scalerIndex = 0; scalerIndex < scalerPlugins.size(); scalerIndex++) {
					if (scumm_stricmp(scalerPlugins[scalerIndex]->get<ScalerPluginObject>().getName(), scaler.c_str()) != 0)
						continue;

					_scalerPopUp->setSelectedTag(scalerIndex);
					updateScaleFactors(scalerIndex);

					if (ConfMan.hasKey("scale_factor", _domain)) {
						int scaleFactor = ConfMan.getInt("scale_factor", _domain);
						if (scalerPlugins[scalerIndex]->get<ScalerPluginObject>().hasFactor(scaleFactor))
							_scaleFactorPopUp->setSelectedTag(scaleFactor);
					}

					break;
				}

			}
		} else {
			_scalerPopUpDesc->setVisible(false);
			_scalerPopUp->setVisible(false);
			_scaleFactorPopUp->setVisible(false);
		}
	}

	// Shader options
	if (_shader) {
		enableShaderControls(g_system->hasFeature(OSystem::kFeatureShaders));

		if (g_system->hasFeature(OSystem::kFeatureShaders)) {
			Common::Path shader(ConfMan.getPath("shader", _domain));
			if (ConfMan.isKeyTemporary("shader")) {
				_shader->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
			}
			if (shader == Common::Path("default", Common::Path::kNoSeparator)) {
				shader.clear();
			}
			if (shader.empty() || !ConfMan.hasKey("shader", _domain)) {
				_shaderClearButton->setEnabled(false);
			} else {
				_shaderClearButton->setEnabled(true);
			}
			_shader->setLabel(shader);
		}
	}

	if (_rendererTypePopUp) {
		_rendererTypePopUp->setEnabled(true);
		_rendererTypePopUp->setSelectedTag(Graphics::Renderer::parseTypeCode(ConfMan.get("renderer", _domain)));
	}

	if (_antiAliasPopUp) {
		_antiAliasPopUp->setEnabled(true);
		if (ConfMan.hasKey("antialiasing", _domain)) {
			_antiAliasPopUp->setSelectedTag(ConfMan.getInt("antialiasing", _domain));
		} else {
			_antiAliasPopUp->setSelectedTag(uint32(-1));
		}
	}

	// Audio options
	if (!loadMusicDeviceSetting(_midiPopUp, "music_driver"))
		_midiPopUp->setSelected(0);

	if (_oplPopUp) {
		OPL::Config::DriverId id = OPL::Config::parse(ConfMan.get("opl_driver", _domain));
		if (id < 0)
			id = OPL::Config::parse("auto");
		_oplPopUp->setSelectedTag(id);
	}

	if (_multiMidiCheckbox) {
		if (!loadMusicDeviceSetting(_gmDevicePopUp, "gm_device"))
			_gmDevicePopUp->setSelected(1);

		// Multi midi setting
		_multiMidiCheckbox->setState(ConfMan.getBool("multi_midi", _domain));
		if (ConfMan.isKeyTemporary("multi_midi"))
			_multiMidiCheckbox->setOverride(true);

		Common::Path soundFont(ConfMan.getPath("soundfont", _domain));
		if (ConfMan.isKeyTemporary("soundfont")) {
			_soundFont->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
		}
		if (soundFont.empty()) {
			_soundFontClearButton->setEnabled(false);
		} else if (!ConfMan.hasKey("soundfont", _domain)) {
			soundFont.clear();
			_soundFontClearButton->setEnabled(false);
		} else {
			_soundFontClearButton->setEnabled(true);
		}
		_soundFont->setLabel(soundFont);

		// MIDI gain setting
		_midiGainSlider->setValue(ConfMan.getInt("midi_gain", _domain));
		_midiGainLabel->setLabel(Common::String::format("%.2f", (double)_midiGainSlider->getValue() / 100.0));
		if (ConfMan.isKeyTemporary("midi_gain"))
			_midiGainDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
	}

	// MT-32 options
	if (_mt32DevicePopUp) {
		if (!loadMusicDeviceSetting(_mt32DevicePopUp, "mt32_device"))
			_mt32DevicePopUp->setSelected(0);

		// Native mt32 setting
		_mt32Checkbox->setState(ConfMan.getBool("native_mt32", _domain));
		if (ConfMan.isKeyTemporary("native_mt32"))
			_mt32Checkbox->setOverride(true);

		// GS extensions setting
		_enableGSCheckbox->setState(ConfMan.getBool("enable_gs", _domain));
		if (ConfMan.isKeyTemporary("enable_gs"))
			_enableGSCheckbox->setOverride(true);
	}

	// Volume options
	if (_musicVolumeSlider) {
		int vol;

		vol = ConfMan.getInt("music_volume", _domain);
		_musicVolumeSlider->setValue(vol);
		_musicVolumeLabel->setValue(vol);

		vol = ConfMan.getInt("sfx_volume", _domain);
		_sfxVolumeSlider->setValue(vol);
		_sfxVolumeLabel->setValue(vol);

		vol = ConfMan.getInt("speech_volume", _domain);
		_speechVolumeSlider->setValue(vol);
		_speechVolumeLabel->setValue(vol);

		bool val = false;
		if (ConfMan.hasKey("mute", _domain)) {
			val = ConfMan.getBool("mute", _domain);
		} else {
			ConfMan.setBool("mute", false);
		}
		_muteCheckbox->setState(val);
	}

	// Subtitle options
	if (_subToggleGroup) {
		int speed;
		int sliderMaxValue = _subSpeedSlider->getMaxValue();

		int subMode = getSubtitleMode(ConfMan.getBool("subtitles", _domain), ConfMan.getBool("speech_mute", _domain));
		_subToggleGroup->setValue(subMode);

		// Engines that reuse the subtitle speed widget set their own max value.
		// Scale the config value accordingly (see addSubtitleControls)
		speed = (ConfMan.getInt("talkspeed", _domain) * sliderMaxValue + 255 / 2) / 255;
		_subSpeedSlider->setValue(speed);
		_subSpeedLabel->setValue(speed);
	}
}

void OptionsDialog::clean() {
	delete _subToggleGroup;
	while (_firstWidget) {
		Widget* w = _firstWidget;
		removeWidget(w);
		// This is called from rebuild() which may result from handleCommand being called by
		// a child widget sendCommand call. In such a case sendCommand is still being executed
		// so we should not delete yet the child widget. Thus delay the deletion.
		g_gui.addToTrash(w, this);
	}
	init();
}

void OptionsDialog::rebuild() {
	int currentTab = _tabWidget->getActiveTab();
	clean();
	build();
	reflowLayout();
	_tabWidget->setActiveTab(currentTab);
	setDefaultFocusedWidget();
}

void OptionsDialog::open() {
	build();

	Dialog::open();

	// Reset result value
	setResult(0);
}

void OptionsDialog::apply() {
	bool graphicsModeChanged = false;

	// Graphic options
	if (_fullscreenCheckbox) {
		if (_enableGraphicSettings) {
			if (g_system->hasFeature(OSystem::kFeatureFilteringMode))
				if (ConfMan.getBool("filtering", _domain) != _filteringCheckbox->getState()) {
					graphicsModeChanged = true;
					ConfMan.setBool("filtering", _filteringCheckbox->getState(), _domain);
					_filteringCheckbox->setOverride(false);
				}
			if (ConfMan.getBool("fullscreen", _domain) != _fullscreenCheckbox->getState()) {
				graphicsModeChanged = true;
				ConfMan.setBool("fullscreen", _fullscreenCheckbox->getState(), _domain);
				_fullscreenCheckbox->setOverride(false);
			}
			if (g_system->hasFeature(OSystem::kFeatureAspectRatioCorrection)) {
				if (ConfMan.getBool("aspect_ratio", _domain) != _aspectCheckbox->getState())
					graphicsModeChanged = true;
			}
			if (g_system->hasFeature(OSystem::kFeatureVSync)) {
				if (ConfMan.getBool("vsync", _domain) != _vsyncCheckbox->getState()) {
					graphicsModeChanged = true;
					ConfMan.setBool("vsync", _vsyncCheckbox->getState(), _domain);
					_vsyncCheckbox->setOverride(false);
				}
			}

			if (g_system->hasFeature(OSystem::kFeatureAspectRatioCorrection)) {
				ConfMan.setBool("aspect_ratio", _aspectCheckbox->getState(), _domain);
			}

			bool isSet = false;

			if ((int32)_gfxPopUp->getSelectedTag() >= 0) {
				const OSystem::GraphicsMode *gm = g_system->getSupportedGraphicsModes();

				while (gm->name) {
					if (gm->id == (int)_gfxPopUp->getSelectedTag()) {
						if (ConfMan.get("gfx_mode", _domain) != gm->name) {
							_gfxPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
							graphicsModeChanged = true;
							ConfMan.set("gfx_mode", gm->name, _domain);
						}
						isSet = true;
						break;
					}
					gm++;
				}
			}
			if (!isSet) {
				_gfxPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
				ConfMan.removeKey("gfx_mode", _domain);
				if (g_system->getGraphicsMode() != g_system->getDefaultGraphicsMode())
					graphicsModeChanged = true;
			}

			if ((int32)_renderModePopUp->getSelectedTag() >= 0) {
				Common::String renderModeCode = Common::getRenderModeCode((Common::RenderMode)_renderModePopUp->getSelectedTag());
				if (_renderModePopUp->getSelectedTag() == 0 || ConfMan.get("render_mode", _domain) != renderModeCode) {
					ConfMan.set("render_mode", renderModeCode, _domain);
					_renderModePopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
				}
			}

			if (g_system->hasFeature(OSystem::kFeatureRotationMode)) {
				if ((int32)_rotationModePopUp->getSelectedTag() >= 0) {
					int rotationModeCode = ((Common::RotationMode)_rotationModePopUp->getSelectedTag());
					if (!ConfMan.hasKey("rotation_mode", _domain) ||
						ConfMan.getInt("rotation_mode", _domain) != rotationModeCode) {
						ConfMan.setInt("rotation_mode", rotationModeCode, _domain);
						_rotationModePopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
						graphicsModeChanged = true;
					}
				} else {
					// default selected
					if (ConfMan.hasKey("rotation_mode", _domain)) {
						ConfMan.removeKey("rotation_mode", _domain);
						graphicsModeChanged = true;
					}
				}
			}

			if (g_system->hasFeature(OSystem::kFeatureStretchMode)) {
				isSet = false;
				if ((int32)_stretchPopUp->getSelectedTag() >= 0) {
					const OSystem::GraphicsMode *sm = g_system->getSupportedStretchModes();

					while (sm->name) {
						if (sm->id == (int)_stretchPopUp->getSelectedTag()) {
							if (ConfMan.get("stretch_mode", _domain) != sm->name) {
								graphicsModeChanged = true;
								ConfMan.set("stretch_mode", sm->name, _domain);
								_stretchPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
							}
							isSet = true;
							break;
						}
						sm++;
					}
				}
				if (!isSet) {
					_stretchPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
					ConfMan.removeKey("stretch_mode", _domain);
					if (g_system->getStretchMode() != g_system->getDefaultStretchMode())
						graphicsModeChanged = true;
				}
			}

			if (g_system->hasFeature(OSystem::kFeatureScalers)) {
				isSet = false;
				const PluginList &scalerPlugins = ScalerMan.getPlugins();
				if ((int32)_scalerPopUp->getSelectedTag() >= 0) {
					const char *name = scalerPlugins[_scalerPopUp->getSelectedTag()]->get<ScalerPluginObject>().getName();
					if (ConfMan.get("scaler", _domain) != name) {
						graphicsModeChanged = true;
						ConfMan.set("scaler", name, _domain);
						_scalerPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
					}

					int factor = _scaleFactorPopUp->getSelectedTag();
					if (ConfMan.getInt("scale_factor", _domain) != factor) {
						ConfMan.setInt("scale_factor", factor, _domain);
						graphicsModeChanged = true;
						_scalerPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
					}
					isSet = true;
				}
				if (!isSet) {
					ConfMan.removeKey("scaler", _domain);
					ConfMan.removeKey("scale_factor", _domain);
					_scalerPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);

					uint defaultScaler = g_system->getDefaultScaler();
					uint defaultScaleFactor = g_system->getDefaultScaleFactor();
					if (g_system->getScaler() != defaultScaler)
						graphicsModeChanged = true;
					else if (g_system->getScaleFactor() != defaultScaleFactor)
						graphicsModeChanged = true;
				}
			}

			if (_rendererTypePopUp) {
				if (_rendererTypePopUp->getSelectedTag() > 0) {
					Graphics::RendererType selected = (Graphics::RendererType) _rendererTypePopUp->getSelectedTag();
					ConfMan.set("renderer", Graphics::Renderer::getTypeCode(selected), _domain);
				} else {
					ConfMan.removeKey("renderer", _domain);
				}
			}

			if (_antiAliasPopUp) {
				if (_antiAliasPopUp->getSelectedTag() != (uint32)-1) {
					uint level = _antiAliasPopUp->getSelectedTag();
					ConfMan.setInt("antialiasing", level, _domain);
				} else {
					ConfMan.removeKey("antialiasing", _domain);
				}
			}

		} else {
			ConfMan.removeKey("fullscreen", _domain);
			ConfMan.removeKey("filtering", _domain);
			ConfMan.removeKey("aspect_ratio", _domain);
			ConfMan.removeKey("gfx_mode", _domain);
			ConfMan.removeKey("stretch_mode", _domain);
			ConfMan.removeKey("scaler", _domain);
			ConfMan.removeKey("scale_factor", _domain);
			ConfMan.removeKey("render_mode", _domain);
			ConfMan.removeKey("rotation_mode", _domain);
			ConfMan.removeKey("renderer", _domain);
			ConfMan.removeKey("antialiasing", _domain);
			ConfMan.removeKey("vsync", _domain);
		}
	}

	Common::Path previousShader;

	// Shader options
	if (g_system->hasFeature(OSystem::kFeatureShaders) && _shader) {
		Common::Path shaderConf;
		if (ConfMan.hasKey("shader", _domain)) {
			shaderConf = ConfMan.getPath("shader", _domain);
			if (!shaderConf.empty()) {
				previousShader = ConfMan.getPath("shader", _domain);
			}
		}

		Common::Path shader(_shader->getLabel());
		if (shader.empty()) {
			shader = Common::Path("default", Common::Path::kNoSeparator);
		}

		if (!ConfMan.hasKey("shader", _domain) || shader != shaderConf)
			graphicsModeChanged = true;

		if (_enableGraphicSettings)
			ConfMan.setPath("shader", shader, _domain);
		else
			ConfMan.removeKey("shader", _domain);

		_shader->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
	}

	// Setup graphics again if needed
	if (_domain == Common::ConfigManager::kApplicationDomain && graphicsModeChanged) {
		g_system->beginGFXTransaction();
		g_system->setGraphicsMode(ConfMan.get("gfx_mode", _domain).c_str());
		g_system->setStretchMode(ConfMan.get("stretch_mode", _domain).c_str());
		g_system->setScaler(ConfMan.get("scaler", _domain).c_str(), ConfMan.getInt("scale_factor", _domain));
		g_system->setRotationMode(ConfMan.getInt("rotation_mode", _domain));
		g_system->setShader(ConfMan.getPath("shader", _domain));

		if (ConfMan.hasKey("aspect_ratio"))
			g_system->setFeatureState(OSystem::kFeatureAspectRatioCorrection, ConfMan.getBool("aspect_ratio", _domain));
		if (ConfMan.hasKey("fullscreen"))
			g_system->setFeatureState(OSystem::kFeatureFullscreenMode, ConfMan.getBool("fullscreen", _domain));
		if (ConfMan.hasKey("filtering"))
			g_system->setFeatureState(OSystem::kFeatureFilteringMode, ConfMan.getBool("filtering", _domain));
		if (ConfMan.hasKey("vsync"))
			g_system->setFeatureState(OSystem::kFeatureVSync, ConfMan.getBool("vsync", _domain));

		OSystem::TransactionError gfxError = g_system->endGFXTransaction();

		// Since this might change the screen resolution we need to give
		// the GUI a chance to update it's internal state. Otherwise we might
		// get a crash when the GUI tries to grab the overlay.
		//
		// This fixes bug #5703 "Switching from HQ2x->HQ3x crashes ScummVM"
		//
		// It is important that this is called *before* any of the current
		// dialog's widgets are destroyed (for example before
		// Dialog::close) is called, to prevent crashes caused by invalid
		// widgets being referenced or similar errors.
		g_gui.checkScreenChange();

		if (gfxError != OSystem::kTransactionSuccess) {
			// Revert ConfMan to what OSystem is using.
			Common::U32String message = _("Failed to apply some of the graphic options changes:");

			if (gfxError & OSystem::kTransactionModeSwitchFailed) {
				const OSystem::GraphicsMode *gm = g_system->getSupportedGraphicsModes();
				while (gm->name) {
					if (gm->id == g_system->getGraphicsMode()) {
						ConfMan.set("gfx_mode", gm->name, _domain);
						break;
					}
					gm++;
				}
				message += Common::U32String("\n");
				message += _("the video mode could not be changed");
			}

			if (gfxError & OSystem::kTransactionStretchModeSwitchFailed) {
				const OSystem::GraphicsMode *sm = g_system->getSupportedStretchModes();
				while (sm->name) {
					if (sm->id == g_system->getStretchMode()) {
						ConfMan.set("stretch_mode", sm->name, _domain);
						break;
					}
					sm++;
				}
				message += Common::U32String("\n");
				message += _("the stretch mode could not be changed");
			}

			if (gfxError & OSystem::kTransactionAspectRatioFailed) {
				ConfMan.setBool("aspect_ratio", g_system->getFeatureState(OSystem::kFeatureAspectRatioCorrection), _domain);
				message += Common::U32String("\n");
				message += _("the aspect ratio setting could not be changed");
			}

			if (gfxError & OSystem::kTransactionFullscreenFailed) {
				ConfMan.setBool("fullscreen", g_system->getFeatureState(OSystem::kFeatureFullscreenMode), _domain);
				message += Common::U32String("\n");
				message += _("the fullscreen setting could not be changed");
			}

			if (gfxError & OSystem::kTransactionFilteringFailed) {
				ConfMan.setBool("filtering", g_system->getFeatureState(OSystem::kFeatureFilteringMode), _domain);
				message += Common::U32String("\n");
				message += _("the filtering setting could not be changed");
			}

			if (gfxError & OSystem::kTransactionVSyncFailed) {
				ConfMan.setBool("vsync", g_system->getFeatureState(OSystem::kFeatureVSync), _domain);
				message += Common::U32String("\n");
				message += _("the vsync setting could not be changed");
			}

			if (gfxError & OSystem::kTransactionShaderChangeFailed) {
				ConfMan.setPath("shader", previousShader, _domain);
				if (previousShader.empty() || previousShader == Common::Path("default", Common::Path::kNoSeparator)) {
					_shader->setLabel(Common::Path());
					_shaderClearButton->setEnabled(false);
				} else {
					_shader->setLabel(previousShader);
					_shaderClearButton->setEnabled(true);
				}

				message += Common::U32String("\n");
				message += _("the shader could not be changed");
			}

			// And display the error
			GUI::MessageDialog dialog(message);
			dialog.runModal();
		} else {
			// Successful transaction. Check if we need to show test screen
			Common::Path shader;
			if (ConfMan.hasKey("shader", _domain))
				shader = ConfMan.getPath("shader", _domain);

			// If shader was changed, show the test dialog
			if (previousShader != shader && !shader.empty() &&
					shader != Common::Path("default", Common::Path::kNoSeparator)) {
				if (!testGraphicsSettings()) {
					ConfMan.setPath("shader", previousShader, _domain);
					if (previousShader.empty() || previousShader == Common::Path("default", Common::Path::kNoSeparator)) {
						_shader->setLabel(Common::Path());
						_shaderClearButton->setEnabled(false);
					} else {
						_shader->setLabel(previousShader);
						_shaderClearButton->setEnabled(true);
					}

					g_system->beginGFXTransaction();
					g_system->setShader(ConfMan.getPath("shader", _domain));
					g_system->endGFXTransaction();
				}
			}
		}
	}

	if (_keymapperWidget) {
		bool changes = _keymapperWidget->save();
		if (changes) {
			Common::Keymapper *keymapper = g_system->getEventManager()->getKeymapper();
			keymapper->reloadAllMappings();
		}
	}

	if (_backendOptions) {
		bool changes = _backendOptions->save();
		if (changes && _domain == Common::ConfigManager::kApplicationDomain)
			g_system->applyBackendSettings();
	}

	// Control options
	if (_enableControlSettings) {
		if (g_system->hasFeature(OSystem::kFeatureTouchpadMode)) {
			if (ConfMan.getBool("touchpad_mouse_mode", _domain) != _touchpadCheckbox->getState()) {
				g_system->setFeatureState(OSystem::kFeatureTouchpadMode, _touchpadCheckbox->getState());
			}
		}
		if (g_system->hasFeature(OSystem::kFeatureKbdMouseSpeed)) {
			if (ConfMan.getInt("kbdmouse_speed", _domain) != _kbdMouseSpeedSlider->getValue()) {
				ConfMan.setInt("kbdmouse_speed", _kbdMouseSpeedSlider->getValue(), _domain);
			}
		}
		if (g_system->hasFeature(OSystem::kFeatureJoystickDeadzone)) {
			if (ConfMan.getInt("joystick_deadzone", _domain) != _joystickDeadzoneSlider->getValue()) {
				ConfMan.setInt("joystick_deadzone", _joystickDeadzoneSlider->getValue(), _domain);
			}
		}
	}

	// Volume options
	if (_musicVolumeSlider) {
		if (_enableVolumeSettings) {
			ConfMan.setInt("music_volume", _musicVolumeSlider->getValue(), _domain);
			ConfMan.setInt("sfx_volume", _sfxVolumeSlider->getValue(), _domain);
			ConfMan.setInt("speech_volume", _speechVolumeSlider->getValue(), _domain);
			ConfMan.setBool("mute", _muteCheckbox->getState(), _domain);
		} else {
			ConfMan.removeKey("music_volume", _domain);
			ConfMan.removeKey("sfx_volume", _domain);
			ConfMan.removeKey("speech_volume", _domain);
			ConfMan.removeKey("mute", _domain);
		}
	}

	// Audio options
	if (_midiPopUp) {
		if (_enableAudioSettings) {
			saveMusicDeviceSetting(_midiPopUp, "music_driver");
		} else {
			ConfMan.removeKey("music_driver", _domain);
		}
	}

	if (_oplPopUp) {
		if (_enableAudioSettings) {
			const OPL::Config::EmulatorDescription *ed = OPL::Config::findDriver(_oplPopUp->getSelectedTag());

			if ((ed && ConfMan.get("opl_driver", _domain) != ed->name) || !ed) {
				_oplPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
				if (ed)
					ConfMan.set("opl_driver", ed->name, _domain);
				else
					ConfMan.removeKey("opl_driver", _domain);
			}
		} else {
			_oplPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
			ConfMan.removeKey("opl_driver", _domain);
		}
	}

	// MIDI options
	if (_multiMidiCheckbox) {
		if (_enableMIDISettings) {
			saveMusicDeviceSetting(_gmDevicePopUp, "gm_device");

			if (_multiMidiCheckbox->getState() != ConfMan.getBool("multi_midi", _domain)) {
				ConfMan.setBool("multi_midi", _multiMidiCheckbox->getState(), _domain);
				_multiMidiCheckbox->setOverride(false);
			}
			if (_midiGainSlider->getValue() != ConfMan.getInt("midi_gain", _domain)) {
				ConfMan.setInt("midi_gain", _midiGainSlider->getValue(), _domain);
				_midiGainDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
			}

			Common::Path soundFont(_soundFont->getLabel());
			if (soundFont != ConfMan.getPath("soundfont", _domain)) {
				_soundFont->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
				if (soundFont.empty())
					ConfMan.removeKey("soundfont", _domain);
				else
					ConfMan.setPath("soundfont", soundFont, _domain);
			}
		} else {
			ConfMan.removeKey("gm_device", _domain);
			ConfMan.removeKey("multi_midi", _domain);
			_multiMidiCheckbox->setOverride(false);
			ConfMan.removeKey("midi_gain", _domain);
			_midiGainDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
			ConfMan.removeKey("soundfont", _domain);
			_soundFont->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
		}
	}

	// MT-32 options
	if (_mt32DevicePopUp) {
		if (_enableMT32Settings) {
			saveMusicDeviceSetting(_mt32DevicePopUp, "mt32_device");
			ConfMan.setBool("native_mt32", _mt32Checkbox->getState(), _domain);
			if (ConfMan.getBool("native_mt32", _domain) != _mt32Checkbox->getState()) {
				ConfMan.setBool("native_mt32", _mt32Checkbox->getState(), _domain);
				_mt32Checkbox->setOverride(false);
			}
			if (ConfMan.getBool("enable_gs", _domain) != _enableGSCheckbox->getState()) {
				ConfMan.setBool("enable_gs", _enableGSCheckbox->getState(), _domain);
				_enableGSCheckbox->setOverride(false);
			}
		} else {
			ConfMan.removeKey("mt32_device", _domain);
			ConfMan.removeKey("native_mt32", _domain);
			_mt32Checkbox->setOverride(false);
			ConfMan.removeKey("enable_gs", _domain);
			_enableGSCheckbox->setOverride(false);
		}
	}

	// Subtitle options
	if (_subToggleGroup) {
		if (_enableSubtitleSettings) {
			if (_enableSubtitleToggle) {
				bool subtitles, speech_mute;
				switch (_subToggleGroup->getValue()) {
				case kSubtitlesSpeech:
					subtitles = speech_mute = false;
					break;
				case kSubtitlesBoth:
					subtitles = true;
					speech_mute = false;
					break;
				case kSubtitlesSubs:
				default:
					subtitles = speech_mute = true;
					break;
				}

				if (subtitles != ConfMan.getBool("subtitles", _domain)) {
					ConfMan.setBool("subtitles", subtitles, _domain);
					_subToggleDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
				}
				ConfMan.setBool("speech_mute", speech_mute, _domain);
			} else if (!_domain.empty()) {
				ConfMan.removeKey("subtitles", _domain);
				_subToggleDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
				ConfMan.removeKey("speech_mute", _domain);
			}

			// Engines that reuse the subtitle speed widget set their own max value.
			// Scale the config value accordingly (see addSubtitleControls)
			int sliderMaxValue = _subSpeedSlider->getMaxValue();
			int talkspeed = (_subSpeedSlider->getValue() * 255 + sliderMaxValue / 2) / sliderMaxValue;
			if (talkspeed != ConfMan.getInt("talkspeed", _domain)) {
				ConfMan.setInt("talkspeed", talkspeed, _domain);
				_subSpeedDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
			}
		} else {
			ConfMan.removeKey("subtitles", _domain);
			ConfMan.removeKey("talkspeed", _domain);
			_subSpeedDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
			ConfMan.removeKey("speech_mute", _domain);
		}
	}

	// Save config file
	ConfMan.flushToDisk();
}

void OptionsDialog::close() {
	if (getResult() > 0)
		apply();

	Dialog::close();
}

void OptionsDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {
	switch (cmd) {
	case kClearShaderCmd:
		_shader->setLabel(Common::Path());
		_shaderClearButton->setEnabled(false);
		break;
	case kMidiGainChanged:
		_midiGainLabel->setLabel(Common::String::format("%.2f", (double)_midiGainSlider->getValue() / 100.0));
		break;
	case kMusicVolumeChanged: {
		const int newValue = _musicVolumeSlider->getValue();
		_musicVolumeLabel->setValue(newValue);
		_musicVolumeLabel->markAsDirty();

		if (_guioptions.contains(GUIO_LINKMUSICTOSFX)) {
			updateSfxVolume(newValue);

			if (_guioptions.contains(GUIO_LINKSPEECHTOSFX)) {
				updateSpeechVolume(newValue);
			}
		}
		_musicVolumeDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
		break;
	}
	case kSfxVolumeChanged: {
		const int newValue = _sfxVolumeSlider->getValue();
		_sfxVolumeLabel->setValue(_sfxVolumeSlider->getValue());
		_sfxVolumeLabel->markAsDirty();

		if (_guioptions.contains(GUIO_LINKMUSICTOSFX)) {
			updateMusicVolume(newValue);
		}

		if (_guioptions.contains(GUIO_LINKSPEECHTOSFX)) {
			updateSpeechVolume(newValue);
		}
		_sfxVolumeDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
		break;
	}
	case kSpeechVolumeChanged: {
		const int newValue = _speechVolumeSlider->getValue();
		_speechVolumeLabel->setValue(newValue);
		_speechVolumeLabel->markAsDirty();

		if (_guioptions.contains(GUIO_LINKSPEECHTOSFX)) {
			updateSfxVolume(newValue);

			if (_guioptions.contains(GUIO_LINKMUSICTOSFX)) {
				updateMusicVolume(newValue);
			}
		}
		_speechVolumeDesc->setFontColor(ThemeEngine::FontColor::kFontColorNormal);
		break;
	}
	case kMuteAllChanged:
		// 'true' because if control is disabled then event do not pass
		setVolumeSettingsState(true);
		break;
	case kSubtitleToggle:
		// We update the slider settings here, when there are sliders, to
		// disable the speech volume in case we are in subtitle only mode.
		if (_musicVolumeSlider)
			setVolumeSettingsState(true);
		break;
	case kSubtitleSpeedChanged:
		_subSpeedLabel->setValue(_subSpeedSlider->getValue());
		_subSpeedLabel->markAsDirty();
		break;
	case kClearSoundFontCmd:
		_soundFont->setLabel(Common::Path());
		_soundFontClearButton->setEnabled(false);
		break;
	case kKbdMouseSpeedChanged:
		_kbdMouseSpeedLabel->setLabel(_(kbdMouseSpeedLabels[_kbdMouseSpeedSlider->getValue()]));
		break;
	case kJoystickDeadzoneChanged:
		_joystickDeadzoneLabel->setValue(_joystickDeadzoneSlider->getValue());
		_joystickDeadzoneLabel->markAsDirty();
		break;
	case kGraphicsTabContainerReflowCmd:
		setupGraphicsTab();
		break;
	case kScalerPopUpCmd:
		updateScaleFactors(data);
		g_gui.scheduleTopDialogRedraw();
		break;
	case kChooseShaderCmd: {
		ShaderBrowserDialog browser(ConfMan.getPath("shader", _domain));
		if (browser.runModal() > 0) {
			// User made his choice...
			_shader->setLabel(browser.getResult());

			if (!browser.getResult().empty())
				_shaderClearButton->setEnabled(true);
			else
				_shaderClearButton->setEnabled(false);
		}
		break;
	}
	case kApplyCmd:
		apply();
		break;
	case kOKCmd:
		setResult(1);
		close();
		break;
	case kCloseCmd:
		close();
		break;
	default:
		Dialog::handleCommand(sender, cmd, data);
	}
}

void OptionsDialog::handleTickle() {
	Dialog::handleTickle();

	if (_keymapperWidget) {
		_keymapperWidget->handleTickle();
	}
}

void OptionsDialog::handleOtherEvent(const Common::Event &event) {
	Dialog::handleOtherEvent(event);

	if (event.type == Common::EVENT_INPUT_CHANGED && _keymapperWidget) {
		_keymapperWidget->handleInputChanged();
	}
}

void OptionsDialog::setGraphicSettingsState(bool enabled) {
	_enableGraphicSettings = enabled;

	_gfxPopUpDesc->setEnabled(enabled);
	_gfxPopUp->setEnabled(enabled);
	_renderModePopUpDesc->setEnabled(enabled);
	_renderModePopUp->setEnabled(enabled);
	if (_rotationModePopUp) {
		_rotationModePopUpDesc->setEnabled(enabled);
		_rotationModePopUp->setEnabled(enabled);
	}

	if (_rendererTypePopUp) {
		_rendererTypePopUpDesc->setEnabled(enabled);
		_rendererTypePopUp->setEnabled(enabled);
	}

	if (_antiAliasPopUp) {
		_antiAliasPopUpDesc->setEnabled(enabled);
		_antiAliasPopUp->setEnabled(enabled);
	}

	if (g_system->hasFeature(OSystem::kFeatureStretchMode)) {
		_stretchPopUpDesc->setEnabled(enabled);
		_stretchPopUp->setEnabled(enabled);
	} else {
		// Happens when we switch to backend that doesn't support stretch modes
		if (_stretchPopUp) {
			_stretchPopUpDesc->setEnabled(false);
			_stretchPopUp->setEnabled(false);
		}
	}

	if (g_system->hasFeature(OSystem::kFeatureScalers)) {
		_scalerPopUpDesc->setEnabled(enabled);
		_scalerPopUp->setEnabled(enabled);
		_scaleFactorPopUp->setEnabled(enabled);
	} else {
		// Happens when we switch to backend that doesn't support scalers
		if (_scalerPopUp) {
			_scalerPopUpDesc->setEnabled(false);
			_scalerPopUp->setEnabled(false);
			_scaleFactorPopUp->setEnabled(false);
		}
	}

	enableShaderControls(g_system->hasFeature(OSystem::kFeatureShaders));

	if (g_system->hasFeature(OSystem::kFeatureFilteringMode))
		_filteringCheckbox->setEnabled(enabled);

	if (g_system->hasFeature(OSystem::kFeatureFullscreenMode))
		_fullscreenCheckbox->setEnabled(enabled);
	else
		_fullscreenCheckbox->setEnabled(false);

	if (g_system->hasFeature(OSystem::kFeatureAspectRatioCorrection)) {
		if (_guioptions.contains(GUIO_NOASPECT))
			_aspectCheckbox->setEnabled(false);
		else
			_aspectCheckbox->setEnabled(enabled);
	}

	if (g_system->hasFeature(OSystem::kFeatureVSync))
		_vsyncCheckbox->setEnabled(enabled);

}

void OptionsDialog::setAudioSettingsState(bool enabled) {
	_enableAudioSettings = enabled;
	_midiPopUpDesc->setEnabled(enabled);
	_midiPopUp->setEnabled(enabled);

	bool hasMidiDefined = _guioptions.contains(GUIO_MIDI_PREFIX);

	if (_domain != Common::ConfigManager::kApplicationDomain && // global dialog
		hasMidiDefined && // No Adlib is specified but others are
		!(_guioptions.contains(GUIO_MIDIADLIB))) {
		_oplPopUpDesc->setEnabled(false);
		_oplPopUp->setEnabled(false);
	} else {
		_oplPopUpDesc->setEnabled(enabled);
		_oplPopUp->setEnabled(enabled);
	}
}

void OptionsDialog::setMIDISettingsState(bool enabled) {
	if (_guioptions.contains(GUIO_NOMIDI))
		enabled = false;

	_gmDevicePopUpDesc->setEnabled(enabled);
	_gmDevicePopUp->setEnabled(enabled);

	_enableMIDISettings = enabled;

	_soundFontButton->setEnabled(enabled);
	_soundFont->setEnabled(enabled);

	if (enabled && !_soundFont->getLabel().empty())
		_soundFontClearButton->setEnabled(enabled);
	else
		_soundFontClearButton->setEnabled(false);

	_multiMidiCheckbox->setEnabled(enabled);
	_midiGainDesc->setEnabled(enabled);
	_midiGainSlider->setEnabled(enabled);
	_midiGainLabel->setEnabled(enabled);
}

void OptionsDialog::setMT32SettingsState(bool enabled) {
	_enableMT32Settings = enabled;

	_mt32DevicePopUpDesc->setEnabled(enabled);
	_mt32DevicePopUp->setEnabled(enabled);

	_mt32Checkbox->setEnabled(enabled);
	_enableGSCheckbox->setEnabled(enabled);
}

void OptionsDialog::setVolumeSettingsState(bool enabled) {
	bool ena;

	_enableVolumeSettings = enabled;

	ena = enabled && !_muteCheckbox->getState();
	if (_guioptions.contains(GUIO_NOMUSIC))
		ena = false;

	_musicVolumeDesc->setEnabled(ena);
	_musicVolumeSlider->setEnabled(ena);
	_musicVolumeLabel->setEnabled(ena);

	ena = enabled && !_muteCheckbox->getState();
	if (_guioptions.contains(GUIO_NOSFX))
		ena = false;

	_sfxVolumeDesc->setEnabled(ena);
	_sfxVolumeSlider->setEnabled(ena);
	_sfxVolumeLabel->setEnabled(ena);

	ena = enabled && !_muteCheckbox->getState();
	// Disable speech volume slider, when we are in subtitle only mode.
	if (_subToggleGroup)
		ena = ena && _subToggleGroup->getValue() != kSubtitlesSubs;
	if (_guioptions.contains(GUIO_NOSPEECH) || _guioptions.contains(GUIO_NOSPEECHVOLUME))
		ena = false;

	_speechVolumeDesc->setEnabled(ena);
	_speechVolumeSlider->setEnabled(ena);
	_speechVolumeLabel->setEnabled(ena);

	_muteCheckbox->setEnabled(enabled);
}

void OptionsDialog::setSubtitleSettingsState(bool enabled) {
	bool ena;
	_enableSubtitleSettings = enabled;

	ena = enabled;
	if ((_guioptions.contains(GUIO_NOSUBTITLES)) || (_guioptions.contains(GUIO_NOSPEECH)))
		ena = false;

	_enableSubtitleToggle = ena;
	_subToggleGroup->setEnabled(ena);
	_subToggleDesc->setEnabled(ena);

	ena = enabled;
	if (_guioptions.contains(GUIO_NOSUBTITLES))
		ena = false;

	_subSpeedDesc->setEnabled(ena);
	_subSpeedSlider->setEnabled(ena);
	_subSpeedLabel->setEnabled(ena);
}

void OptionsDialog::addControlControls(GuiObject *boss, const Common::String &prefix) {
	// Touchpad Mouse mode
	if (g_system->hasFeature(OSystem::kFeatureTouchpadMode))
		_touchpadCheckbox = new CheckboxWidget(boss, prefix + "grTouchpadCheckbox", _("Touchpad mouse mode"));

	// Keyboard and joystick mouse speed
	if (g_system->hasFeature(OSystem::kFeatureKbdMouseSpeed)) {
		if (!g_gui.useLowResGUI())
			_kbdMouseSpeedDesc = new StaticTextWidget(boss, prefix + "grKbdMouseSpeedDesc", _("Pointer Speed:"), _("Speed for keyboard/joystick mouse pointer control"));
		else
			_kbdMouseSpeedDesc = new StaticTextWidget(boss, prefix + "grKbdMouseSpeedDesc", _c("Pointer Speed:", "lowres"), _("Speed for keyboard/joystick mouse pointer control"));
		_kbdMouseSpeedSlider = new SliderWidget(boss, prefix + "grKbdMouseSpeedSlider", _("Speed for keyboard/joystick mouse pointer control"), kKbdMouseSpeedChanged);
		_kbdMouseSpeedLabel = new StaticTextWidget(boss, prefix + "grKbdMouseSpeedLabel", Common::U32String("  "), Common::U32String(), ThemeEngine::kFontStyleBold, Common::UNK_LANG, false);
		_kbdMouseSpeedSlider->setMinValue(0);
		_kbdMouseSpeedSlider->setMaxValue(7);
		_kbdMouseSpeedLabel->setFlags(WIDGET_CLEARBG);
	}

	// Joystick deadzone
	if (g_system->hasFeature(OSystem::kFeatureJoystickDeadzone)) {
		if (!g_gui.useLowResGUI())
			_joystickDeadzoneDesc = new StaticTextWidget(boss, prefix + "grJoystickDeadzoneDesc", _("Joy Deadzone:"), _("Analog joystick Deadzone"));
		else
			_joystickDeadzoneDesc = new StaticTextWidget(boss, prefix + "grJoystickDeadzoneDesc", _c("Joy Deadzone:", "lowres"), _("Analog joystick Deadzone"));
		_joystickDeadzoneSlider = new SliderWidget(boss, prefix + "grJoystickDeadzoneSlider", _("Analog joystick Deadzone"), kJoystickDeadzoneChanged);
		_joystickDeadzoneLabel = new StaticTextWidget(boss, prefix + "grJoystickDeadzoneLabel", Common::U32String("  "), Common::U32String(), ThemeEngine::kFontStyleBold, Common::UNK_LANG, false);
		_joystickDeadzoneSlider->setMinValue(1);
		_joystickDeadzoneSlider->setMaxValue(10);
		_joystickDeadzoneLabel->setFlags(WIDGET_CLEARBG);
	}
	_enableControlSettings = true;
}

void OptionsDialog::addKeyMapperControls(GuiObject *boss, const Common::String &prefix, const Common::KeymapArray &keymaps, const Common::String &domain) {
	Common::Keymapper *mapper = g_system->getEventManager()->getKeymapper();
	for (uint i = 0; i < keymaps.size(); i++) {
		mapper->initKeymap(keymaps[i], ConfMan.getDomain(domain));
	}

	_keymapperWidget = new Common::RemapWidget(boss, prefix + "Container", keymaps);
}

void OptionsDialog::addAchievementsControls(GuiObject *boss, const Common::String &prefix) {
	GUI::ScrollContainerWidget *scrollContainer;
	scrollContainer = new GUI::ScrollContainerWidget(boss, prefix + "Container", "");
	scrollContainer->setBackgroundType(GUI::ThemeEngine::kWidgetBackgroundNo);

	uint16 nAchieved = 0;
	uint16 nHidden = 0;
	uint16 nMax = AchMan.getAchievementCount();
	uint16 lineHeight = g_gui.xmlEval()->getVar("Globals.Line.Height");
	uint16 yStep = lineHeight;
	uint16 ySmallStep = yStep / 3;
	uint16 yPos = lineHeight + yStep * 3;

	uint16 width;
	uint16 textline_numchars;  // max number of chars in textline
	uint16 progressBarWidth;
	uint16 commentDelta = g_gui.useLowResGUI() ? 20 : 30; // textline left tabbing
	float scale_factor = g_gui.getScaleFactor();

	if (!g_gui.useLowResGUI()) { // hires
		width = 440 + 800 * (scale_factor - 1);
		textline_numchars = 70 + 80 * (scale_factor - 1);
		progressBarWidth = 240;
		commentDelta *= scale_factor;
		if (g_system->getOverlayWidth() - width < 100) {
			width = g_system->getOverlayWidth() - 100 * scale_factor; // clamp width to at least 100px smaller than overlay to prevent glitchy scrollbars
		}
	} else { // lores
		width = 250 + 480 * (scale_factor - 1);
		textline_numchars = 40 + 60 * (scale_factor - 1);
		progressBarWidth = 130;
	}

	for (int16 viewAchieved = 1; viewAchieved >= 0; viewAchieved--) {
		// run this twice, first view all achieved, then view all non-hidden & non-achieved

		for (uint16 idx = 0; idx < nMax ; idx++) {
			const Common::AchievementDescription *descr = AchMan.getAchievementDescription(idx);
			int16 isAchieved = AchMan.isAchieved(descr->id) ? 1 : 0;

			if (isAchieved != viewAchieved) {
				continue;
			}

			if (isAchieved) {
				nAchieved++;
			}

			if (!isAchieved && descr->isHidden) {
				nHidden++;
				continue;
			}

			CheckboxWidget *checkBox;
			checkBox = new CheckboxWidget(scrollContainer, lineHeight, yPos, width, yStep, Common::U32String(descr->title));
			checkBox->setEnabled(false);
			checkBox->setState(isAchieved);
			yPos += yStep;

			if (!descr->comment.empty()) {
				Common::U32String comment_line(descr->comment);
				uint16 str_chars = comment_line.size(), printed_chars = 0, i = 0;
				while ((str_chars - printed_chars) > textline_numchars) { // check if string needs to go on multiple lines
					for (i = (printed_chars + textline_numchars - 1); comment_line[i] != ' ' && i > 0; i--) {}; // find a space to avoid breaking words
					new StaticTextWidget(scrollContainer, lineHeight + commentDelta, yPos, width - commentDelta, yStep, Common::U32String(comment_line.begin() + (!printed_chars ? 0 : (printed_chars + 1)), comment_line.begin() + i), Graphics::kTextAlignStart, Common::U32String(), ThemeEngine::kFontStyleNormal);
					yPos += yStep;
					printed_chars = i;
					i++; // to skip trailing space when leaving the loop
				}
				new StaticTextWidget(scrollContainer, lineHeight + commentDelta, yPos, width - commentDelta, yStep, Common::U32String(comment_line.begin() + i, comment_line.end()), Graphics::kTextAlignStart, Common::U32String(), ThemeEngine::kFontStyleNormal);
				yPos += yStep;
			}
			yPos += ySmallStep;
		}
	}

	if (nHidden) {
		Common::U32String hiddenStr = Common::U32String::format(_("%d hidden achievements remaining"), nHidden);
		new StaticTextWidget(scrollContainer, lineHeight, yPos, width, yStep, hiddenStr, Graphics::kTextAlignStart);
	}

	if (nMax) {
		Common::U32String totalStr = Common::U32String::format(_("Achievements unlocked: %d/%d"), nAchieved, nMax);
		new StaticTextWidget(scrollContainer, lineHeight, lineHeight, width, yStep, totalStr, Graphics::kTextAlignStart);

		SliderWidget *progressBar;
		progressBar = new SliderWidget(scrollContainer, lineHeight, lineHeight*2, progressBarWidth, lineHeight);
		progressBar->setMinValue(0);
		progressBar->setValue(nAchieved);
		progressBar->setMaxValue(nMax);
		progressBar->setEnabled(false);
	}
}

void OptionsDialog::addStatisticsControls(GuiObject *boss, const Common::String &prefix) {
	GUI::ScrollContainerWidget *scrollContainer;
	scrollContainer = new GUI::ScrollContainerWidget(boss, prefix + "Container", "");
	scrollContainer->setBackgroundType(GUI::ThemeEngine::kWidgetBackgroundNo);

	uint16 nMax = AchMan.getStatCount();

	uint16 lineHeight = g_gui.xmlEval()->getVar("Globals.Line.Height");
	uint16 yStep = lineHeight;
	uint16 ySmallStep = yStep / 3;
	uint16 yPos = lineHeight;
	uint16 width = g_gui.useLowResGUI() ? 240 : 410;

	for (uint16 idx = 0; idx < nMax ; idx++) {
		const Common::StatDescription *descr = AchMan.getStatDescription(idx);

		const Common::String &key = descr->comment.empty() ? descr->id : descr->comment;
		Common::String value = AchMan.getStatRaw(descr->id);

		Common::U32String str = Common::U32String::format("%s: %s", key.c_str(), value.c_str());
		new StaticTextWidget(scrollContainer, lineHeight, yPos, width, yStep, str, Graphics::kTextAlignStart);

		yPos += yStep;
		yPos += ySmallStep;
	}
}

void OptionsDialog::addGraphicControls(GuiObject *boss, const Common::String &prefix) {
	const OSystem::GraphicsMode *gm = g_system->getSupportedGraphicsModes();
	Common::String context;
	if (g_gui.useLowResGUI())
		context = "lowres";

	// The GFX mode popup
	_gfxPopUpDesc = new StaticTextWidget(boss, prefix + "grModePopupDesc", _("Graphics mode:"));
	if (ConfMan.isKeyTemporary("gfx_mode"))
		_gfxPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
	_gfxPopUp = new PopUpWidget(boss, prefix + "grModePopup");

	_gfxPopUp->appendEntry(_("<default>"));
	_gfxPopUp->appendEntry(Common::U32String());
	while (gm->name) {
		_gfxPopUp->appendEntry(_c(gm->description, context), gm->id);
		gm++;
	}

	// RenderMode popup
	bool renderingTypeDefined = _guioptions.contains(GUIO_RENDER_PREFIX);

	_renderModePopUpDesc = new StaticTextWidget(boss, prefix + "grRenderPopupDesc", _("Render mode:"), _("Special dithering modes supported by some games"));
	if (ConfMan.isKeyTemporary("render_mode"))
		_renderModePopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
	_renderModePopUp = new PopUpWidget(boss, prefix + "grRenderPopup", _("Special dithering modes supported by some games"));
	_renderModePopUp->appendEntry(_("<default>"), Common::kRenderDefault);
	_renderModePopUp->appendEntry(Common::U32String());
	const Common::RenderModeDescription *rm = Common::g_renderModes;
	for (; rm->code; ++rm) {
		Common::String renderGuiOption = Common::renderMode2GUIO(rm->id);
		if ((_domain == Common::ConfigManager::kApplicationDomain) || (_domain != Common::ConfigManager::kApplicationDomain && renderingTypeDefined && _guioptions.contains(renderGuiOption)))
			_renderModePopUp->appendEntry(_c(rm->description, context), rm->id);
	}

	// The Stretch mode popup
	if (g_system->hasFeature(OSystem::kFeatureStretchMode)) {
		const OSystem::GraphicsMode *sm = g_system->getSupportedStretchModes();
		_stretchPopUpDesc = new StaticTextWidget(boss, prefix + "grStretchModePopupDesc", _("Stretch mode:"));
		if (ConfMan.isKeyTemporary("stretch_mode"))
			_stretchPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
		_stretchPopUp = new PopUpWidget(boss, prefix + "grStretchModePopup");

		_stretchPopUp->appendEntry(_("<default>"));
		_stretchPopUp->appendEntry(Common::U32String());
		while (sm->name) {
			_stretchPopUp->appendEntry(_c(sm->description, context), sm->id);
			sm++;
		}
	}

	// The Scaler popup
	if (g_system->hasFeature(OSystem::kFeatureScalers)) {
		const PluginList &scalerPlugins = ScalerMan.getPlugins();
		_scalerPopUpDesc = new StaticTextWidget(boss, prefix + "grScalerPopupDesc", _("Scaler:"));
		_scalerPopUp = new PopUpWidget(boss, prefix + "grScalerPopup", Common::U32String(), kScalerPopUpCmd);

		_scalerPopUp->appendEntry(_("<default>"));
		_scalerPopUp->appendEntry(Common::U32String());
		for (uint scalerIndex = 0; scalerIndex < scalerPlugins.size(); scalerIndex++) {
			_scalerPopUp->appendEntry(_c(scalerPlugins[scalerIndex]->get<ScalerPluginObject>().getPrettyName(), context), scalerIndex);
		}

		_scaleFactorPopUp = new PopUpWidget(boss, prefix + "grScaleFactorPopup");
		updateScaleFactors(_scalerPopUp->getSelectedTag());
	}

	if (g_system->hasFeature(OSystem::kFeatureRotationMode)) {
		const Common::RotationModeDescription *rotm = Common::g_rotationModes;
		_rotationModePopUpDesc = new StaticTextWidget(boss, prefix + "grRotationModePopupDesc", _("Rotation mode:"));
		if (ConfMan.isKeyTemporary("rotation_mode"))
			_rotationModePopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
		_rotationModePopUp = new PopUpWidget(boss, prefix + "grRotationModePopup");

		_rotationModePopUp->appendEntry(_("<default>"));
		_rotationModePopUp->appendEntry(Common::U32String());
		while (rotm->description) {
			_rotationModePopUp->appendEntry(_c(rotm->description, context), rotm->id);
			rotm++;
		}
	}

	if (!g_gui.useLowResGUI())
		_shaderButton = new ButtonWidget(boss, prefix + "grShaderButton", _("Shader:"), _("Specifies path to the shader used for scaling the game screen"), kChooseShaderCmd);
	else
		_shaderButton = new ButtonWidget(boss, prefix + "grShaderButton", _c("Shader Path:", "lowres"), _("Specifies path to the shader used for scaling the game screen"), kChooseShaderCmd);
	_shader = new PathWidget(boss, prefix + "grShader", Common::Path(), _c("None", "shader"), _("Specifies path to the shader used for scaling the game screen"));

	_shaderClearButton = addClearButton(boss, prefix + "grShaderClearButton", kClearShaderCmd);

#ifdef USE_HTTP
#ifndef EMSCRIPTEN // No shader updates on Emscripten, they are always loaded directly from server
	_updateShadersButton = new ButtonWidget(boss, prefix + "UpdateShadersButton", _("Download Shaders"), _("Check on the scummvm.org website for updates of shader packs"), kUpdateShadersCmd);
#endif
#endif

	enableShaderControls(g_system->hasFeature(OSystem::kFeatureShaders));

	// Fullscreen checkbox
	_fullscreenCheckbox = new CheckboxWidget(boss, prefix + "grFullscreenCheckbox", _("Fullscreen mode"), Common::U32String(), kFullscreenToggled);

	if (g_system->hasFeature(OSystem::kFeatureVSync))
		_vsyncCheckbox = new CheckboxWidget(boss, prefix + "grVSyncCheckbox", _("V-Sync"), _("Wait for the vertical sync to refresh the screen in order to prevent tearing artifacts"));

	Common::Array<Graphics::RendererTypeDescription> rt = Graphics::Renderer::listTypes();
	if (!rt.empty()) {
		if (!g_gui.useLowResGUI())
			_rendererTypePopUpDesc = new StaticTextWidget(boss, prefix + "grRendererTypePopupDesc", _("Game 3D Renderer:"));
		else
			_rendererTypePopUpDesc = new StaticTextWidget(boss, prefix + "grRendererTypePopupDesc", _c("Game 3D Renderer:", "lowres"));

		_rendererTypePopUp = new PopUpWidget(boss, prefix + "grRendererTypePopup");
		_rendererTypePopUp->appendEntry(_("<default>"), Graphics::kRendererTypeDefault);
		_rendererTypePopUp->appendEntry("");
		for (Common::Array<Graphics::RendererTypeDescription>::iterator it = rt.begin();
		        it != rt.end(); ++it) {
			if (!g_gui.useLowResGUI()) {
				_rendererTypePopUp->appendEntry(_(it->description), it->id);
			} else {
				_rendererTypePopUp->appendEntry(_c(it->description, "lowres"), it->id);
			}
		}
	}

	const Common::Array<uint> levels = g_system->getSupportedAntiAliasingLevels();
	if (!levels.empty()) {
		_antiAliasPopUpDesc = new StaticTextWidget(boss, prefix + "grAntiAliasPopupDesc", _("3D Anti-aliasing:"));
		_antiAliasPopUp = new PopUpWidget(boss, prefix + "grAntiAliasPopup");
		_antiAliasPopUp->appendEntry(_("<default>"), uint32(-1));
		_antiAliasPopUp->appendEntry("");
		_antiAliasPopUp->appendEntry(_("Disabled"), 0);
		for (uint i = 0; i < levels.size(); i++) {
			_antiAliasPopUp->appendEntry(Common::String::format("%dx", levels[i]), levels[i]);
		}
	}

	// Filtering checkbox
	if (g_system->hasFeature(OSystem::kFeatureFilteringMode))
		_filteringCheckbox = new CheckboxWidget(boss, prefix + "grFilteringCheckbox", _("Filter graphics"), _("Use linear filtering when scaling graphics"));

	// Aspect ratio checkbox
	if (g_system->hasFeature(OSystem::kFeatureAspectRatioCorrection))
		_aspectCheckbox = new CheckboxWidget(boss, prefix + "grAspectCheckbox", _("Aspect ratio correction"), _("Correct aspect ratio for games"));

	_enableGraphicSettings = true;
}

void OptionsDialog::enableShaderControls(bool enable) {
	_shaderButton->setEnabled(enable);
	_shader->setEnabled(enable);
	_shaderClearButton->setVisible(enable);

	if (_updateShadersButton)
		_updateShadersButton->setEnabled(enable);
}

void OptionsDialog::addAudioControls(GuiObject *boss, const Common::String &prefix) {
	// The MIDI mode popup & a label
	if (!g_gui.useLowResGUI())
		_midiPopUpDesc = new StaticTextWidget(boss, prefix + "auMidiPopupDesc", _domain == Common::ConfigManager::kApplicationDomain ? _("Preferred device:") : _("Music device:"), _domain == Common::ConfigManager::kApplicationDomain ? _("Specifies preferred sound device or sound card emulator") : _("Specifies output sound device or sound card emulator"));
	else
		_midiPopUpDesc = new StaticTextWidget(boss, prefix + "auMidiPopupDesc", _domain == Common::ConfigManager::kApplicationDomain ? _c("Preferred dev.:", "lowres") : _c("Music device:", "lowres"), _domain == Common::ConfigManager::kApplicationDomain ? _("Specifies preferred sound device or sound card emulator") : _("Specifies output sound device or sound card emulator"));
	_midiPopUp = new PopUpWidget(boss, prefix + "auMidiPopup", _("Specifies output sound device or sound card emulator"));

	// Populate it
	bool hasMidiDefined = _guioptions.contains(GUIO_MIDI_PREFIX);

	const PluginList p = MusicMan.getPlugins();
	for (const auto &m : p) {
		MusicDevices i = m->get<MusicPluginObject>().getDevices();
		for (auto &d : i) {
			Common::String deviceGuiOption = MidiDriver::musicType2GUIO(d.getMusicType());

			if ((_domain == Common::ConfigManager::kApplicationDomain && d.getMusicType() != MT_TOWNS  // global dialog - skip useless FM-Towns, C64, Amiga, AppleIIGS, Macintosh and SegaCD options there
				 && d.getMusicType() != MT_C64 && d.getMusicType() != MT_AMIGA && d.getMusicType() != MT_APPLEIIGS && d.getMusicType() != MT_PC98 && d.getMusicType() != MT_MACINTOSH && d.getMusicType() != MT_SEGACD)
				|| (_domain != Common::ConfigManager::kApplicationDomain && !hasMidiDefined) // No flags are specified
				|| (_guioptions.contains(deviceGuiOption)) // flag is present
				// HACK/FIXME: For now we have to show GM devices, even when the game only has GUIO_MIDIMT32 set,
				// else we would not show for example external devices connected via ALSA, since they are always
				// marked as General MIDI device.
				|| (deviceGuiOption.contains(GUIO_MIDIGM) && (_guioptions.contains(GUIO_MIDIMT32)))
				|| d.getMusicDriverId() == "auto" || d.getMusicDriverId() == "null") // always add default and null device
				_midiPopUp->appendEntry(_(d.getCompleteName()), d.getHandle());
		}
	}

	// The OPL emulator popup & a label
	_oplPopUpDesc = new StaticTextWidget(boss, prefix + "auOPLPopupDesc", _("AdLib emulator:"), _("AdLib is used for music in many games"));
	if (ConfMan.isKeyTemporary("opl_driver"))
		_oplPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
	_oplPopUp = new PopUpWidget(boss, prefix + "auOPLPopup", _("AdLib is used for music in many games"));

	// Populate it
	const OPL::Config::EmulatorDescription *ed = OPL::Config::getAvailable();
	while (ed->name) {
		_oplPopUp->appendEntry(_(ed->description), ed->id);
		++ed;
	}

	_enableAudioSettings = true;
}

void OptionsDialog::addMIDIControls(GuiObject *boss, const Common::String &prefix) {
	_gmDevicePopUpDesc = new StaticTextWidget(boss, prefix + "auPrefGmPopupDesc", _("GM device:"), _("Specifies default sound device for General MIDI output"));
	_gmDevicePopUp = new PopUpWidget(boss, prefix + "auPrefGmPopup");

	// Populate
	const PluginList p = MusicMan.getPlugins();
	// Make sure the null device is the first one in the list to avoid undesired
	// auto detection for users who don't have a saved setting yet.
	for (const auto &m : p) {
		MusicDevices i = m->get<MusicPluginObject>().getDevices();
		for (auto &d : i) {
			if (d.getMusicDriverId() == "null")
				_gmDevicePopUp->appendEntry(_("Don't use General MIDI music"), d.getHandle());
		}
	}
	// Now we add the other devices.
	for (const auto &m : p) {
		MusicDevices i = m->get<MusicPluginObject>().getDevices();
		for (auto &d : i) {
			if (d.getMusicType() >= MT_GM) {
				if (d.getMusicType() != MT_MT32)
					_gmDevicePopUp->appendEntry(d.getCompleteName(), d.getHandle());
			} else if (d.getMusicDriverId() == "auto") {
				_gmDevicePopUp->appendEntry(_("Use first available device"), d.getHandle());
			}
		}
	}

	if (!_domain.equals(Common::ConfigManager::kApplicationDomain)) {
		_gmDevicePopUpDesc->setEnabled(false);
		_gmDevicePopUp->setEnabled(false);
	}

	// SoundFont
	if (!g_gui.useLowResGUI())
		_soundFontButton = new ButtonWidget(boss, prefix + "mcFontButton", _("SoundFont:"), _("SoundFont is supported by some audio cards, FluidSynth and Timidity"), kChooseSoundFontCmd);
	else
		_soundFontButton = new ButtonWidget(boss, prefix + "mcFontButton", _c("SoundFont:", "lowres"), _("SoundFont is supported by some audio cards, FluidSynth and Timidity"), kChooseSoundFontCmd);

	Common::U32String soundFountPlaceholder;
	if (ConfMan.hasDefault("soundfont")) {
		soundFountPlaceholder = _("Default");
	} else {
		soundFountPlaceholder = _c("None", "soundfont");
	}
	_soundFont = new PathWidget(boss, prefix + "mcFontPath", Common::Path(), soundFountPlaceholder, _("SoundFont is supported by some audio cards, FluidSynth and Timidity"));

	_soundFontClearButton = addClearButton(boss, prefix + "mcFontClearButton", kClearSoundFontCmd);

	// Multi midi setting
	_multiMidiCheckbox = new CheckboxWidget(boss, prefix + "mcMixedCheckbox", _("Mixed AdLib/MIDI mode"), _("Use both MIDI and AdLib sound generation"));

	// MIDI gain setting (FluidSynth uses this)
	_midiGainDesc = new StaticTextWidget(boss, prefix + "mcMidiGainText", _("MIDI gain:"));
	_midiGainSlider = new SliderWidget(boss, prefix + "mcMidiGainSlider", Common::U32String(), kMidiGainChanged);
	_midiGainSlider->setMinValue(0);
	_midiGainSlider->setMaxValue(1000);
	_midiGainLabel = new StaticTextWidget(boss, prefix + "mcMidiGainLabel", Common::U32String("1.00"));

	_enableMIDISettings = true;
}

void OptionsDialog::addMT32Controls(GuiObject *boss, const Common::String &prefix) {
	_mt32DevicePopUpDesc = new StaticTextWidget(boss, prefix + "auPrefMt32PopupDesc", _("MT-32 device:"), _("Specifies default sound device for Roland MT-32/LAPC1/CM32l/CM64 output"));
	_mt32DevicePopUp = new PopUpWidget(boss, prefix + "auPrefMt32Popup");

	// Native mt32 setting
	if (!g_gui.useLowResGUI())
		_mt32Checkbox = new CheckboxWidget(boss, prefix + "mcMt32Checkbox", _("True Roland MT-32 (disable GM emulation)"), _("Check if you want to use your real hardware Roland-compatible sound device connected to your computer"));
	else
		_mt32Checkbox = new CheckboxWidget(boss, prefix + "mcMt32Checkbox", _c("True Roland MT-32 (no GM emulation)", "lowres"), _("Check if you want to use your real hardware Roland-compatible sound device connected to your computer"));

	// GS Extensions setting
	_enableGSCheckbox = new CheckboxWidget(boss, prefix + "mcGSCheckbox", _("Roland GS device (enable MT-32 mappings)"), _("Check if you want to enable patch mappings to emulate an MT-32 on a Roland GS device"));

	const PluginList p = MusicMan.getPlugins();
	// Make sure the null device is the first one in the list to avoid undesired
	// auto detection for users who don't have a saved setting yet.
	for (const auto &m : p) {
		MusicDevices i = m->get<MusicPluginObject>().getDevices();
		for (auto &d : i) {
			if (d.getMusicDriverId() == "null")
				_mt32DevicePopUp->appendEntry(_("Don't use Roland MT-32 music"), d.getHandle());
		}
	}
	// Now we add the other devices.
	for (const auto &m : p) {
		MusicDevices i = m->get<MusicPluginObject>().getDevices();
		for (auto &d : i) {
			if (d.getMusicType() >= MT_GM)
				_mt32DevicePopUp->appendEntry(d.getCompleteName(), d.getHandle());
			else if (d.getMusicDriverId() == "auto")
				_mt32DevicePopUp->appendEntry(_("Use first available device"), d.getHandle());
		}
	}

	if (!_domain.equals(Common::ConfigManager::kApplicationDomain)) {
		_mt32DevicePopUpDesc->setEnabled(false);
		_mt32DevicePopUp->setEnabled(false);
	}

	_enableMT32Settings = true;
}

// The function has an extra slider range parameter, since both the launcher and SCUMM engine
// make use of the widgets. The launcher range is 0-255. SCUMM's 0-9
void OptionsDialog::addSubtitleControls(GuiObject *boss, const Common::String &prefix, int maxSliderVal) {

	if (!g_gui.useLowResGUI()) {
		_subToggleDesc = new StaticTextWidget(boss, prefix + "subToggleDesc", _("Text and speech:"));

		_subToggleGroup = new RadiobuttonGroup(boss, kSubtitleToggle);

		_subToggleSpeechOnly = new RadiobuttonWidget(boss, prefix + "subToggleSpeechOnly", _subToggleGroup, kSubtitlesSpeech, _("Speech"));
		_subToggleSubOnly = new RadiobuttonWidget(boss, prefix + "subToggleSubOnly", _subToggleGroup, kSubtitlesSubs, _("Subtitles"));
		_subToggleSubBoth = new RadiobuttonWidget(boss, prefix + "subToggleSubBoth", _subToggleGroup, kSubtitlesBoth, _("Both"));

		_subSpeedDesc = new StaticTextWidget(boss, prefix + "subSubtitleSpeedDesc", _("Subtitle speed:"));
	} else {
		_subToggleDesc = new StaticTextWidget(boss, prefix + "subToggleDesc", _c("Text and speech:", "lowres"));

		_subToggleGroup = new RadiobuttonGroup(boss, kSubtitleToggle);

		_subToggleSpeechOnly = new RadiobuttonWidget(boss, prefix + "subToggleSpeechOnly", _subToggleGroup, kSubtitlesSpeech, _("Spch"), _("Speech"));
		_subToggleSubOnly = new RadiobuttonWidget(boss, prefix + "subToggleSubOnly", _subToggleGroup, kSubtitlesSubs, _("Subs"), _("Subtitles"));
		_subToggleSubBoth = new RadiobuttonWidget(boss, prefix + "subToggleSubBoth", _subToggleGroup, kSubtitlesBoth, _c("Both", "lowres"), _("Show subtitles and play speech"));

		_subSpeedDesc = new StaticTextWidget(boss, prefix + "subSubtitleSpeedDesc", _c("Subtitle speed:", "lowres"));
	}

	if (ConfMan.isKeyTemporary("talkspeed"))
		_subSpeedDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
	if (ConfMan.isKeyTemporary("subtitles"))
		_subToggleDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);

	// Subtitle speed
	_subSpeedSlider = new SliderWidget(boss, prefix + "subSubtitleSpeedSlider", Common::U32String(), kSubtitleSpeedChanged);
	_subSpeedLabel = new StaticTextWidget(boss, prefix + "subSubtitleSpeedLabel", Common::U32String("100%"), Common::U32String(), ThemeEngine::kFontStyleBold, Common::UNK_LANG, false);
	_subSpeedSlider->setMinValue(0); _subSpeedSlider->setMaxValue(maxSliderVal);
	_subSpeedLabel->setFlags(WIDGET_CLEARBG);

	_enableSubtitleSettings = true;
	_enableSubtitleToggle = true;
}

void OptionsDialog::addVolumeControls(GuiObject *boss, const Common::String &prefix) {

	// Volume controllers
	if (!g_gui.useLowResGUI())
		_musicVolumeDesc = new StaticTextWidget(boss, prefix + "vcMusicText", _("Music volume:"));
	else
		_musicVolumeDesc = new StaticTextWidget(boss, prefix + "vcMusicText", _c("Music volume:", "lowres"));
	if (ConfMan.isKeyTemporary("music_volume"))
		_musicVolumeDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
	_musicVolumeSlider = new SliderWidget(boss, prefix + "vcMusicSlider", Common::U32String(), kMusicVolumeChanged);
	_musicVolumeLabel = new StaticTextWidget(boss, prefix + "vcMusicLabel", Common::U32String("100%"), Common::U32String(), ThemeEngine::kFontStyleBold, Common::UNK_LANG, false);
	_musicVolumeSlider->setMinValue(0);
	_musicVolumeSlider->setMaxValue(Audio::Mixer::kMaxMixerVolume);
	_musicVolumeLabel->setFlags(WIDGET_CLEARBG);

	_muteCheckbox = new CheckboxWidget(boss, prefix + "vcMuteCheckbox", _("Mute all"), Common::U32String(), kMuteAllChanged);

	if (!g_gui.useLowResGUI())
		_sfxVolumeDesc = new StaticTextWidget(boss, prefix + "vcSfxText", _("SFX volume:"), _("Special sound effects volume"));
	else
		_sfxVolumeDesc = new StaticTextWidget(boss, prefix + "vcSfxText", _c("SFX volume:", "lowres"), _("Special sound effects volume"));
	if (ConfMan.isKeyTemporary("sfx_volume"))
		_sfxVolumeDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
	_sfxVolumeSlider = new SliderWidget(boss, prefix + "vcSfxSlider", _("Special sound effects volume"), kSfxVolumeChanged);
	_sfxVolumeLabel = new StaticTextWidget(boss, prefix + "vcSfxLabel", Common::U32String("100%"), Common::U32String(), ThemeEngine::kFontStyleBold, Common::UNK_LANG, false);
	_sfxVolumeSlider->setMinValue(0);
	_sfxVolumeSlider->setMaxValue(Audio::Mixer::kMaxMixerVolume);
	_sfxVolumeLabel->setFlags(WIDGET_CLEARBG);

	if (!g_gui.useLowResGUI())
		_speechVolumeDesc = new StaticTextWidget(boss, prefix + "vcSpeechText" , _("Speech volume:"));
	else
		_speechVolumeDesc = new StaticTextWidget(boss, prefix + "vcSpeechText" , _c("Speech volume:", "lowres"));
	if (ConfMan.isKeyTemporary("speech_volume"))
		_speechVolumeDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
	_speechVolumeSlider = new SliderWidget(boss, prefix + "vcSpeechSlider", Common::U32String(), kSpeechVolumeChanged);
	_speechVolumeLabel = new StaticTextWidget(boss, prefix + "vcSpeechLabel", Common::U32String("100%"), Common::U32String(), ThemeEngine::kFontStyleBold, Common::UNK_LANG, false);
	_speechVolumeSlider->setMinValue(0);
	_speechVolumeSlider->setMaxValue(Audio::Mixer::kMaxMixerVolume);
	_speechVolumeLabel->setFlags(WIDGET_CLEARBG);

	_enableVolumeSettings = true;
}

bool OptionsDialog::loadMusicDeviceSetting(PopUpWidget *popup, Common::String setting, MusicType preferredType) {
	if (!popup || !popup->isEnabled())
		return true;

	if (_domain != Common::ConfigManager::kApplicationDomain || ConfMan.hasKey(setting, _domain) || preferredType) {
		const Common::String drv = ConfMan.get(setting, (_domain != Common::ConfigManager::kApplicationDomain && !ConfMan.hasKey(setting, _domain)) ? Common::ConfigManager::kApplicationDomain : _domain);
		const PluginList p = MusicMan.getPlugins();

		for (const auto &m : p) {
			MusicDevices i = m->get<MusicPluginObject>().getDevices();
			for (auto &d : i) {
				if (setting.empty() ? (preferredType == d.getMusicType()) : (drv == d.getCompleteId())) {
					popup->setSelectedTag(d.getHandle());
					return popup->getSelected() != -1;
				}
			}
		}
	}

	return false;
}

void OptionsDialog::saveMusicDeviceSetting(PopUpWidget *popup, Common::String setting) {
	if (!popup || !_enableAudioSettings)
		return;

	const PluginList p = MusicMan.getPlugins();
	bool found = false;
	for (PluginList::const_iterator m = p.begin(); m != p.end() && !found; ++m) {
		MusicDevices i = (*m)->get<MusicPluginObject>().getDevices();
		for (auto &d : i) {
			if (d.getHandle() == popup->getSelectedTag()) {
				ConfMan.set(setting, d.getCompleteId(), _domain);
				found = true;
				break;
			}
		}
	}

	if (!found)
		ConfMan.removeKey(setting, _domain);
}

int OptionsDialog::getSubtitleMode(bool subtitles, bool speech_mute) {
	if (_guioptions.contains(GUIO_NOSUBTITLES))
		return kSubtitlesSpeech; // Speech only
	if (_guioptions.contains(GUIO_NOSPEECH))
		return kSubtitlesSubs; // Subtitles only

	if (!subtitles && !speech_mute) // Speech only
		return kSubtitlesSpeech;
	else if (subtitles && !speech_mute) // Speech and subtitles
		return kSubtitlesBoth;
	else if (subtitles && speech_mute) // Subtitles only
		return kSubtitlesSubs;
	else
		warning("Wrong configuration: Both subtitles and speech are off. Assuming subtitles only");
	return kSubtitlesSubs;
}

void OptionsDialog::updateMusicVolume(const int newValue) const {
	_musicVolumeLabel->setValue(newValue);
	_musicVolumeSlider->setValue(newValue);
	_musicVolumeLabel->markAsDirty();
	_musicVolumeSlider->markAsDirty();
}

void OptionsDialog::updateSfxVolume(const int newValue) const {
	_sfxVolumeLabel->setValue(newValue);
	_sfxVolumeSlider->setValue(newValue);
	_sfxVolumeLabel->markAsDirty();
	_sfxVolumeSlider->markAsDirty();
}

void OptionsDialog::updateSpeechVolume(const int newValue) const {
	_speechVolumeLabel->setValue(newValue);
	_speechVolumeSlider->setValue(newValue);
	_speechVolumeLabel->markAsDirty();
	_speechVolumeSlider->markAsDirty();
}

void OptionsDialog::reflowLayout() {
	if (_graphicsTabId != -1 && _tabWidget)
		_tabWidget->setTabTitle(_graphicsTabId, g_gui.useLowResGUI() ? _("GFX") : _("Graphics"));

	Dialog::reflowLayout();
	setupGraphicsTab();
}

void OptionsDialog::setupGraphicsTab() {
	if (_graphicsTabId != -1) {
		// Since we do not create shader controls, the rebuild is required
		// Fixes crash when switching from SDL Surface to OpenGL
		if (!_scalerPopUp && g_system->hasFeature(OSystem::kFeatureScalers)) {
			rebuild();
		} else if (!_stretchPopUp && g_system->hasFeature(OSystem::kFeatureStretchMode)) {
			rebuild();
		} else if (!_aspectCheckbox && g_system->hasFeature(OSystem::kFeatureAspectRatioCorrection)) {
			rebuild();
		}
		setGraphicSettingsState(_enableGraphicSettings);
	}
	if (!_fullscreenCheckbox)
		return;
	_gfxPopUpDesc->setVisible(true);
	_gfxPopUp->setVisible(true);
	if (g_system->hasFeature(OSystem::kFeatureStretchMode)) {
		_stretchPopUpDesc->setVisible(true);
		_stretchPopUp->setVisible(true);
	}
	if (g_system->hasFeature(OSystem::kFeatureRotationMode)) {
		_rotationModePopUpDesc->setVisible(true);
		_rotationModePopUp->setVisible(true);
	}
	_fullscreenCheckbox->setVisible(true);
	if (g_system->hasFeature(OSystem::kFeatureFilteringMode))
		_filteringCheckbox->setVisible(true);

	if (g_system->hasFeature(OSystem::kFeatureAspectRatioCorrection))
		_aspectCheckbox->setVisible(true);

	_renderModePopUpDesc->setVisible(true);
	_renderModePopUp->setVisible(true);

	if (g_system->hasFeature(OSystem::kFeatureScalers)) {
		_scalerPopUpDesc->setVisible(true);
		if (ConfMan.isKeyTemporary("scaler") || ConfMan.isKeyTemporary("scale_factor"))
			_scalerPopUpDesc->setFontColor(ThemeEngine::FontColor::kFontColorOverride);
		_scalerPopUp->setVisible(true);
		_scaleFactorPopUp->setVisible(true);
	}

	enableShaderControls(g_system->hasFeature(OSystem::kFeatureShaders));
}

void OptionsDialog::updateScaleFactors(uint32 tag) {
	if ((int32)tag >= 0) {
		const PluginList &scalerPlugins = ScalerMan.getPlugins();
		const Common::Array<uint> &factors = scalerPlugins[tag]->get<ScalerPluginObject>().getFactors();

		_scaleFactorPopUp->clearEntries();
		for (const auto &factor : factors) {
			_scaleFactorPopUp->appendEntry(Common::U32String::format("%dx", factor), factor);
		}

		if (g_system->getScaler() == tag) {
			_scaleFactorPopUp->setSelectedTag(g_system->getScaleFactor());
		} else {
			_scaleFactorPopUp->setSelectedTag(scalerPlugins[tag]->get<ScalerPluginObject>().getDefaultFactor());
		}
	} else {
		_scaleFactorPopUp->clearEntries();
		_scaleFactorPopUp->appendEntry(_("<default>"));
		_scaleFactorPopUp->setSelected(0);
	}
}

bool OptionsDialog::testGraphicsSettings() {
	int xres = 320, yres = 240;

	g_system->beginGFXTransaction();
	g_system->initSize(xres, yres);
	g_system->endGFXTransaction();

	Graphics::ManagedSurface *pm5544 = Graphics::renderPM5544(xres, yres);

	byte palette[768];
	pm5544->grabPalette(palette, 0, 256);
	g_system->getPaletteManager()->setPalette(palette, 0, 256);

	g_system->copyRectToScreen(pm5544->surfacePtr()->getPixels(), pm5544->surfacePtr()->pitch, 0, 0, xres, yres);
	g_system->updateScreen();

	delete pm5544;

	// And display the error
	GUI::CountdownMessageDialog dialog(_("A test pattern should be displayed.\nDo you want to keep these shader scaler settings?"),
				10000,
				_("Yes"), _("No"), Graphics::kTextAlignCenter,
				_("Reverting automatically in %d seconds"));

	g_gui.displayTopDialogOnly(true);

	bool retval = (dialog.runModal() == GUI::kMessageOK);

	g_gui.displayTopDialogOnly(false);

	// Clear screen so we do not see any artefacts
	g_system->fillScreen(0);
	g_system->updateScreen();

	return retval;
}

} // End of namespace GUI
