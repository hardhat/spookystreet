ZOS_PATH ?= ../Zeal-8-bit-OS
ZVB_SDK_PATH ?= ../Zeal-VideoBoard-SDK
BIN=bin/spookyst.bin
OBJ=obj/main.rel obj/game.rel obj/menu.rel obj/img.rel
IMG=img/tileset.zts
CC=sdcc
CFLAGS=-mz80 --std-c2x -c -I $(ZOS_PATH)/kernel_headers/sdcc/include/ -I $(ZVB_SDK_PATH)/include --codeseg TEXT --debug --opt-code-size
AS=sdasz80 -o -l -s
OBJCOPY=objcopy
LD=sdldz80
MAP=map/bg.ztm map/fg.ztm
LDFLAGS=-n -y -mjwx -i -b _HEADER=0x4000 -k $(ZOS_PATH)/kernel_headers/sdcc/lib -l z80 $(ZOS_LDFLAGS)
ZOS_LIBS=-k $(ZVB_SDK_PATH)/lib -l zvb_sound -l zvb_gfx
# 1. Detect the operating system
ifeq ($(OS),Windows_NT)
    # Windows settings (cmd.exe native commands)
    # Note: 'RM' uses 'del /q /f' and 'rmdir /s /q' to mimic 'rm -rf'
    RM = del /q /f
    RMDIR = rmdir /s /q
    CP = copy /y
    MKDIR = mkdir
    FIX_PATH = $(subst /,\,$1)
    EXE_EXT = .exe
else
    # Linux / macOS settings
    RM = rm -f
    RMDIR = rm -rf
    CP = cp
    MKDIR = mkdir -p
    FIX_PATH = $1
    EXE_EXT =
endif

all: init $(BIN)

PHONY: init clean reallyclean

init:
	-$(MKDIR) obj
	-$(MKDIR) bin

$(BIN): $(OBJ)
	$(LD) $(LDFLAGS) -o $(BIN:.bin=.ihx) \
	../Zeal-8-bit-OS/kernel_headers/sdcc/bin/zos_crt0.rel $(OBJ) $(ZOS_LIBS)
	$(OBJCOPY) --input-target=ihex --output-target binary $(BIN:.bin=.ihx) $(BIN)
	-$(CP) $(call FIX_PATH,$(BIN)) s

obj/%.rel: src/%.c
	$(CC) $(CFLAGS) -o $@ $<

obj/%.rel: src/%.asm
	$(AS) $@ $<

img/%.zts: img/%.gif
	python3 $(ZVB_SDK_PATH)/tools/zeal2gif/gif2zeal.py -i $<
	-$(CP) $(call FIX_PATH,$(IMG)) $(call FIX_PATH,bin/spookyst.zts)

map/bg.ztm: map/street.tmx
	python3 $(ZVB_SDK_PATH)/tools/tiled2zeal/tiled2zeal.py -i $< -o $@ -l 1
map/fg.ztm: map/street.tmx
	python3 $(ZVB_SDK_PATH)/tools/tiled2zeal/tiled2zeal.py -i $< -o $@ -l 2


obj/game.rel: src/game.c src/game.h src/main.h # map/tutorial_map.h
obj/menu.rel: src/menu.c src/menu.h src/game.h
obj/main.rel: src/main.c src/game.h src/menu.h src/img.h img/title.xbm
obj/img.rel: src/img.asm $(IMG) $(MAP)

clean:
	-$(RM) $(call FIX_PATH,$(OBJ))
	-$(RM) $(call FIX_PATH,$(BIN))
	-$(RM) $(call FIX_PATH,obj/*.lst obj/*.sym obj/*.adb obj/*.asm)
	-$(RM) $(call FIX_PATH,bin/*.ihx bin/*.noi bin/*.map bin/*.cdb bin/*.zts)

reallyclean: clean
	-$(RM) $(call FIX_PATH,img/*.zts img/*.ztp map/*.ztm)
