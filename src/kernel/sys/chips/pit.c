#include "pit.h"
#include "../cpu/io.h"
#include "../../utils/alloc.h"
#include "../../utils/log.h"
#include <stddef.h>
#include <mem.h>

#define trace_pit(msg, ...) trace("PINT", msg, ##__VA_ARGS__)

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
    pit_callback_t* callback;
    callback = callbacks.start;
    while (callback != NULL)
    {
        if (callback->handler != NULL)
            callback->handler(frame);
        callback = callback->next;
    }
}

int pit_register_callback(isr_handler_t handler)
{
    if (callbacks.start == NULL)
    {
        callbacks.start = calloc(1, sizeof(pit_callback_t));
        if (callbacks.start == NULL)
        {
            trace_pit("Could not initialize PIT callback list");
            return -1;
        }
        callbacks.end = callbacks.start;
        callbacks.end->handler = handler;
    }
    else
    {
        callbacks.end->next = calloc(1, sizeof(pit_callback_t));
        if (callbacks.end->next == NULL)
        {
            trace_pit("Could not allocate next PIT callback entry");
            return -1;
        }
        callbacks.end = callbacks.end->next;
        callbacks.end->handler = handler;
    }
    return 0;
}

void pit_set_interval(uint64_t interval)
{
    uint64_t frequency;
    uint16_t divider;
    
    frequency = 1000 / interval;
    divider = (uint16_t) (PIT_BASE_FREQUENCY / frequency);
    port_out(PIT_CH0, (uint8_t) divider);
    port_wait();
    port_out(PIT_CH0, (uint8_t) (divider >> 8));
    port_wait();
}

void pit_init(void)
{
    memset(&callbacks, 0, sizeof(pit_callback_list_t));   
    port_out(PIT_COMM, PIT_CONFIG);
    port_wait();
    isr_register_handler(IRQ(0), &pit_handler);
}