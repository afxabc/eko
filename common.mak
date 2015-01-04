
#set 1 if make for debug
STAGE_DEBUG := 1

CROSSBUILD := /others/gcc-arm-2009q3/bin/arm-none-linux-gnueabi-
#LFLAGS := -static

DEBUG_DIR = ./debug
RELEASE_DIR = ./release

FLAGS_DEBUG = -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP
FLAGS_RELEASE = -O3 -Wall -c -fmessage-length=0 -MMD -MP

ifeq ($(STAGE_DEBUG),1)
  FLAGS = $(FLAGS_DEBUG)
  OUTPATH = $(DEBUG_DIR)
else
  FLAGS = $(FLAGS_RELEASE)
  OUTPATH = $(RELEASE_DIR)
endif

TARGETPATH = ../$(OUTPATH)
 
$(shell test -e $(OUTPATH) || mkdir $(OUTPATH))
$(shell test -e $(TARGETPATH) || mkdir $(TARGETPATH))

