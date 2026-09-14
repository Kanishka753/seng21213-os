#include "../include/idt.h"
#include "../include/types.h"
#include "../include/process.h"
#include "../include/scheduler.h"

extern void timer_isr(void);

/*
 * The actual IDT table.
 */
idt_entry_t idt[IDT_ENTRIES];

static idt_ptr_t idt_ptr;

/*
 * Load the IDT using the lidt instruction.
 */
static void idt_load(void)
{
    __asm__ __volatile__(
        "lidtl (%0)"
        :
        : "r" (&idt_ptr)
    );
}

/*
 * Set one IDT entry.
 */
static void idt_set_gate(uint8_t vector,
                         uint32_t handler,
                         uint16_t selector,
                         uint8_t type_attr)
{
    idt[vector].offset_low = (uint16_t)(handler & 0xFFFF);
    idt[vector].selector = selector;
    idt[vector].zero = 0;
    idt[vector].type_attr = type_attr;
    idt[vector].offset_high = (uint16_t)((handler >> 16) & 0xFFFF);
}

/*
 * Initialize all IDT entries.
 */
static void idt_clear(void)
{
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }
}

/*
 * Remap the 8259 PIC.
 *
 * Master IRQs:
 *   IRQ0-IRQ7 -> vectors 32-39
 *
 * Slave IRQs:
 *   IRQ8-IRQ15 -> vectors 40-47
 */
static void pic_remap(void)
{
    uint8_t mask1;
    uint8_t mask2;

    /* Save current masks. */
    __asm__ __volatile__(
        "inb %1, %0"
        : "=a"(mask1)
        : "Nd"((uint16_t)PIC1_DATA)
    );

    __asm__ __volatile__(
        "inb %1, %0"
        : "=a"(mask2)
        : "Nd"((uint16_t)PIC2_DATA)
    );

    /* Start initialization sequence. */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0x11),
          "Nd"((uint16_t)PIC1_CMD)
    );

    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0x11),
          "Nd"((uint16_t)PIC2_CMD)
    );

    /* Master PIC vector offset = 32. */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0x20),
          "Nd"((uint16_t)PIC1_DATA)
    );

    /* Slave PIC vector offset = 40. */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0x28),
          "Nd"((uint16_t)PIC2_DATA)
    );

    /* Tell master that slave is connected to IRQ2. */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0x04),
          "Nd"((uint16_t)PIC1_DATA)
    );

    /* Tell slave its cascade identity. */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0x02),
          "Nd"((uint16_t)PIC2_DATA)
    );

    /* Use 8086/88 mode. */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0x01),
          "Nd"((uint16_t)PIC1_DATA)
    );

    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0x01),
          "Nd"((uint16_t)PIC2_DATA)
    );

    /* Restore interrupt masks. */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"(mask1),
          "Nd"((uint16_t)PIC1_DATA)
    );

    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"(mask2),
          "Nd"((uint16_t)PIC2_DATA)
    );
}

/*
 * Initialize PIT channel 0.
 *
 * PIT base frequency = 1193182 Hz
 * Desired frequency = 100 Hz
 * Divisor = 11931
 */
void pit_init(void)
{
    uint16_t divisor = PIT_DIVISOR;

    /* Channel 0, lobyte/hibyte, mode 3. */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0x36),
          "Nd"((uint16_t)PIT_CMD)
    );

    /* Send low byte. */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)(divisor & 0xFF)),
          "Nd"((uint16_t)PIT_CH0)
    );

    /* Send high byte. */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)((divisor >> 8) & 0xFF)),
          "Nd"((uint16_t)PIT_CH0)
    );
}

/*
 * Send End Of Interrupt.
 */
void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8) {
        __asm__ __volatile__(
            "outb %0, %1"
            :
            : "a"((uint8_t)0x20),
              "Nd"((uint16_t)PIC2_CMD)
        );
    }

    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0x20),
          "Nd"((uint16_t)PIC1_CMD)
    );
}

/*
 * Initialize IDT + PIC + PIT.
 *
 * At this step we only configure the hardware.
 * The actual IRQ handler/context switching will
 * be added in the next step.
 */
void idt_init(void)
{
    __asm__ __volatile__("cli");

    idt_clear();

    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt;

    /*
     * IRQ0 = interrupt vector 32.
     *
     * 0x08 = kernel code segment.
     * 0x8E = present + ring 0 + 32-bit interrupt gate.
     */
    idt_set_gate(32,
                 (uint32_t)timer_isr,
                 0x08,
                 0x8E);

    pic_remap();
    pit_init();

    /*
     * Enable only IRQ0 (timer) and IRQ1 (keyboard)
     * on the master PIC.
     *
     * 0 = enabled
     * 1 = masked
     *
     * 0xFC = 11111100
     * IRQ0 = enabled
     * IRQ1 = enabled
     * IRQ2-7 = masked
     */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0xFE),
          "Nd"((uint16_t)PIC1_DATA)
    );

    /*
     * Mask all slave PIC IRQs.
     */
    __asm__ __volatile__(
        "outb %0, %1"
        :
        : "a"((uint8_t)0xFF),
          "Nd"((uint16_t)PIC2_DATA)
    );

    idt_load();
}
static volatile uint32_t timer_ticks = 0;

void timer_handler(void)
{
    timer_ticks++;

    scheduler_tick();
}
