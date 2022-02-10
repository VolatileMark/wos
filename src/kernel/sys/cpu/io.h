#ifndef __PORTS_H__
#define __PORTS_H__ 1

#include <stdint.h>

extern void write_to_port(uint16_t port_address, uint8_t data);
extern uint8_t read_from_port(uint16_t port_address);
extern void wait_for_ports(void);

#endif