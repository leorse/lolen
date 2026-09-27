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

#ifndef OPTIONS_DIALOG_H
#define OPTIONS_DIALOG_H

#include "engines/metaengine.h"

#include "gui/dialog.h"
#include "common/str.h"
#include "audio/mididrv.h"

#ifdef USE_FLUIDSYNTH
#include "gui/fluidsynth-dialog.h"
#endif

#ifdef USE_CLOUD
#include "backends/cloud/storage.h"
#endif

namespace Common {
class RemapWidget;
}

namespace GUI {
class LauncherDialog;

class CheckboxWidget;
class EditTextWidget;
class PopUpWidget;
class SliderWidget;
class StaticTextWidget;
class TabWidget;
class ButtonWidget;
class CommandSender;
class GuiObject;
class RadiobuttonGroup;
class RadiobuttonWidget;
class PathWidget;
class ScrollContainerWidget;
class OptionsContainerWidget;

class OptionsDialog : public Dialog {
public:
	OptionsDialog(const Common::String &domain, int x, int y, int w, int h);
	OptionsDialog(const Common::String &domain, const Common::String &name);
	~OptionsDialog() override;

	void init();

	void open() override;
	virtual void apply();
	void close() override;
	void handleCommand(CommandSender *sender, uint32 cmd, uint32 data) override;
	void handleTickle() override;
	void handleOtherEvent(const Common::Event &event) override;

	const Common::String& getDomain() const { return _domain; }

	void reflowLayout() override;

protected:
	/** Config domain this dialog is used to edit. */
	Common::String _domain;

	ButtonWidget *_soundFontButton;
	PathWidget *_soundFont;
	ButtonWidget *_soundFontClearButton;

	virtual void build();
	virtual void clean();
	void rebuild();
	bool testGraphicsSettings();

	void addControlControls(GuiObject *boss, const Common::String &prefix);
	void addKeyMapperControls(GuiObject *boss, const Common::String &prefix, const Common::Array<Common::Keymap *> &keymaps, const Common::String &domain);
	void addAchievementsControls(GuiObject *boss, const Common::String &prefix);
	void addStatisticsControls(GuiObject *boss, const Common::String &prefix);
	void addGraphicControls(GuiObject *boss, const Common::String &prefix);
	void addAudioControls(GuiObject *boss, const Common::String &prefix);
	void addMIDIControls(GuiObject *boss, const Common::String &prefix);
	void addMT32Controls(GuiObject *boss, const Common::String &prefix);
	void addVolumeControls(GuiObject *boss, const Common::String &prefix);
	// The default value is the launcher's non-scaled talkspeed value. When SCUMM uses the widget,
	// it uses its own scale
	void addSubtitleControls(GuiObject *boss, const Common::String &prefix, int maxSliderVal = 255);

	void setGraphicSettingsState(bool enabled);
	void setAudioSettingsState(bool enabled);
	void setMIDISettingsState(bool enabled);
	void setMT32SettingsState(bool enabled);
	void setVolumeSettingsState(bool enabled);
	void setSubtitleSettingsState(bool enabled);

	void enableShaderControls(bool enabled);

	virtual void setupGraphicsTab();
	void updateScaleFactors(uint32 tag);

	bool loadMusicDeviceSetting(PopUpWidget *popup, Common::String setting, MusicType preferredType = MT_AUTO);
	void saveMusicDeviceSetting(PopUpWidget *popup, Common::String setting);

	TabWidget *_tabWidget;
	int _graphicsTabId;
	int _midiTabId;

	ScrollContainerWidget *_pathsContainer;

	PathWidget *_shader;
	ButtonWidget *_shaderClearButton;
	ButtonWidget *_updateShadersButton = nullptr;

private:

	//
	// Control controls
	//
	bool _enableControlSettings;

	CheckboxWidget *_touchpadCheckbox;

	StaticTextWidget *_kbdMouseSpeedDesc;
	SliderWidget *_kbdMouseSpeedSlider;
	StaticTextWidget *_kbdMouseSpeedLabel;
	StaticTextWidget *_joystickDeadzoneDesc;
	SliderWidget *_joystickDeadzoneSlider;
	StaticTextWidget *_joystickDeadzoneLabel;

