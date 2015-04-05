# PLATFORM = os4
# PLATFORM = win32
# PLATFORM = nix
# PLATFORM = aros
# PLATFORM = morphos
# PLATFORM = psp
# PLATFORM = pandora
# PLATFORM = haiku
PLATFORM = linux

# note: $(CC) is the c compiler for your target platform.
#       $(LOCALCC) is the c compiler for the platform you are compiling on

####### DEFAULT SETTINGS HERE #######
CFLAGS = -Wall -Werror -O3

CC = gcc
CXX = g++
AR = ar
RANLIB = ranlib
DEBUGLIB =
TARGET = giddy3
RM = rm
SLASH = /

LOCALCC = gcc
LOCALCXX = g++
TOOL_BDB = tools/bmp_de_bmp
BDBCFLAGS = -Wall -Werror

####### PLATFORM SPECIFIC STUFF HERE #######

# Amiga OS4
ifeq ($(PLATFORM),os4)
CFLAGS += -DMINIGL -mcrt=newlib -gstabs -I/SDK/Local/common/include/ -I/SDK/Local/common/include/SDL/ -I/SDK/Local/newlib/include/ -I/SDK/Local/newlib/include/SDL/
LFLAGS += `sdl-config --libs` -lSDL_mixer -lsmpeg -lvorbisfile -lvorbis -logg -lGL -lGLU -lGLUT -lm -mcrt=newlib -gstabs
BDBLFLAGS += -lunix -lm
endif

# I386 AROS (untested for now)
ifeq ($(PLATFORM),aros)
CFLAGS += -D__SW_SURFACE__
LFLAGS += -lSDL -lm
CC = i686-aros-gcc
LOCALCC = i686-aros-gcc
RANLIB = i686-aros-gcc
AR = i686-aros-ar
BDBLFLAGS = -lm
endif

# PPC MORPHOS (untested for now)
ifeq ($(PLATFORM),morphos)
CFLAGS += `sdl-config --cflags` -DUSED=""
LFLAGS += `sdl-config --libs` -lSDL_mixer -lm
CC = gcc
RANLIB = gcc
AR = ar
BDBLFLAGS = -lm
endif

# Windows 32bit
ifeq ($(PLATFORM),win32)
CFLAGS += -Dmain=SDL_main -IC:/MinGW/include/SDL
LFLAGS += -mwindows -lmingw32 -lSDLmain -lSDL -lSDL_Mixer -lopengl32 -lm
TARGET = giddy3.exe
TOOL_BDB = tools/bmp_de_bmp.exe
BDBLFLAGS += -lmingw32 -lm
RM = del
SLASH = $(subst	/,\,/)
endif

# Unix like (Posix)
ifeq ($(PLATFORM),nix)
LFLAGS += `sdl-config --libs` -lGL -lSDL_mixer -lm
CFLAGS += `sdl-config --cflags`
BDBLFLAGS = -lm
endif

# Haiku
ifeq ($(PLATFORM),haiku)
LFLAGS += `sdl-config --libs` -lGL -lSDL_mixer
CFLAGS += `sdl-config --cflags` -DNO_SDL_GLEXT
endif

# Pandora
ifeq ($(PLATFORM),pandora)
LFLAGS += `sdl-config --libs` -lGLES_CM -lEGL -lSDL_mixer -lm
CFLAGS += `sdl-config --cflags` 
CFLAGS += -DPANDORA -DHAVE_GLES -gdwarf-2 -g
CFLAGS += -mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -ftree-vectorize -fsingle-precision-constant -mno-unaligned-access -fdiagnostics-color=auto -fsigned-char
BDBLFLAGS = -lm
endif

# Linux
ifeq ($(PLATFORM),linux)
ifeq (x86_64,$(shell uname -m))
BASELIBDIR := lib64
CFLAGS += -m64
LFLAGS += -m64
else
BASELIBDIR := lib
CFLAGS += -m32
LFLAGS += -m32
endif
STRIP :=  $(CROSS_COMPILE)$(STRIP)
CFLAGS += -g $(shell PKG_CONFIG_PATH=/usr/$(BASELIBDIR)/pkgconfig pkg-config sdl --cflags)
LFLAGS += -L/usr/$(BASELIBDIR) $(shell PKG_CONFIG_PATH=/usr/$(BASELIBDIR)/pkgconfig pkg-config sdl --libs) -lSDL_mixer -lGL -lX11
endif

