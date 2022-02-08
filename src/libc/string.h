#ifndef __STRING_H__
#define __STRING_H__ 1

#include "stdint.h"

uint64_t strlen(const char* str);
char strcmp(const char* str1, const char* str2);
char strncmp(const char* str1, const char* str2, uint64_t bytes);
uint64_t strcspn(const char* str1, const char* str2);
int atoi(const char* str);
char* itoa(int val, char* str, int base);

#endif