	//
	// KeyMapper controls
	//
	Common::RemapWidget *_keymapperWidget;

	//
	// Graphics controls
	//
	bool _enableGraphicSettings;
	StaticTextWidget *_gfxPopUpDesc;
	PopUpWidget *_gfxPopUp;
	StaticTextWidget *_stretchPopUpDesc;
	PopUpWidget *_stretchPopUp;
	StaticTextWidget *_scalerPopUpDesc;
	PopUpWidget *_scalerPopUp, *_scaleFactorPopUp;
	ButtonWidget *_shaderButton;
	CheckboxWidget *_fullscreenCheckbox;
	CheckboxWidget *_filteringCheckbox;
	CheckboxWidget *_aspectCheckbox;
	CheckboxWidget *_vsyncCheckbox;
	StaticTextWidget *_rendererTypePopUpDesc;
	PopUpWidget *_rendererTypePopUp;
	StaticTextWidget *_antiAliasPopUpDesc;
	PopUpWidget *_antiAliasPopUp;
	StaticTextWidget *_renderModePopUpDesc;
	PopUpWidget *_renderModePopUp;
	StaticTextWidget *_rotationModePopUpDesc;
	PopUpWidget *_rotationModePopUp;

	//
	// Audio controls
	//
	bool _enableAudioSettings;
	StaticTextWidget *_midiPopUpDesc;
	PopUpWidget *_midiPopUp;
	StaticTextWidget *_oplPopUpDesc;
	PopUpWidget *_oplPopUp;

	StaticTextWidget *_mt32DevicePopUpDesc;
	PopUpWidget *_mt32DevicePopUp;
	StaticTextWidget *_gmDevicePopUpDesc;
	PopUpWidget *_gmDevicePopUp;

	//
	// MIDI controls
	//
	bool _enableMIDISettings;
	CheckboxWidget *_multiMidiCheckbox;
	StaticTextWidget *_midiGainDesc;
	SliderWidget *_midiGainSlider;
	StaticTextWidget *_midiGainLabel;

	//
	// MT-32 controls
	//
	bool _enableMT32Settings;
	CheckboxWidget *_mt32Checkbox;
	CheckboxWidget *_enableGSCheckbox;

	//
	// Subtitle controls
	//
	int getSubtitleMode(bool subtitles, bool speech_mute);
	bool _enableSubtitleSettings;
	bool _enableSubtitleToggle;
	StaticTextWidget *_subToggleDesc;
	RadiobuttonGroup *_subToggleGroup;
	RadiobuttonWidget *_subToggleSubOnly;
	RadiobuttonWidget *_subToggleSpeechOnly;
	RadiobuttonWidget *_subToggleSubBoth;
	static const char *_subModeDesc[];
	static const char *_lowresSubModeDesc[];
	StaticTextWidget *_subSpeedDesc;
	SliderWidget *_subSpeedSlider;
	StaticTextWidget *_subSpeedLabel;

	//
	// Volume controls
	//
	void updateMusicVolume(const int newValue) const;
	void updateSfxVolume(const int newValue) const;
	void updateSpeechVolume(const int newValue) const;
	bool _enableVolumeSettings;

	StaticTextWidget *_musicVolumeDesc;
	SliderWidget *_musicVolumeSlider;
	StaticTextWidget *_musicVolumeLabel;

	StaticTextWidget *_sfxVolumeDesc;
	SliderWidget *_sfxVolumeSlider;
	StaticTextWidget *_sfxVolumeLabel;

	StaticTextWidget *_speechVolumeDesc;
	SliderWidget *_speechVolumeSlider;
	StaticTextWidget *_speechVolumeLabel;

	CheckboxWidget *_muteCheckbox;

protected:
	//
	// Game GUI options
	//
	Common::String _guioptions;
	Common::String _guioptionsString;

	//
	// Backend controls
	//
	OptionsContainerWidget *_backendOptions;
};


} // End of namespace GUI

#endif
