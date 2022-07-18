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
} pit_callback_list_entry_t;

typedef struct
{
    pit_callback_list_entry_t* head;
    pit_callback_list_entry_t* tail;
} pit_callback_list_t;

static pit_callback_list_t callbacks;

static void pit_handler(const interrupt_frame_t* frame)
{
    pit_callback_list_entry_t* callback;
    callback = callbacks.head;
    while (callback != NULL)
    {
        if (callback->handler != NULL)
            callback->handler(frame);
        callback = callback->next;
    }
}

int pit_register_callback(isr_handler_t handler)
{
    pit_callback_list_entry_t* callback;

    callback = malloc(sizeof(pit_callback_list_entry_t));
    if (callback == NULL)
    {
        trace_pit("Could register callback");
        return -1;
    }

    callback->handler = handler;
    callback->next = NULL;

    if (callbacks.tail == NULL)
        callbacks.head = callback;
    else
        callbacks.tail->next = callback;
    callbacks.tail = callback;

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
