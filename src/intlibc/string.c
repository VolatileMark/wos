#include "string.h"
#include "math.h"
#include "mem.h"

uint64_t strlen(const char* str)
{
    uint64_t len;
    const char* ptr;
    for 
    (
        ptr = str, len = 0; 
        *ptr != '\0'; 
        ptr++, len++
    );
    return len;
}

char strcmp(const char* str1, const char* str2)
{
    return strncmp(str1, str2, maxu(strlen(str1), strlen(str2)));
}

char strncmp(const char* str1, const char* str2, uint64_t bytes)
{
    for 
    (
        ; 
        bytes > 0 && *str1 != '\0' && *str2 != '\0' && *str1++ == *str2++; 
        bytes--
    );
    --str1; --str2;
    if (*str1 == *str2 && bytes > 0)
        return -1;
    return (*str1 - *str2);
}

uint64_t strcspn(const char* str1, const char* str2)
{
    uint64_t i, j;
    for (i = 0; i < strlen(str1); i++)
    {
        for (j = 0; j < strlen(str2); j++)
        {
            if (str1[i] == str2[j])
                return i;
        }
    }
    return strlen(str1);
}

char* strrev(char* str)
{
    char tmp;
    uint64_t i, j;
    uint64_t str_length = strlen(str) - 1;
    for (i = 0, j = str_length; i < j; i++, j--)
    {
        tmp = str[i];
        str[i] = str[j];
        str[j] = tmp;
    }
    return str;
}


int atoi(const char* str)
{
    uint64_t i, j;
    int out;

    i = 1;
    out = 0;
    for (j = strlen(str); j > 0; j--)
    {
        out += (str[j - 1] - '0') * i;
        i *= 10;
    }
    
    return out;
}

char* itoa(long val, char* str, int base)
{
    uint64_t i;
    char is_negative;
    long num;
    
    i = 0;
    is_negative = (val < 0);
    if (is_negative)
        val *= -1;

    do {
        num = (val % base);
        if (num >= 0xA)
        {
            num -= 0xA;
            str[i++] = num + 'A';
        }
        else
            str[i++] = num + '0';
    } while ((val /= base) > 0);

    if (is_negative)
        str[i++] = '-';
    
    str[i] = '\0';

    strrev(str);
    return str;
}

char* utoa(uint64_t val, char* str, int base)
{
    uint64_t i, num;

    i = 0;
    do {
        num = (val % base);
        if (num >= 0xA)
        {
            num -= 0xA;
            str[i++] = num + 'A';
        }
        else
            str[i++] = num + '0';
    } while ((val /= base) > 0);
    str[i] = '\0';

    strrev(str);
    return str;
}

char* utoan(uint64_t val, char* str, int base, int n)
{
    uint64_t i, num;

    memset(str, '0', n);

    i = 0;
    do {
        num = (val % base);
        if (num >= 0xA)
        {
            num -= 0xA;
            str[i++] = num + 'A';
        }
        else
            str[i++] = num + '0';
    } while ((val /= base) > 0 && i < n);
    str[n] = '\0';

    strrev(str);
    return str;
}

char* strcpy(char* dest, const char* src)
{
    memcpy((void*) dest, (void*) src, strlen(src) + 1);
    return dest;
}