# PSP
ifeq ($(PLATFORM),psp)



#LFLAGS += `sdl-config --libs` -lGL -lSDL_mixer
#CFLAGS += `sdl-config --cflags`

CFLAGS += -D_CRAPPED_UP_SDL_MIXER_INSTALL_
CFLAGS += -D__PSP__
CFLAGS += -Ic:\pspsdk\psp\sdk\include

LFLAGS += -Lc:\pspsdk\psp\lib -lSDL_mixer -lvorbisidec -lmikmod -lSDL -lGL -lGLU -lGLUT -lpspvfpu -lm
LFLAGS += -Lc:\pspsdk\psp\sdk\lib -lpsppower
LFLAGS +=  -lpsplibc -lpsprtc
LFLAGS += -lpspdebug  -lpspgu -lpspctrl -lpspge  -lpspdisplay -lpsphprm -lpspsdk -lpsprtc -lpspaudio -lc -lpspuser -lpsputility -lpspkernel -lpspnet_inet


#CFLAGS += -D_CRAPPED_UP_SDL_MIXER_INSTALL_
#CFLAGS += -D__PSP__
#CFLAGS += -Ic:\pspsdk\psp\sdk\include
#LFLAGS += -Lc:\pspsdk\psp\lib -lSDL_mixer -lvorbisidec -lmikmod -lSDL -lGL -lGLU -lGLUT -lpspvfpu   
#LFLAGS += -Lc:\pspsdk\psp\sdk\lib -lpsppower
#LFLAGS += -Lc:\pspsdk\psp\sdk\lib -lpspdebug -lpspdisplay -lpspge -lpspctrl -lpspsdk -lc -lpspnet -lpspnet_inet -lpspnet_apctl -lpspnet_resolver -lpsputility -lpspuser -lpspkernel

BDBLFLAGS = -Lc:\pspsdk\psp\sdk\lib -lm -lpspdebug -lpspgu -lpspctrl -lpspge -lpspdisplay -lpsphprm -lpspsdk -lpsprtc -lpspaudio -lc -lpspuser -lpsputility -lpspkernel -lpspnet_inet

CC = psp-gcc
CXX = psp-g++
endif


####### SHOULDN'T HAVE TO CHANGE THIS STUFF #######

all: ../$(TARGET)

tools: $(TOOL_BDB)

strip_aros:
	i686-aros-strip --strip-unneeded --remove-section .comment giddy3

../$(TARGET): giddy3.o render.o tables.o specials.o enemies.o samples.o titles.o ../hats/sprites.bin ../hats/texts.bin ../onion1/sprites.bin ../onion1/blocks.bin ../onion1/psprites.bin ../onion2/sprites.bin ../onion2/blocks.bin ../onion2/psprites.bin ../onion3/sprites.bin ../onion3/blocks.bin ../onion3/psprites.bin ../onion4/sprites.bin ../onion4/blocks.bin ../onion4/psprites.bin ../onion5/sprites.bin ../onion5/blocks.bin ../onion5/psprites.bin ../hats/titlestuff.bin ../hats/victorystuff.bin
	$(CXX) -o ../$(TARGET) giddy3.o render.o tables.o specials.o enemies.o samples.o titles.o $(LFLAGS)

giddy3.o: giddy3.c giddy3.h render.h samples.h
	$(CC) -c giddy3.c -o giddy3.o $(CFLAGS)

render.o: render.c giddy3.h render.h specials.h enemies.h samples.h
	$(CC) -c render.c -o render.o $(CFLAGS)

specials.o: specials.c giddy3.h render.h specials.h samples.h
	$(CC) -c specials.c -o specials.o $(CFLAGS)

enemies.o: enemies.c giddy3.h render.h enemies.h samples.h
	$(CC) -c enemies.c -o enemies.o $(CFLAGS)

tables.o: tables.c render.h giddy3.h samples.h
	$(CC) -c tables.c -o tables.o $(CFLAGS)

