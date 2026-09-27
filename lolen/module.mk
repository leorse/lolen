MODULE := lolen

MODULE_OBJS := \
	application.o \
	splash.o

ifdef WIN32
MODULE_OBJS += \
	main.o
endif

# Include common rules
include $(srcdir)/rules.mk
