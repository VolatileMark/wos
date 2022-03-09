#ifndef __PORTS_H__
#define __PORTS_H__ 1

#include <stdint.h>

extern void port_out(uint16_t port_address, uint8_t data);
extern uint8_t port_in(uint16_t port_address);
extern void port_wait(void);

#endif