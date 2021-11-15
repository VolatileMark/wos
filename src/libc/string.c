#include "string.h"

uint64_t strlen(const char* str)
{
    uint64_t len;
    const char* ptr;
    for 
    (
        ptr = str, len = 0; 
        *ptr != '\0'; 
        ptr ++, len ++
    );
    return len;
}

char strcmp(const char* str1, const char* str2)
{
    while
    (
        *str1 != '\0' &&
        *str2 != '\0' &&
        *str1 ++ == *str2 ++
    );
    return (*str1 - *str2);
}

char strncmp(const char* str1, const char* str2, uint64_t bytes)
{
    for 
    (
        ; 
        bytes > 0 && *str1 == *str2 && *str1 != '\0' && *str2 != '\0'; 
        bytes --
    );
    return (*str1 - *str2);
}

uint64_t strcspn(const char* str1, const char* str2)
{
    uint64_t i, j;
    for (i = 0; i < strlen(str1); i ++)
        for (j = 0; j < strlen(str2); j ++)
            if (str1[i] == str2[j])
                return i;
    return strlen(str1);
}