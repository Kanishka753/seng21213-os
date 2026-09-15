/* =============================================================================
 * SENG21213-OS :: Main Kernel  (Stage 0 – Foundations)
 * File   : kernel/kernel.c
 *
 * PURPOSE
 *   This is the heart of your operating system. Right now it:
 *     1. Initialises VGA text-mode display
 *     2. Initialises the keyboard driver
 *     3. Prints a splash screen
 *     4. Runs a minimal interactive shell ("ksh")
 *
 * ASSIGNMENT MILESTONES  (what YOU will add in later lectures)
 *   Lecture  9  – Process Management  →  process.h / process.c / scheduler.c
 *   Lecture 10  – Threads             →  thread.h  / thread.c
 *   Lecture 11  – Memory Management   →  pmm.h     / pmm.c / vmm.c
 *   Lecture 12  – File System         →  fs.h      / fs.c
 *
 * CODING CONVENTION
 *   - Prefix kernel-internal functions with k_ (e.g. k_strcmp)
 *   - All driver APIs live in their own .h/.c pair
 *   - NEVER call malloc – use the PMM you build in Lecture 11
 * ============================================================================*/

#include "vga.h"
#include "keyboard.h"
#include "../include/types.h"
#include "../include/idt.h"
#include "../include/process.h"
#include "../include/scheduler.h"
#include "../include/thread.h"

static void show_threads(void);

/* ---------------------------------------------------------------------------
 * Forward declarations of shell commands
 * --------------------------------------------------------------------------*/
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_echo(const char *args);
static void cmd_version(void);
static void cmd_colour(const char *args);
static void cmd_halt(void);

/* ---------------------------------------------------------------------------
 * Utility: minimal string helpers (no libc in a freestanding kernel!)
 * --------------------------------------------------------------------------*/
static int k_strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (uint8_t)*a - (uint8_t)*b;
}

static int k_strncmp(const char *a, const char *b, size_t n) {
    while (n-- && *a && (*a == *b)) { a++; b++; }
    return n == (size_t)-1 ? 0 : (uint8_t)*a - (uint8_t)*b;
}

