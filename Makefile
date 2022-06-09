export PATH := $(abspath tools/x86_64-qemu/bin):$(abspath tools/x86_64-elf-cross/bin):$(PATH)

OS_NAME = wos

BUILD_DIR = $(abspath build/)
SOURCE_DIR = $(abspath src/)
OVMF_DIR = $(abspath tools/ovmf-bins)

EMU = qemu-system-x86_64
DBG = gdb

MKSTANDALONE = $(abspath tools/x86_64-grub2/efi/bin/grub-mkstandalone)
MKRESCUE = $(abspath tools/x86_64-grub2/pc/bin/grub-mkrescue)

EMU_FLAGS_LEGACY = -drive file=$(BUILD_DIR)/$(OS_NAME).iso,format=raw \
				   -m 256M \
				   -cpu qemu64 \
				   -vga std \
				   -net none \
				   -machine q35

EMU_FLAGS_UEFI = -drive file=$(BUILD_DIR)/$(OS_NAME).img,format=raw \
				 -m 256M \
				 -cpu qemu64 \
				 -vga std \
				 -drive if=pflash,format=raw,unit=0,file="$(OVMF_DIR)/OVMF_CODE-pure-efi.fd",readonly=on \
				 -drive if=pflash,format=raw,unit=1,file="$(OVMF_DIR)/OVMF_VARS-pure-efi.fd" \
				 -net none \
				 -machine q35

EMU_DBG_FLAGS = -s -S -d guest_errors,cpu_reset,int -no-reboot -no-shutdown

DBG_FLAGS = -ex "target remote localhost:1234" \
			-ex "set confirm off" \
			-ex "set disassemble-next-line on" \
			-ex "set step-mode on" \
			-ex "set disassembly-flavor intel" \
			-ex "add-symbol-file $(BUILD_DIR)/test/test.elf" -ex "add-symbol-file $(BUILD_DIR)/kernel/kernel.elf"



.PHONY: default
default: legacy

.PHONY: legacy
legacy: build iso

.PHONY: uefi
uefi: build img



.PHONY: build
build: build-intlibc build-bootstrap build-kernel
	$(MAKE) -C $(SOURCE_DIR)/test SOURCE_DIR="$(SOURCE_DIR)/test" BUILD_DIR="$(BUILD_DIR)/test"

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
run: run-legacy

.PHONY: debug
debug: debug-legacy

.PHONY: run-legacy
run-legacy:
	$(EMU) $(EMU_FLAGS_LEGACY)

.PHONY: debug-legacy
debug-legacy:
	$(EMU) $(EMU_FLAGS_LEGACY) $(EMU_DBG_FLAGS) & $(DBG) $(DBG_FLAGS)

.PHONY: run-uefi
run-uefi:
	$(EMU) $(EMU_FLAGS_UEFI)

.PHONY: debug-uefi
debug-uefi:
	$(EMU) $(EMU_FLAGS_UEFI) $(EMU_DBG_FLAGS) & $(DBG) $(DBG_FLAGS)



.PHONY: iso
iso: $(BUILD_DIR)/iso/boot/grub/grub.cfg
	cp -f $(BUILD_DIR)/bootstrap/bootstrap.elf $(BUILD_DIR)/iso/boot/wboot.elf
	cp -f $(BUILD_DIR)/kernel/kernel.elf $(BUILD_DIR)/iso/boot/wkernel.elf
	cp -f $(BUILD_DIR)/test/test.elf $(BUILD_DIR)/iso/test.elf
	@rm -f $(BUILD_DIR)/$(OS_NAME).iso
	$(MKRESCUE) -o $(BUILD_DIR)/$(OS_NAME).iso $(BUILD_DIR)/iso

.PHONY: img
img: $(BUILD_DIR)/$(OS_NAME).img $(BUILD_DIR)/grub/BOOTX64.EFI $(BUILD_DIR)/img/boot/grub/grub.cfg
	@mkdir -p $(BUILD_DIR)/img/boot
	cp -f $(BUILD_DIR)/bootstrap/bootstrap.elf $(BUILD_DIR)/img/boot/wboot.elf
	cp -f $(BUILD_DIR)/kernel/kernel.elf $(BUILD_DIR)/img/boot/wkernel.elf
	mformat -i $< -F ::
	mmd -i $< ::/EFI
	mmd -i $< ::/EFI/BOOT
	mcopy -i $< $(BUILD_DIR)/grub/BOOTX64.EFI ::/EFI/BOOT
	mcopy -si $< $(BUILD_DIR)/img/* ::

$(BUILD_DIR)/$(OS_NAME).img: 
	@mkdir -p $(BUILD_DIR)
	dd if=/dev/zero of=$@ bs=512 count=93750

$(BUILD_DIR)/iso/boot/grub/grub.cfg:
	@mkdir -p $(dir $@)
	printf "\
	insmod all_video\n\
	set timeout=0\n\
	set default=0\n\
	menuentry "$(OS_NAME)" {\n\
		multiboot2 /boot/wboot.elf\n\
		module2 /boot/wkernel.elf\n\
		boot\n\
	}\n\
	" > $@

$(BUILD_DIR)/img/boot/grub/grub.cfg:
	@mkdir -p $(dir $@)
	printf "\
	insmod part_msdos\n\
	insmod all_video\n\
	set root=(hd0,msdos1)\n\
	set timeout=0\n\
	set default=0\n\
	menuentry "$(OS_NAME)" {\n\
		multiboot2 /boot/wboot.elf\n\
		module2 /boot/wkernel.elf\n\
		boot\n\
	}\n\
	" > $@

$(BUILD_DIR)/grub/grub-memdisk.cfg:
	@mkdir -p $(dir $@)
	printf "\
	insmod part_msdos\n\
	configfile (hd0,msdos1)/boot/grub/grub.cfg\n\
	" > $@

$(BUILD_DIR)/grub/BOOTX64.EFI: $(BUILD_DIR)/grub/grub-memdisk.cfg
	$(MKSTANDALONE) -O x86_64-efi -o $@ "boot/grub/grub.cfg=$<"



.PHONY: clean
clean:
	find $(BUILD_DIR) -name '*.o' -delete
	find $(BUILD_DIR) -name '*.so' -delete
	find $(BUILD_DIR) -name '*.elf' -delete
	find $(BUILD_DIR) -name '*.bin' -delete
	find $(BUILD_DIR) -name '*.a' -delete
	find $(BUILD_DIR) -name '*.cfg' -delete
	find $(BUILD_DIR) -name '*.EFI' -delete

.PHONY: clean-all
clean-all:
	rm -rf $(BUILD_DIR)



.PHONY: grub-version
grub-version:
	@$(MKRESCUE) --version
	@$(MKSTANDALONE) --version

.PHONY: qemu-version
qemu-version:
	@$(EMU) --version