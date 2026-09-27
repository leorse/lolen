MODULE := math

MODULE_OBJS := \
	angle.o \
	matrix3.o \
	matrix4.o \
	vector2d.o \
	vector3d.o \
	vector4d.o

# Include common rules
include $(srcdir)/rules.mk
