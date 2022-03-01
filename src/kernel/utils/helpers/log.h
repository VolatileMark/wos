#ifndef __LOG_H__
#define __LOG_H__

#include <stdint.h>

void printf(const char* str, ...);
int init_logger(void);

void resize_logger_viewport(uint32_t width, uint32_t height);
void offset_logger_viewport(uint32_t x, uint32_t y);
void set_cursor_pos(uint32_t cx, uint32_t cy);

#define log(lvl, msg, ...) printf("[" lvl "] " msg "\n", ##__VA_ARGS__)

#define trace(invk, msg, ...) log(invk, "@ (%s:%d): " msg, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define error(msg, ...) log("ERRO", "@ (%s:%d) " msg, __FILE__, __LINE__, ##__VA_ARGS__)
#define warning(msg, ...) log("WARN", "@ (%s:%d) " msg, __FILE__, __LINE__, ##__VA_ARGS__)
#define info(msg, ...) log("INFO", msg, ##__VA_ARGS__)

#endif