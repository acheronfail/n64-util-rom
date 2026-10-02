BUILD_DIR := build
.DEFAULT_GOAL := all
ifeq ($(N64_INST),)
$(error N64_INST is unset. Use ./tools/build-rom.sh or install libdragon)
endif
include $(N64_INST)/include/n64.mk
src := src/main.c src/app.c src/ui.c src/tone.c src/pak.c src/pak_write.c src/pak_n64.c
N64_CFLAGS += -Wall -Wextra -Werror
all: n64-util.z64
$(BUILD_DIR)/n64-util.elf: $(src:%.c=$(BUILD_DIR)/%.o)
n64-util.z64: N64_ROM_TITLE="N64 Utilities"
clean:
	rm -rf $(BUILD_DIR) n64-util.z64
-include $(wildcard $(BUILD_DIR)/src/*.d)
.PHONY: all clean
