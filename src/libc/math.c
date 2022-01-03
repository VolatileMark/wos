#include "math.h"

long ceil(double value)
{
    long integer = (long) value;
    double decimal = value - integer;
    if (decimal > 0)
        ++ integer;
    return integer;
}

long floor(double num)
{
    return (long) num;
}

uint64_t minu(uint64_t a, uint64_t b)
{
    return (a < b) ? a : b;
}

uint64_t maxu(uint64_t a, uint64_t b)
{
    return (a > b) ? a : b;
}