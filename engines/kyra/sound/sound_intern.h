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

#ifndef KYRA_SOUND_INTERN_H
#define KYRA_SOUND_INTERN_H

#include "kyra/sound/sound.h"
#include "kyra/sound/sound_pc_v1.h"

#include "audio/midiparser.h"
#include "audio/miles.h"
#include "audio/softsynth/emumidi.h"
#include "audio/softsynth/fmtowns_pc98/towns_audio.h"

#include "common/mutex.h"

class TownsPC98_AudioDriver;

namespace Audio {
class PCSpeakerStream;
} // End of namespace Audio

namespace Kyra {

class MidiOutput;

/**
 * MIDI output device.
 *
 * This device supports both MT-32 MIDI, as used in
 * Kyrandia 1 and 2, and GM MIDI, as used in Kyrandia 2.
 */
class SoundMidiPC : public Sound {
public:
	SoundMidiPC(KyraEngine_v1 *vm, Audio::Mixer *mixer, MidiDriver *driver, kType type);
	~SoundMidiPC() override;

	kType getMusicType() const override { return _type; }

	bool init() override;

	void updateVolumeSettings() override;

	void initAudioResourceInfo(int set, void *info) override;
	void selectAudioResourceSet(int set) override;
	bool hasSoundFile(uint file) const override;
	void loadSoundFile(uint file) override;
	void loadSoundFile(const Common::Path &file) override;
	void loadSfxFile(const Common::Path &file) override;

	void playTrack(uint8 track) override;
	void haltTrack() override;
	bool isPlaying() const override;

	void playSoundEffect(uint16 track, uint8 volume = 0xFF) override;
	void stopAllSoundEffects() override;

	void beginFadeOut() override;

	void pause(bool paused) override;
private:
	static void onTimer(void *data);

	// Our channel handling
	int _musicVolume, _sfxVolume;

	uint32 _fadeStartTime;
	bool _fadeMusicOut;

	// Midi file related
	Common::Path _mFileName, _sFileName;
	byte *_musicFile, *_sfxFile;

	MidiParser *_music;
	MidiParser *_sfx[3];

	const SoundResourceInfo_PC *res() const {return _resInfo[_currentResourceSet]; }
	SoundResourceInfo_PC *_resInfo[3];
	int _currentResourceSet;

	// misc
	kType _type;
	Common::Path getFileName(const Common::Path &str);

	bool _nativeMT32;
	MidiDriver *_driver;
	Audio::MidiDriver_Miles_Midi *_output;

	Common::Mutex _mutex;
};

class SoundTownsPC98_v2 : public Sound {
public:
	SoundTownsPC98_v2(KyraEngine_v1 *vm, Audio::Mixer *mixer);
	~SoundTownsPC98_v2() override;

	kType getMusicType() const override { return _vm->gameFlags().platform == Common::kPlatformFMTowns ? kTowns : kPC98; }

	bool init() override;
	void process() override;

	void initAudioResourceInfo(int set, void *info) override;
	void selectAudioResourceSet(int set) override;
	bool hasSoundFile(uint file) const override;
	void loadSoundFile(uint file) override {}
	void loadSoundFile(const Common::Path &file) override;

	void playTrack(uint8 track) override;
	void haltTrack() override;
	void beginFadeOut() override;

	int32 voicePlay(const char *file, Audio::SoundHandle *handle, uint8 volume = 255, uint8 priority = 255, bool isSfx = true) override;
	void playSoundEffect(uint16 track, uint8 volume = 0xFF) override;

	void updateVolumeSettings() override;

private:
	Audio::AudioStream *_currentSFX;
	int _lastTrack;
	bool _useFmSfx;

	uint8 *_musicTrackData;
	uint8 *_sfxTrackData;
	TownsPC98_AudioDriver *_driver;

	const SoundResourceInfo_TownsPC98V2 *res() const { return _resInfo[_currentResourceSet]; }
	SoundResourceInfo_TownsPC98V2 *_resInfo[3];
	int _currentResourceSet;
};

// PC Speaker MIDI driver
class MidiDriver_PCSpeaker : public MidiDriver_Emulated {
public:
	MidiDriver_PCSpeaker(Audio::Mixer *mixer);
	~MidiDriver_PCSpeaker() override;

	// MidiDriver interface
	void close() override {}

	void send(uint32 data) override;

	MidiChannel *allocateChannel() override { return 0; }
	MidiChannel *getPercussionChannel() override { return 0; }

	// MidiDriver_Emulated interface
	void generateSamples(int16 *buffer, int numSamples) override;

	// AudioStream interface
	bool isStereo() const override { return false; }
	int getRate() const override { return _rate; }
private:
	Common::Mutex _mutex;
	Audio::PCSpeakerStream *_speaker;
	int _rate;

	struct Channel {
		uint8 pitchBendLow, pitchBendHigh;
		uint8 hold;
		uint8 modulation;
		uint8 voiceProtect;
		uint8 noteCount;
	} _channel[2];

	void resetController(int channel);

	struct Note {
		bool enabled;
		uint8 hardwareChannel;
		uint8 midiChannel;
		uint8 note;
		bool processHold;
		uint8 flags;
		uint8 hardwareFlags;
		uint16 priority;
		int16 modulation;
		uint16 precedence;
	} _note[2];

	void noteOn(int channel, int note);
	void noteOff(int channel, int note);

	void turnNoteOn(int note);
	void overwriteNote(int note);
	void turnNoteOff(int note);

	void setupTone(int note);

	uint16 _countdown;
	uint8 _hardwareChannel[1];
	bool _modulationFlag;

	uint8 _timerValue;
	void onTimer() override;

	static const uint8 _noteTable1[];
	static const uint8 _noteTable2[];
};

} // End of namespace Kyra

#endif
