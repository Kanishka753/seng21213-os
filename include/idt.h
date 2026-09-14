#ifndef IDT_H
#define IDT_H

#include "types.h"

/*
 * IDT entry for 32-bit protected mode.
 */
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

/*
 * IDT pointer used by lidt.
 */
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

/* 256 interrupt vectors */
#define IDT_ENTRIES 256

/* PIC ports */
#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

/* PIT ports */
#define PIT_CH0   0x40
#define PIT_CMD   0x43

/* PIT frequency */
#define PIT_HZ       100
#define PIT_DIVISOR  (1193182 / PIT_HZ)

/* IRQ numbers after PIC remapping */
#define IRQ_BASE 32
#define IRQ0      32

extern idt_entry_t idt[IDT_ENTRIES];

/*
 * Initialize the Interrupt Descriptor Table,
 * PIC and PIT timer.
 */
void idt_init(void);

/*
 * Initialize the PIT at PIT_HZ frequency.
 */
void pit_init(void);

/*
 * Send End Of Interrupt signal to the PIC.
 */
void pic_send_eoi(uint8_t irq);

void timer_handler(void);

#endif
