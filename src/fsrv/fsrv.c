#include <syscall.h>

#define SPPFUNC __attribute__((section(".exposed"))) void

int main(int argc, char** argv)
{
    while(1);
    return 0;
}