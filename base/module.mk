MODULE := base

MODULE_OBJS := \
	plugins.o \
	version.o

# Include common rules
include $(srcdir)/rules.mk
