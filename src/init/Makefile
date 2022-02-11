CC = x86_64-elf-gcc
LD = x86_64-elf-ld
AC = nasm

BUILD_DIR ?= .
SOURCE_DIR = .

TARGET = init

C_SRCS = $(shell find $(SOURCE_DIR) -name '*.c')
C_HDRS = $(shell find $(SOURCE_DIR) -name '*.h')
C_OBJS = $(patsubst $(SOURCE_DIR)%.c, $(BUILD_DIR)%.o, $(C_SRCS))

ASM_SRCS = $(shell find $(SOURCE_DIR) -name '*.asm')
ASM_OBJS = $(patsubst $(SOURCE_DIR)%.asm, $(BUILD_DIR)%_asm.o, $(ASM_SRCS))

LDS = $(SOURCE_DIR)/../libc/link.ld

AC_FLAGS = -I $(SOURCE_DIR) -f elf64 -F dwarf -g
CC_FLAGS = -g -std=c99 -m64 -Wall -Werror -ffreestanding -mno-red-zone -nostdinc -nostartfiles -nodefaultlibs -nostdlib
LD_FLAGS = -nostdlib -z max-page-size=4096 --entry=main



.PHONY: all
all: $(C_OBJS) $(ASM_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LD_FLAGS) --oformat elf64-x86-64 -o $(BUILD_DIR)/$(TARGET).elf $^

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(C_HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/%_asm.o: $(SOURCE_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AC) $(AC_FLAGS) -o $@ $<