MODULE := audio

MODULE_OBJS := \
	adlib.o \
	audiostream.o \
	chip.o \
	cms.o \
	fmopl.o \
	mac_plugin.o \
	mididrv.o \
	mididrv_ms.o \
	midiparser_xmidi.o \
	midiparser.o \
	miles_midi.o \
	mixer.o \
	mpu401.o \
	mt32gm.o \
	musicplugin.o \
	null.o \
	rate.o \
	ym2149.o \
	timestamp.o \
	decoders/adpcm.o \
	decoders/g711.o \
	decoders/quicktime.o \
	decoders/raw.o \
	decoders/voc.o \
	decoders/vorbis.o \
	decoders/wave.o \
	mods/paula.o \
	softsynth/cms.o \
	softsynth/opl/dbopl.o \
	softsynth/opl/dosbox.o \
	softsynth/opl/mame.o \
	softsynth/appleiigs.o \
	softsynth/pcspk.o

ifdef USE_HMI_AUDIO
MODULE_OBJS += \
	effects/hmi/interfaces/envelope.o \
	effects/hmi/interfaces/filter1.o \
	effects/hmi/hmifxfp.o \
	effects/hmi/hmifxlib.o \
	effects/hmi/interfaces/mono_delay.o \
	effects/hmi/interfaces/phasor.o \
	effects/hmi/interfaces/resonator.o \
	effects/hmi/interfaces/reverb1.o \
	effects/hmi/interfaces/reverb2.o \
	effects/hmi/interfaces/ring_modulator.o \
	effects/hmi/interfaces/stereo_delay.o
endif

ifndef DISABLE_NUKED_OPL
MODULE_OBJS += \
	softsynth/opl/nuked.o
endif

ifdef USE_A52
MODULE_OBJS += \
	decoders/ac3.o
endif

ifdef USE_ALSA
MODULE_OBJS += \
	alsa_opl.o
endif

ifeq ($(BACKEND),atari)
MODULE_OBJS += \
	atari_ym2149.o
else
MODULE_OBJS += \
	softsynth/ym2149.o
endif

ifdef USE_FMTOWNS_PC98_AUDIO
MODULE_OBJS += \
	softsynth/fmtowns_pc98/pc98_audio.o \
	softsynth/fmtowns_pc98/pcm_common.o \
	softsynth/fmtowns_pc98/sega_audio.o \
	softsynth/fmtowns_pc98/towns_audio.o \
	softsynth/fmtowns_pc98/towns_euphony.o \
	softsynth/fmtowns_pc98/towns_pc98_driver.o \
	softsynth/fmtowns_pc98/towns_pc98_fmsynth.o \
	softsynth/fmtowns_pc98/towns_pc98_plugins.o
endif

ifdef USE_MPCDEC
MODULE_OBJS += \
	decoders/mpc.o
endif

ifdef USE_MT32EMU
MODULE_OBJS += \
	softsynth/mt32.o
endif

ifdef USE_SID_AUDIO
MODULE_OBJS += \
	softsynth/sid.o \
	softsynth/wave6581.o
endif

ifdef ENABLE_OPL2LPT
MODULE_OBJS += \
	opl2lpt.o
endif

ifdef USE_RETROWAVE
MODULE_OBJS += \
	rwopl3.o
endif

ifdef USE_NFM
MODULE_OBJS += \
	nfmopl.o
endif
	
ifdef USE_VGMTRANS_AUDIO
MODULE_OBJS += \
	soundfont/rawfile.o \
	soundfont/rifffile.o \
	soundfont/sf2file.o \
	soundfont/synthfile.o \
	soundfont/vgmcoll.o \
	soundfont/vgminstrset.o \
	soundfont/vgmitem.o \
	soundfont/vgmsamp.o \
	soundfont/vab/psxspu.o \
	soundfont/vab/vab.o
endif

# Include common rules
include $(srcdir)/rules.mk
