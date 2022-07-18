#include "guid.h"

int guidcmp(guid_t* g1, guid_t* g2)
{
    return 
    (
        g1->d1 == g2->d1 &&
        g1->d2 == g2->d2 &&
        g1->d3 == g2->d3 &&
        g1->d4 == g2->d4 && 
        g1->d5 == g2->d5
    );
}
