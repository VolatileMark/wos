#include <syscall.h>

void main(void)
{
    syscall(0);
    while(1);
}