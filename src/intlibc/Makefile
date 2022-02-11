CC = x86_64-elf-gcc
AR = x86_64-elf-ar
AC = nasm

BUILD_DIR ?= .
SOURCE_DIR = .

TARGET = intlibc.a

CC_FLAGS = -I $(SOURCE_DIR) -g -std=c99 -m64 -Wall -Werror -ffreestanding -mcmodel=kernel -mno-red-zone -nostdinc -nostartfiles -nodefaultlibs -nostdlib
AC_FLAGS = -I $(SOURCE_DIR) -f elf64 -F dwarf -g
AR_FLAGS = -rc --target=elf64-x86-64

C_SRCS = $(shell find $(SOURCE_DIR) -name '*.c')
C_HDRS = $(shell find $(SOURCE_DIR) -name '*.h')
C_OBJS = $(patsubst $(SOURCE_DIR)%.c, $(BUILD_DIR)%.o, $(C_SRCS))

.PHONY: all
all: $(C_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(AR) $(AR_FLAGS) $(BUILD_DIR)/$(TARGET) $^

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(C_HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) -c $< -o $@