static size_t k_strlen(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

/* Skip leading spaces */
static const char *k_ltrim(const char *s) {
    while (*s == ' ') s++;
    return s;
}

/* ---------------------------------------------------------------------------
 * Splash Screen
 * --------------------------------------------------------------------------*/
static void print_splash(void) {
    vga_clear(VGA_BLACK);

    /* Top banner box */
    vga_draw_box(0, 0, 7, 80, VGA_LIGHT_MAGENTA);

    vga_set_cursor(1, 2);
    vga_puts_color("  SENG21213-OS  |  Computer Architecture & Operating Systems",
                   VGA_YELLOW, VGA_BLACK);

    vga_set_cursor(2, 2);
    vga_puts_color("  Stage 0: Kernel Foundations", VGA_LIGHT_CYAN, VGA_BLACK);

    vga_set_cursor(3, 2);
    vga_puts_color("  Faculty of Engineering – Department of Software Engineering",
                   VGA_LIGHT_GREY, VGA_BLACK);

    vga_set_cursor(4, 2);
    vga_puts_color("  Built by students, for students.  Type 'help' to begin.",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    vga_set_cursor(5, 2);
    vga_puts_color("  CPU: i686 (32-bit Protected Mode)  |  Display: VGA 80x25",
                   VGA_DARK_GREY, VGA_BLACK);

    vga_set_cursor(8, 0);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("  Welcome! This kernel was compiled from source and booted entirely\n");
    vga_puts("  from bare metal. There is no Linux or Windows underneath – only\n");
    vga_puts("  the code you and your team write.\n");
    vga_puts("\n");
    vga_puts("  Assignment milestones to implement:\n");
    vga_puts_color("    [L09] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Process Management  – PCB, ready queue, round-robin scheduler\n");
    vga_puts_color("    [L10] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Threads & Sync      – kernel threads, mutex, semaphore\n");
    vga_puts_color("    [L11] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("Memory Management   – physical page allocator, virtual memory\n");
    vga_puts_color("    [L12] ", VGA_YELLOW, VGA_BLACK);
    vga_puts("File System         – RAM disk, FAT-like directory structure\n");
    vga_puts("\n");
}

/* ---------------------------------------------------------------------------
 * Shell command implementations
 * --------------------------------------------------------------------------*/
static void cmd_help(void) {
    vga_puts_color("\n  SENG21213-OS Shell Commands\n", VGA_YELLOW, VGA_BLACK);
    vga_puts("  ---------------------------------------------\n");
    vga_puts("  help              - Show this help message\n");
    vga_puts("  clear             - Clear the screen\n");
    vga_puts("  echo <text>       - Echo text to screen\n");
    vga_puts("  version           - Show kernel version\n");
    vga_puts("  colour <fg> <bg>  - Change text colour (0-15)\n");
    vga_puts("  halt              - Halt the CPU\n");
    vga_puts("  runthreads        - Run kernel threads continuously\n\n");
}

static void cmd_clear(void) {
    vga_clear(VGA_BLACK);
}

static void cmd_echo(const char *args) {
    vga_puts("  ");
    vga_puts(args);
    vga_puts("\n");
}

static void cmd_version(void) {
    vga_puts_color("\n  SENG21213-OS\n", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  Version: 0.1 - Stage 0\n");
    vga_puts("  Architecture: x86 32-bit Protected Mode\n\n");
}

static void cmd_colour(const char *args) {
    int fg = -1;
    int bg = -1;

    /* Read foreground colour */
    if (args[0] >= '0' && args[0] <= '9') {
        fg = args[0] - '0';

        if (args[1] >= '0' && args[1] <= '9') {
            fg = fg * 10 + (args[1] - '0');
        }
    }

    /* Skip foreground number */
    int i = 0;
    while (args[i] >= '0' && args[i] <= '9') {
        i++;
    }

    /* Skip spaces */
    while (args[i] == ' ') {
        i++;
    }

    /* Read background colour */
    if (args[i] >= '0' && args[i] <= '9') {
        bg = args[i] - '0';
        i++;

        if (args[i] >= '0' && args[i] <= '9') {
            bg = bg * 10 + (args[i] - '0');
        }
    }

    if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
        vga_puts("  Usage: colour <fg> <bg>  (0-15)\n");
        return;
    }

    vga_set_color((vga_color_t)fg, (vga_color_t)bg);
    vga_puts("  Colour changed.\n");
}

static void cmd_halt(void) {
    vga_puts_color("\n  System halted.\n", VGA_YELLOW, VGA_BLACK);

    __asm__ __volatile__("cli");

    while (true) {
        __asm__ __volatile__("hlt");
    }
}

static void cmd_ps(void)
{
    static const char *states[] = {
        "UNUSED",
        "READY",
        "RUNNING",
        "BLOCKED",
        "ZOMBIE"
    };

    vga_puts("\n");
    vga_puts("PID  NAME            STATE       TICKS\n");
    vga_puts("---------------------------------------\n");

    for (int i = 0; i < MAX_PROCS; i++) {

        if (proc_table[i].state == PROC_UNUSED) {
            continue;
        }

        vga_printf("%d", proc_table[i].pid);

        if (proc_table[i].pid < 10)
            vga_puts("    ");
        else if (proc_table[i].pid < 100)
            vga_puts("   ");
        else
            vga_puts("  ");

        vga_puts(proc_table[i].name);

        int name_len = 0;
        while (proc_table[i].name[name_len] != '\0') {
            name_len++;
        }

        while (name_len < 16) {
            vga_puts(" ");
            name_len++;
        }

        vga_puts(states[proc_table[i].state]);

        int state_len = 0;
        while (states[proc_table[i].state][state_len] != '\0') {
            state_len++;
        }

        while (state_len < 12) {
            vga_puts(" ");
            state_len++;
        }

        vga_printf("%u\n", proc_table[i].ticks);
    }

    vga_puts("\n");
}

/* ---------------------------------------------------------------------------
 * Shell process
 * --------------------------------------------------------------------------*/
static char  shell_buf[256];
static char  prompt[] = "\n  ksh> ";

static void shell_run(void) {
    vga_puts_color("\n  Kernel Shell ready. Type 'help' for commands.\n",
                   VGA_LIGHT_GREEN, VGA_BLACK);

    while (true) {
        vga_puts_color(prompt, VGA_LIGHT_GREEN, VGA_BLACK);
        kb_readline(shell_buf, sizeof(shell_buf));

        /* Trim leading whitespace */
        const char *cmd = k_ltrim(shell_buf);
        if (k_strlen(cmd) == 0) continue;

        /* Dispatch */
        if (k_strcmp(cmd, "help")  == 0) { cmd_help();  continue; }
        if (k_strcmp(cmd, "clear") == 0) { cmd_clear(); continue; }
        if (k_strcmp(cmd, "version") == 0) { cmd_version(); continue; }
        if (k_strncmp(cmd, "colour ", 7) == 0) { cmd_colour(k_ltrim(cmd + 7)); continue; }
        if (k_strcmp(cmd, "halt")    == 0) { cmd_halt();    continue; }       

        if (k_strncmp(cmd, "echo ", 5) == 0) {
            cmd_echo(k_ltrim(cmd + 5));
            continue;
        }
	if (k_strcmp(cmd, "ps") == 0) {
    	    cmd_ps();
     	    continue;
	}
	if (k_strcmp(cmd, "threads") == 0) {
            show_threads();
            continue;
        }

	if (k_strcmp(cmd, "runthreads") == 0) {
    	    vga_puts("Starting continuous thread test...\n");

            while (1) {
        	thread_yield();
    	    }
	}

        /* Milestone stubs */
        if (
            k_strcmp(cmd, "kill")    == 0 ||
            k_strcmp(cmd, "free")    == 0 ||
            k_strcmp(cmd, "ls")      == 0 ||
            k_strcmp(cmd, "cat")     == 0) {
            vga_puts_color("  [TODO] This command is not yet implemented.\n",
                           VGA_YELLOW, VGA_BLACK);
            vga_puts("  Implement it as part of your lecture assignment.\n");
            continue;
        }

        vga_puts_color("  Unknown command: ", VGA_LIGHT_RED, VGA_BLACK);
        vga_puts(cmd);
        vga_puts("\n  Type 'help' for a list of commands.\n");
    }
}

/* ---------------------------------------------------------------------------
 * Kernel entry point – called from kernel_entry.asm
 * --------------------------------------------------------------------------*/

static void idle_process(void)
{
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}

void process_one(void) {
    while (1) {
        for (volatile int i = 0; i < 1000000; i++);
    }
}

void process_two(void) {
    while (1) {
        for (volatile int i = 0; i < 1000000; i++);
    }
}

static void show_threads(void)
{
    vga_puts("TID   STATE\n");
    vga_puts("----------------\n");

    for (int i = 0; i < MAX_THREADS; i++) {
        if (thread_table[i].state == THREAD_UNUSED) {
            continue;
        }

        vga_puts("Thread ");

        if (thread_table[i].tid == 0) {
            vga_puts("0");
        } else if (thread_table[i].tid == 1) {
            vga_puts("1");
        } else if (thread_table[i].tid == 2) {
            vga_puts("2");
        } else if (thread_table[i].tid == 3) {
            vga_puts("3");
        } else if (thread_table[i].tid == 4) {
            vga_puts("4");
        } else if (thread_table[i].tid == 5) {
            vga_puts("5");
        } else if (thread_table[i].tid == 6) {
            vga_puts("6");
        } else if (thread_table[i].tid == 7) {
            vga_puts("7");
        }

        vga_puts("  ");

        switch (thread_table[i].state) {
            case THREAD_READY:
                vga_puts("READY");
                break;

            case THREAD_RUNNING:
                vga_puts("RUNNING");
                break;

            case THREAD_BLOCKED:
                vga_puts("BLOCKED");
                break;

            case THREAD_FINISHED:
                vga_puts("FINISHED");
                break;

            default:
                vga_puts("UNKNOWN");
                break;
        }

        vga_puts("\n");
    }
}

static void thread_one(void *arg)
{
    (void)arg;

    while (1) {
        vga_puts_color("T1 ", VGA_LIGHT_GREEN, VGA_BLACK);

        for (volatile int i = 0; i < 1000000; i++);
        thread_yield();
    }
}

static void thread_two(void *arg)
{
    (void)arg;

    while (1) {
        vga_puts_color("T2 ", VGA_LIGHT_CYAN, VGA_BLACK);

        for (volatile int i = 0; i < 1000000; i++);
        thread_yield();
    }
}

void kernel_main(void)
{
    vga_init();
    kb_init();

    /*
     * Create the initial process table.
     *
     * Process 0 = idle
     * Process 1 = test process
     * Process 2 = test process
     */
    proc_create("idle", idle_process);
    proc_create("proc1", process_one);
    proc_create("proc2", process_two);

    /*
     * Process 0 starts as the running process.
     */
     scheduler_init();

    /*
    * Initialize IDT, PIC and PIT.
    */
    
    thread_init();
    thread_create(thread_one, NULL);
    thread_create(thread_two, NULL);

    idt_init();

    /*
    * Enable CPU interrupts.
    */
    __asm__ __volatile__("sti");

    print_splash();

    vga_puts("\nStarting kernel threads...\n");

    for (volatile int i = 0; i < 1000000; i++);

    thread_yield();

    shell_run();

    __asm__ __volatile__("cli");

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
