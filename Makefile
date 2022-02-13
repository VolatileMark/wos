export PATH := $(abspath tools/x86_64-elf-cross/bin):$(PATH)

OS_NAME = wos

BUILD_DIR = $(abspath build/)
SOURCE_DIR = $(abspath src/)
DATA_DIR = $(abspath data/)

EMU = qemu-system-x86_64
DBG = gdb

EMU_FLAGS = -cdrom $(BUILD_DIR)/$(OS_NAME).iso \
			-m 256M \
			-cpu qemu64 \
			-vga std \
			-net none \
			-machine q35

EMU_DBG_FLAGS = -s -d guest_errors,cpu_reset,int -no-reboot -no-shutdown

DBG_FLAGS = -ex "target remote localhost:1234" \
			-ex "set confirm off" \
			-ex "set disassemble-next-line on" \
			-ex "set step-mode on" \
			-ex "set disassembly-flavor intel" \
			-ex "add-symbol-file $(BUILD_DIR)/bootstrap/bootstrap.elf" -ex "add-symbol-file $(BUILD_DIR)/kernel/kernel.elf"



.PHONY: all
all: build iso

.PHONY: build
build: build-intlibc build-bootstrap build-kernel

.PHONY: build-kernel
build-kernel:
	$(MAKE) -C $(SOURCE_DIR)/kernel SOURCE_DIR="$(SOURCE_DIR)/kernel" BUILD_DIR="$(BUILD_DIR)/kernel"

.PHONY: build-bootstrap
build-bootstrap:
	$(MAKE) -C $(SOURCE_DIR)/bootstrap SOURCE_DIR="$(SOURCE_DIR)/bootstrap" BUILD_DIR="$(BUILD_DIR)/bootstrap"

.PHONY: build-intlibc
build-intlibc:
	$(MAKE) -C $(SOURCE_DIR)/intlibc SOURCE_DIR="$(SOURCE_DIR)/intlibc" BUILD_DIR="$(BUILD_DIR)/intlibc"

.PHONY: run
run:
	$(EMU) $(EMU_FLAGS)

.PHONY: debug
debug:
	$(EMU) $(EMU_FLAGS) $(EMU_DBG_FLAGS) & $(DBG) $(DBG_FLAGS)

.PHONY: iso
iso: cfg-file
	cp -f $(BUILD_DIR)/bootstrap/bootstrap.elf $(BUILD_DIR)/iso/boot/wboot.elf
	cp -f $(BUILD_DIR)/kernel/kernel.elf $(BUILD_DIR)/iso/boot/wkernel.elf
	@rm -f $(BUILD_DIR)/$(OS_NAME).iso
	grub-mkrescue -o $(BUILD_DIR)/$(OS_NAME).iso $(BUILD_DIR)/iso

.PHONY: cfg-file
cfg-file:
	@mkdir -p $(BUILD_DIR)/iso/boot/grub
	printf "\
	set timeout=0\n\
	set default=0\n\
	menuentry "$(OS_NAME)" {\n\
		multiboot2 /boot/wboot.elf\n\
		module2 /boot/wkernel.elf\n\
		boot\n\
	}\n\
	" > $(BUILD_DIR)/iso/boot/grub/grub.cfg

.PHONY: clean
clean:
	find $(BUILD_DIR) -name '*.o' -delete
	find $(BUILD_DIR) -name '*.elf' -delete
	find $(BUILD_DIR) -name '*.bin' -delete
	find $(BUILD_DIR) -name '*.a' -delete
	rm -f $(BUILD_DIR)/initrd/initrd

.PHONY: clean-all
clean-all:
	rm -rf $(BUILD_DIR)