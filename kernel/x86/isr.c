#include <kernel/sched/process.h>
#include <kernel/x86/strace.h>
#include <kernel/x86/isr.h>
#include <kernel/x86/idt.h>
#include <kernel/x86/smp.h>
#include <kernel/debug.h>

static char *exception_messages[] =
{
    "Division by Zero",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range",
    "Invalid Opcode",
    "Device not Available",
    "Double Fault",
    "Reserved",
    "Invalid TSS",
    "Segment not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point",
    "Virtualization",
    "Control Protection",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection",
    "VMM Communication",
    "Security",
    "Reserved"
};

void isr_init()
{
    // exceptions (0-31)
    idt_set_gate(0, isr0);
    idt_set_gate(1, isr1);
    idt_set_gate(2, isr2);
    idt_set_gate(3, isr3);
    idt_set_gate(4, isr4);
    idt_set_gate(5, isr5);
    idt_set_gate(6, isr6);
    idt_set_gate(7, isr7);
    idt_set_gate(8, isr8);
    idt_set_gate(9, isr9);
    idt_set_gate(10, isr10);
    idt_set_gate(11, isr11);
    idt_set_gate(12, isr12);
    idt_set_gate(13, isr13);
    idt_set_gate(14, isr14);
    idt_set_gate(15, isr15);
    idt_set_gate(16, isr16);
    idt_set_gate(17, isr17);
    idt_set_gate(18, isr18);
    idt_set_gate(19, isr19);
    idt_set_gate(20, isr20);
    idt_set_gate(21, isr21);
    idt_set_gate(22, isr22);
    idt_set_gate(23, isr23);
    idt_set_gate(24, isr24);
    idt_set_gate(25, isr25);
    idt_set_gate(26, isr26);
    idt_set_gate(27, isr27);
    idt_set_gate(28, isr28);
    idt_set_gate(29, isr29);
    idt_set_gate(30, isr30);
    idt_set_gate(31, isr31);

    // system reserved (32-47)
    idt_set_gate(32, isr_schedule);
    idt_set_gate(39, isr_spurious);

    // system reserved (240-255)
    idt_set_gate(254, isr_tlbflush);
    idt_set_gate(255, isr_spurious);
}

static void isr_dump(isr_stack_t *stack)
{
    kp_warn("isr", "Exception: #%d (%s)", stack->int_no, exception_messages[stack->int_no]);
    kp_warn("isr", "Core: #%d", smp_core_id());

    if(stack->int_no >= 10 && stack->int_no <= 13)
    {
        kp_warn("isr", "Error information:");
        kp_warn("isr", "External:  %s", ((stack->error & 0x1) ? "Yes" : "No"));
        if(stack->error & 0x02)
        {
            kp_warn("isr", "IDT index: %d", (stack->error >> 3));
        }
        else
        {
            kp_warn("isr", "%s index: %d", ((stack->error & 0x4) ? "LDT" : "GDT"), (stack->error >> 3));
        }
    }
    else if(stack->int_no == 14)
    {
        uint64_t addr;
        asm volatile("mov %%cr2, %0" : "=r" (addr));

        kp_warn("isr", "Error information:");
        kp_warn("isr", "Page:     %s", ((stack->error & 0x1) ? "Page-level protection violation" : "Non-present page"));
        kp_warn("isr", "Access:   %s", ((stack->error & 0x2) ? "Write" : "Read"));
        kp_warn("isr", "Mode:     %s", ((stack->error & 0x4) ? "User" : "Kernel"));
        kp_warn("isr", "Reserved: %s", ((stack->error & 0x8) ? "Yes" : "No"));
        kp_warn("isr", "Fetch:    %s", ((stack->error & 0x10) ? "Yes" : "No"));
        kp_warn("isr", "Address:  %#016lx", addr);
    }
    else if(stack->int_no == 19)
    {
        uint32_t mxcsr;
        asm volatile("stmxcsr %0" : "=m"(mxcsr));

        kp_warn("isr", "Error information:");
        kp_warn("isr", "MXCSR: %#x", mxcsr);
    }

    kp_warn("isr", "Register dumps:");
    kp_warn("isr", "RAX: %016lx RBX: %016lx RCX: %016lx", stack->rax, stack->rbx, stack->rcx);
    kp_warn("isr", "RDX: %016lx RDI: %016lx RSI: %016lx", stack->rdx, stack->rdi, stack->rsi);
    kp_warn("isr", "RBP: %016lx RSP: %016lx RIP: %016lx", stack->rbp, stack->rsp, stack->rip);
    kp_warn("isr", "R8:  %016lx R9:  %016lx R10: %016lx", stack->r8, stack->r9, stack->r10);
    kp_warn("isr", "R11: %016lx R12: %016lx R13: %016lx", stack->r11, stack->r12, stack->r13);
    kp_warn("isr", "R14: %016lx R15: %016lx SS:  %016lx", stack->r14, stack->r15, stack->ss);
    kp_warn("isr", "CS:  %016lx RFLAGS: %016lx", stack->cs, stack->rflags);
}

void isr_handler(isr_stack_t *stack)
{
    process_t *pr;

    if(stack->cs == 0x10)
    {
        isr_dump(stack);
        strace(10);
        while(1)
        {
            asm("hlt");
        }
    }

    pr = process_handle();
    if(pr->pid)
    {
        isr_dump(stack);
        kp_warn("isr", "Terminating %s (pid %d)", pr->name, pr->pid);
        process_exit(137);
    }

    while(1)
    {
        asm("hlt");
    }
}
