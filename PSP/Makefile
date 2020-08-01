TARGET = NikoTest
OBJS = NikoTest.o exports.o

PRX_EXPORTS=exports.exp
BUILD_PRX = 1
PSP_FW_VERSION=371
LARGE_MEMORY=1 

INCDIR = 
CFLAGS = -Os -G0 -Wall -fno-strict-aliasing -fno-builtin-printf
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =  
LIBS =
LDFLAGS = -mno-crt0 -nostartfiles 

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

