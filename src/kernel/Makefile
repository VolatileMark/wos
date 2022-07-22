CC = x86_64-elf-gcc
LD = x86_64-elf-ld
CP = x86_64-elf-objcopy
AC = nasm

BUILD_DIR ?= .
SOURCE_DIR = .

CSTD_LIB = $(abspath $(BUILD_DIR)/../cstd/cstd.a)
CSTD_INCLUDE = $(SOURCE_DIR)/../cstd

TARGET = kernel.elf

C_SRCS = $(shell find $(SOURCE_DIR) -name '*.c')
C_HDRS = $(shell find $(SOURCE_DIR) -name '*.h')
C_OBJS = $(patsubst $(SOURCE_DIR)%.c, $(BUILD_DIR)%.o, $(C_SRCS))

ASM_SRCS = $(shell find $(SOURCE_DIR) -name '*.asm')
ASM_OBJS = $(patsubst $(SOURCE_DIR)%.asm, $(BUILD_DIR)%_asm.o, $(ASM_SRCS))

FONT_SRC = $(SOURCE_DIR)/font.psf
FONT_OBJ = $(patsubst $(SOURCE_DIR)%.psf, $(BUILD_DIR)%.o, $(FONT_SRC))

LDS = $(SOURCE_DIR)/Linker.ld

AC_FLAGS = -I$(SOURCE_DIR) -f elf64 -F dwarf -g
CC_FLAGS = -I$(CSTD_INCLUDE) -g -std=c99 -m64 -Wall -Werror -ffreestanding -fstack-protector-all -mcmodel=kernel -mno-red-zone
LD_FLAGS = -T$(LDS) -z max-page-size=4096 -nostdlib



.PHONY: all
all: $(C_OBJS) $(ASM_OBJS) $(FONT_OBJ) $(CSTD_LIB)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LD_FLAGS) -o $(BUILD_DIR)/$(TARGET) $^

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(C_HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/%_asm.o: $(SOURCE_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AC) $(AC_FLAGS) -o $@ $<

$(FONT_OBJ): $(FONT_SRC)
	cd $(dir $<) && $(CP) -O elf64-x86-64 -B i386 -I binary $(notdir $<) $@
