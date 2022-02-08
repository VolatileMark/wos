#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "constants.h"

#define MIN_MEMORY (SIZE_1MB * 16)
#define INITRD_RELOC_ADDR (SIZE_1MB * 12)

#define KERNEL_ELF_PATH "./wkernel.elf"
#define FSRV_EXEC_FILE_PATH "./wfsrv.elf"
#define INIT_EXEC_FILE_PATH "./winit.elf"

#endif