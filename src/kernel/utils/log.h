#ifndef __LOG_H__
#define __LOG_H__

#include "tty.h"

#define log(lvl, msg, ...) tty_printf("[" lvl "] " msg "\n", ##__VA_ARGS__)

#define trace(invk, msg, ...) log("@2"invk"@0", "@ (%s): " msg, __FUNCTION__, ##__VA_ARGS__)
#define error(msg, ...) log("@1ERRO@0", "@ (%s:%d) " msg, __FILE__, __LINE__, ##__VA_ARGS__)
#define info(msg, ...) log("@3INFO@0", msg, ##__VA_ARGS__)

#endif