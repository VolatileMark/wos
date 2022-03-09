#ifndef __ISR_H__
#define __ISR_H__

#include <stdint.h>

#define DECLARE_ISR(index) extern void isr##index(void)
#define DECLARE_IRQ(index) extern void irq##index(void)
#define DECLARE_SFT(index) extern void sft##index(void)

#define IRQ(index) (32 + index)


/* ISRs */
DECLARE_ISR(0);
DECLARE_ISR(1);
DECLARE_ISR(2);
DECLARE_ISR(3);
DECLARE_ISR(4);
DECLARE_ISR(5);
DECLARE_ISR(6);
DECLARE_ISR(7);
DECLARE_ISR(8);
DECLARE_ISR(9);
DECLARE_ISR(10);
DECLARE_ISR(11);
DECLARE_ISR(12);
DECLARE_ISR(13);
DECLARE_ISR(14);
DECLARE_ISR(15);
DECLARE_ISR(16);
DECLARE_ISR(17);
DECLARE_ISR(18);
DECLARE_ISR(19);
DECLARE_ISR(20);
DECLARE_ISR(21);
DECLARE_ISR(22);
DECLARE_ISR(23);
DECLARE_ISR(24);
DECLARE_ISR(25);
DECLARE_ISR(26);
DECLARE_ISR(27);
DECLARE_ISR(28);
DECLARE_ISR(29);
DECLARE_ISR(30);
DECLARE_ISR(31);

/* IRQs */
DECLARE_IRQ(0);
DECLARE_IRQ(1);
DECLARE_IRQ(2);
DECLARE_IRQ(3);
DECLARE_IRQ(4);
DECLARE_IRQ(5);
DECLARE_IRQ(6);
DECLARE_IRQ(7);
DECLARE_IRQ(8);
DECLARE_IRQ(9);
DECLARE_IRQ(10);
DECLARE_IRQ(11);
DECLARE_IRQ(12);
DECLARE_IRQ(13);
DECLARE_IRQ(14);
DECLARE_IRQ(15);

DECLARE_SFT(128);


struct interrupt_info
{
    uint64_t interrupt_number;
    uint64_t error_code;
} __attribute__((packed));
typedef struct interrupt_info interrupt_info_t;

struct registers_state
{
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
} __attribute__((packed));
typedef struct registers_state registers_state_t;

struct stack_state
{
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed));
typedef struct stack_state stack_state_t;

typedef __attribute__((aligned(16))) uint8_t fpu_state_t[512];

struct interrupt_frame
{
    interrupt_info_t interrupt_info;
    registers_state_t registers_state;
    stack_state_t stack_state;
    fpu_state_t fpu_state;
} __attribute__((packed));
typedef struct interrupt_frame interrupt_frame_t;

typedef void (*isr_handler_t)(const interrupt_frame_t* info); 

void isr_init(void);
uint8_t isr_register_handler(uint8_t interrupt_number, isr_handler_t handler);

#endif