#!/bin/python3

import sys

def main(addr_str):
    addr = int(addr_str, 16)
    offset = addr & 0x0000000000000FFF
    addr >>= 12
    pt_i = addr & 0x00000000000001FF
    addr >>= 9
    pd_i = addr & 0x00000000000001FF
    addr >>= 9
    pdp_i = addr & 0x00000000000001FF
    addr >>= 9
    pml4_i = addr & 0x00000000000001FF
    print("PML4 index:", pml4_i)
    print("PDP index:", pdp_i)
    print("PD index:", pd_i)
    print("PT index:", pt_i)
    print("Page offset:", hex(offset))

if __name__ == '__main__':
    main(sys.argv[1])
