#include "guid.h"
#include <string.h>

int guidcmp(guid_t* g1, guid_t* g2)
{
    return 
    (
        g1->d1 == g2->d1 &&
        g1->d2 == g2->d2 &&
        g1->d3 == g2->d3 &&
        !strncmp((char*) g1->d4, (char*) g2->d4, 8)
    );
}