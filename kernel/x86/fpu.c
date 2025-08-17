#include <kernel/x86/cpuid.h>
#include <kernel/x86/fpu.h>
#include <kernel/debug.h>
#include <string.h>

static int context_size = 512;
static int context_max_size = 512;
static uint8_t fxsave[512] __attribute__((aligned(64)));
int xsave_support = 0;

int fpu_xstate_size()
{
    // add 64 for alignment
    return context_size + 64;
}

void fpu_xstate_init(thread_t *thread)
{
    thread->xstate = thread->stack + 64;
    thread->xstate = (thread->xstate & -64UL);
    memcpy((void*)thread->xstate, fxsave, 32);
}

void fpu_init()
{
    static int init = 1;
    uint32_t eax, ebx, ecx, edx;
    uint64_t xcr0lo, xcr0hi;
    uint64_t cr4, cr0, cw;

    // Enable FPU features
    asm volatile("movq %%cr4, %0" : "=r"(cr4));
    asm volatile("movq %%cr0, %0" : "=r"(cr0));

    cr0 &= ~(1UL << 2);  // Clear EM bit (emulated)
    cr0 |=  (1UL << 5);  // Set NE bit (native exception)
    cr0 |=  (1UL << 1);  // Set MP bit (monitor co-processor)
    cr4 |=  (1UL << 9);  // Set OSFXSR bit (enable SSE support)
    cr4 |=  (1UL << 10); // Set OSXMMEXCPT bit (enable #XF exception)

    if(cpuid_feature(CPU_FEATURE_XSAVE))
    {
        cr4 |=  (1UL << 18); // Set OSXSAVE bit (enable xsave extension)
    }

    asm volatile("movq %0, %%cr4" :: "r"(cr4));
    asm volatile("movq %0, %%cr0" :: "r"(cr0));

    // Enable XSAVE features
    if(cpuid_feature(CPU_FEATURE_XSAVE))
    {
        asm volatile("xgetbv" : "=a"(xcr0lo), "=d"(xcr0hi) : "c"(0));

        xcr0lo |= (1UL << 0); // x87 FPU/MMX support (must be 1)
        xcr0lo |= (1UL << 1); // SSE support and XSAVE support XMM registers

        if(cpuid_feature(CPU_FEATURE_AVX))
        {
            xcr0lo |= (1UL << 2); // AVX enabled and XSAVE support for upper halves of YMM registers
        }

        asm volatile("xsetbv" :: "a"(xcr0lo), "d"(xcr0hi), "c"(0));

        // Context size
        if(init)
        {
            cpuid(0xD, eax, ebx, ecx, edx);
            xsave_support = 1;
            context_size = ebx;
            context_max_size = ecx;
            kp_info("fpu", "xsave: flags = %#x, context size = %d", xcr0lo, context_size);
        }
    }

    // Initialize FPU
    asm volatile("fninit");

    // Default control word
    cw = 0x037F;

    // Enable exceptions
    cw &= ~(1UL << 0); // Invalid Operation
    cw &= ~(1UL << 2); // Divide by Zero

    // Write control word
    asm volatile("fldcw %0" :: "m"(cw));

    // Save default state
    if(init)
    {
        asm volatile("fxsave %0" : "=m"(fxsave));
        init = 0;
    }
}
