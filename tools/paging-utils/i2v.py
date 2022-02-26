#!/bin/python3

import sys

def main(idxs):
    addr = int(idxs[0])
    if addr >= 480:
        addr |= 0x1FFFE00
    addr <<= 9
    addr |= int(idxs[1])
    addr <<= 9
    addr |= int(idxs[2])
    addr <<= 9
    addr |= int(idxs[3])
    addr <<= 12
    print("VAddr:", hex(addr))

if __name__ == '__main__':
    main(sys.argv[1:])
