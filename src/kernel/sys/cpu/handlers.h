#ifndef __HANDLERS_H__
#define __HANDLERS_H__

#include "isr.h"
#include "../../utils/panic.h"

void handler_pf(const interrupt_frame_t* int_frame);

#endif