samples.o: samples.c samples.h giddy3.h
	$(CC) -c samples.c -o samples.o $(CFLAGS)

titles.o: titles.c giddy3.h titles.h samples.h render.h
	$(CC) -c titles.c -o titles.o $(CFLAGS)

../hats/sprites.bin: data/sprites.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../hats/sprites.bin data/sprites.bmp

../onion1/sprites.bin: data/sprites1.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion1/sprites.bin data/sprites1.bmp

../onion1/blocks.bin: data/blocks1.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion1/blocks.bin data/blocks1.bmp

../onion1/psprites.bin: data/psprites1.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion1/psprites.bin data/psprites1.bmp

../onion2/sprites.bin: data/sprites2.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion2/sprites.bin data/sprites2.bmp

../onion2/blocks.bin: data/blocks2.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion2/blocks.bin data/blocks2.bmp

../onion2/psprites.bin: data/psprites2.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion2/psprites.bin data/psprites2.bmp

../onion3/sprites.bin: data/sprites3.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion3/sprites.bin data/sprites3.bmp

../onion3/blocks.bin: data/blocks3.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion3/blocks.bin data/blocks3.bmp

../onion3/psprites.bin: data/psprites3.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion3/psprites.bin data/psprites3.bmp

../onion4/sprites.bin: data/sprites4.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion4/sprites.bin data/sprites4.bmp

../onion4/blocks.bin: data/blocks4.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion4/blocks.bin data/blocks4.bmp

../onion4/psprites.bin: data/psprites4.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion4/psprites.bin data/psprites4.bmp

../onion5/sprites.bin: data/sprites5.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion5/sprites.bin data/sprites5.bmp

../onion5/blocks.bin: data/blocks5.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion5/blocks.bin data/blocks5.bmp

../onion5/psprites.bin: data/psprites5.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../onion5/psprites.bin data/psprites5.bmp

../hats/texts.bin: data/texts.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../hats/texts.bin data/texts.bmp

../hats/titlestuff.bin: data/titlestuff.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../hats/titlestuff.bin data/titlestuff.bmp

../hats/victorystuff.bin: data/victorystuff.bmp $(TOOL_BDB)
	$(TOOL_BDB) -o../hats/victorystuff.bin data/victorystuff.bmp

$(TOOL_BDB): tools/bmp_de_bmp.o
	$(LOCALCC) -o $(TOOL_BDB) tools/bmp_de_bmp.o $(BDBLFLAGS)

tools/bmp_de_bmp.o: tools/bmp_de_bmp.c
	$(LOCALCC) -c tools/bmp_de_bmp.c -o tools/bmp_de_bmp.o $(BDBCFLAGS)

psp_eboot:
	psp-fixup-imports ../$(TARGET)
	mksfo 'Giddy3 PSP' PARAM.sfo
	psp-strip ../$(TARGET) -o ../$(TARGET)_stripped.elf
	pack-pbp ../eboot.pbp param.sfo NULL NULL NULL NULL NULL ../$(TARGET)_stripped.elf NULL

cleanish:
	$(RM) *.o $(TOOL_BDB) tools$(SLASH)*.o *.bak
	strip ../$(TARGET)

#clean:
#	$(RM) ..$(SLASH)$(TARGET) *.bak *.o $(TOOL_BDB) tools$(SLASH)*.o ..$(SLASH)onion1$(SLASH)*.bin ..$(SLASH)onion2$(SLASH)*.bin ..$(SLASH)onion3$(SLASH)*.bin ..$(SLASH)onion4$(SLASH)*.bin ..$(SLASH)onion5$(SLASH)*.bin ..$(SLASH)hats$(SLASH)*.bin

clean:
	$(RM) ..$(SLASH)$(TARGET) *.bak *.o ..$(SLASH)onion1$(SLASH)*.bin ..$(SLASH)onion2$(SLASH)*.bin ..$(SLASH)onion3$(SLASH)*.bin ..$(SLASH)onion4$(SLASH)*.bin ..$(SLASH)onion5$(SLASH)*.bin ..$(SLASH)hats$(SLASH)*.bin
