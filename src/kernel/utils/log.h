#ifndef __LOG_H__
#define __LOG_H__

#include "tty.h"

#define log(lvl, msg, ...) printf("[" lvl "] " msg "\n", ##__VA_ARGS__)

#define trace(invk, msg, ...) log(invk, "@ (%s): " msg, __FUNCTION__, ##__VA_ARGS__)
#define error(msg, ...) log("ERRO", "@ (%s:%d) " msg, __FILE__, __LINE__, ##__VA_ARGS__)
#define warning(msg, ...) log("WARN", "@ (%s:%d) " msg, __FILE__, __LINE__, ##__VA_ARGS__)
#define info(msg, ...) log("INFO", msg, ##__VA_ARGS__)

#endif