#ifndef __STRING_H__
#define __STRING_H__ 1

#include "stdint.h"

uint64_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char strcmp(const char* str1, const char* str2);
char strncmp(const char* str1, const char* str2, uint64_t bytes);
uint64_t strcspn(const char* str1, const char* str2);
char* strrev(char* str);
int atoi(const char* str);
char* itoa(long val, char* str, int base);
char* utoa(uint64_t val, char* str, int base);
char* utoan(uint64_t val, char* str, int base, int n);

#endif
