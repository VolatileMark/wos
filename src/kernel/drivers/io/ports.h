#ifndef __PORTS_H__
#define __PORTS_H__ 1

#include <stdint.h>

extern void ports_write_byte(uint16_t portAddress, uint8_t dataByte);
extern uint8_t ports_read_byte(uint16_t portAddress);
extern void ports_wait(void);

#endif