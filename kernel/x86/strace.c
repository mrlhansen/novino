#include <kernel/x86/strace.h>
#include <kernel/debug.h>

static uint64_t symtab_start = 0;
static uint64_t symtab_end = 0;

static char *symtab_lookup(uint64_t address)
{
    uint64_t pos, start, end;
    struct symtab *curr;

    pos = symtab_start;
    curr = (void*)pos;

    while(pos < symtab_end)
    {
        start = curr->address;
        end = (curr->address + curr->size);

        if((address >= start) && (address < end))
        {
            return curr->name;
        }

        if(address < start)
        {
            return 0;
        }

        pos = (pos + curr->length);
        curr = (void*)pos;
    }

    return 0;
}

void strace_init(uint64_t address, uint32_t size)
{
    if(address > 0)
    {
        symtab_start = address;
        symtab_end = (address + size);
    }
}

void strace(int depth)
{
    const char *name;
    struct frame *frame;

    asm("movq %%rbp, %0" : "=r"(frame));
    kp_info("trace", "Running stack trace:");

    for(int i = 0; i < depth; i++)
    {
        if(frame->rbp == 0)
        {
            break;
        }
        name = symtab_lookup(frame->rip);
        kp_info("trace", "%02d: %#016lx %s", i, frame->rip, (name ? name : "???"));
        frame = frame->rbp;
    }
}
