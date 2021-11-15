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