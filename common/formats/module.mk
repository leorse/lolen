MODULE := common/formats

MODULE_OBJS := \
	cue.o \
	iff_container.o \
	ini-file.o \
	markdown.o \
	quicktime.o \
	winexe.o \
	winexe_ne.o \
	winexe_pe.o \
	xmlparser.o

# Include common rules
include $(srcdir)/rules.mk
