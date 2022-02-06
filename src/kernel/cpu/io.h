#ifndef __PORTS_H__
#define __PORTS_H__ 1

#include <stdint.h>

extern void write_port(uint16_t portAddress, uint8_t dataByte);
extern uint8_t read_port(uint16_t portAddress);
extern void wait_for_ports(void);

#endif