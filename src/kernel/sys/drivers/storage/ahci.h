#ifndef __AHCI_H__
#define __AHCI_H__

#include "disk-mgr.h"
#include <stdint.h>

#define AHCI_SECTOR_SIZE 512

int init_ahci_driver(void);

#endif