#ifndef __USTAR_H__
#define __USTAR_H__ 1

#include <stdint.h>
#include "../../sys/vfs.h"

int init_ustar(uint64_t addr, uint64_t size, vfs_t* vfs);

#endif