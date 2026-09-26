MODULE := engines/kyra

MODULE_OBJS := \
	engine/items_lol.o \
	engine/kyra_rpg.o \
	engine/kyra_v1.o \
	engine/lol.o \
	engine/scene_lol.o \
	engine/scene_rpg.o \
	engine/sprites_lol.o \
	engine/sprites_rpg.o \
	engine/timer.o \
	engine/timer_lol.o \
	engine/timer_rpg.o \
	engine/util.o \
	graphics/animator_tim.o \
	graphics/screen.o \
	graphics/screen_lol.o \
	graphics/screen_v2.o \
	graphics/wsamovie.o \
	gui/debugger.o \
	gui/gui.o \
	gui/gui_lol.o \
	gui/gui_rpg.o \
	gui/gui_v1.o \
	gui/saveload.o \
	gui/saveload_lol.o \
	gui/saveload_rpg.o \
	metaengine.o \
	resource/resource.o \
	resource/resource_intern.o \
	resource/staticres.o \
	resource/staticres_lol.o \
	resource/staticres_rpg.o \
	script/script.o \
	script/script_lol.o \
	script/script_tim.o \
	script/script_v1.o \
	sequence/sequences_lol.o \
	sound/sound.o \
	sound/sound_lol.o \
	sound/sound_pc98_v2.o \
	sound/sound_pc_midi.o \
	sound/sound_pc_v1.o \
	sound/drivers/adlib.o \
	sound/drivers/audstream.o \
	sound/drivers/pcspeaker_v2.o \
	text/text.o \
	text/text_lol.o \
	text/text_rpg.o

# This module can be built as a plugin
ifeq ($(ENABLE_KYRA), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
