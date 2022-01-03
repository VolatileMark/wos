#include "pit.h"
#include "../io/ports.h"
#include "../../cpu/interrupts.h"
#include <stddef.h>
#include <mem.h>
#include "../../utils/alloc.h"

#define PIT_BASE_FREQUENCY 1193182

#define PIT_CH0 0x40
#define PIT_CH1 0x41
#define PIT_CH2 0x42
#define PIT_COMM 0x43

#define PIT_CONFIG 0b00110110

typedef struct pit_callback
{
    isr_handler_t handler;
    struct pit_callback* next;
} pit_callback_t;

typedef struct
{
    pit_callback_t* start;
    pit_callback_t* end;
} pit_callback_list_t;

static pit_callback_list_t callbacks;

static void pit_handler(const interrupt_frame_t* frame)
{
    pit_callback_t* callback = callbacks.start;
    while (callback != NULL)
    {
        if (callback->handler != NULL)
            callback->handler(frame);
        callback = callback->next;
    }
}

int register_pit_callback(isr_handler_t handler)
{
    if (callbacks.start == NULL)
    {
        callbacks.start = calloc(1, sizeof(pit_callback_t));
        if (callbacks.start == NULL)
            return 1;
        callbacks.end = callbacks.start;
        callbacks.end->handler = handler;
    }
    else
    {
        callbacks.end->next = calloc(1, sizeof(pit_callback_t));
        if (callbacks.end->next == NULL)
            return 2;
        callbacks.end = callbacks.end->next;
        callbacks.end->handler = handler;
    }
    return 0;
}

void set_pit_interval(uint64_t interval)
{
    uint64_t frequency = 1000 / interval;
    uint16_t divider = (uint16_t) (PIT_BASE_FREQUENCY / frequency);
    ports_write_byte(PIT_CH0, (uint8_t) divider);
    ports_wait();
    ports_write_byte(PIT_CH0, (uint8_t) (divider >> 8));
    ports_wait();
}

void init_pit(void)
{
    memset(&callbacks, 0, sizeof(pit_callback_list_t));   
    ports_write_byte(PIT_COMM, PIT_CONFIG);
    ports_wait();
    isr_register_handler(IRQ(0), &pit_handler);
}