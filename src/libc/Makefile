CC = x86_64-elf-gcc
AR = x86_64-elf-ar
AC = nasm

ROOT_BUILD_DIR ?= .
BUILD_DIR = $(ROOT_BUILD_DIR)/var-$(MAKECMDGOALS)
SOURCE_DIR = .

TARGET = libc.a

CC_FLAGS = -I $(SOURCE_DIR) -g -std=c99 -m64 -Wall -Werror -ffreestanding -mcmodel=kernel -mno-red-zone -nostdinc -nostartfiles -nodefaultlibs -nostdlib
AC_FLAGS = -I $(SOURCE_DIR) -f elf64 -F dwarf -g
AR_FLAGS = -rc --target=elf64-x86-64

ifeq ($(MAKECMDGOALS), utils)
CC_FLAGS += -DLIBC_UTILS_ONLY
endif

C_SRCS = $(shell find $(SOURCE_DIR) -name '*.c')
C_HDRS = $(shell find $(SOURCE_DIR) -name '*.h')
C_OBJS = $(patsubst $(SOURCE_DIR)%.c, $(BUILD_DIR)%.o, $(C_SRCS))

ASM_SRCS = $(shell find $(SOURCE_DIR) -name '*.asm' -not -iname 'crt0.asm')
ifeq ($(MAKECMDGOALS), full)
ASM_SRCS += $(shell find $(SOURCE_DIR) -iname 'crt0.asm')
endif
ASM_OBJS = $(patsubst $(SOURCE_DIR)%.asm, $(BUILD_DIR)%_asm.o, $(ASM_SRCS))

.PHONY: utils
utils: $(C_OBJS) $(ASM_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(AR) $(AR_FLAGS) $(BUILD_DIR)/$(TARGET) $^

.PHONY: nocrt
nocrt: $(C_OBJS) $(ASM_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(AR) $(AR_FLAGS) $(BUILD_DIR)/$(TARGET) $^

.PHONY: full
full: $(C_OBJS) $(ASM_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(AR) $(AR_FLAGS) $(BUILD_DIR)/$(TARGET) $^

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(C_HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) -c $< -o $@

$(BUILD_DIR)/%_asm.o: $(SOURCE_DIR)/%.asm
	@mkdir -p $(dir $@)
	$(AC) $(AC_FLAGS) -o $@ $<