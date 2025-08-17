#pragma once

#include <kernel/types.h>

#define cpuid(in,a,b,c,d) \
    asm volatile("cpuid" : "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "a" (in), "c" (0))

enum {
    CPU_FEATURE_FPU      = 1UL << 0,  // Floating-Point Unit
    CPU_FEATURE_VME      = 1UL << 1,  // Virtual 8086 Mode Extensions
    CPU_FEATURE_DE       = 1UL << 2,  // Debugging Extensions (CR4 bit 3)
    CPU_FEATURE_PSE      = 1UL << 3,  // Page Size Extension (CR4 bit 4)
    CPU_FEATURE_TSC      = 1UL << 4,  // Time Stamp Counter
    CPU_FEATURE_MSR      = 1UL << 5,  // Model-Specific Registers
    CPU_FEATURE_PAE      = 1UL << 6,  // Physical Address Extension (CR4 bit 5)
    CPU_FEATURE_MCE      = 1UL << 7,  // Machine Check Exception
    CPU_FEATURE_MCA      = 1UL << 8,  // Machine Check Architecture
    CPU_FEATURE_PGE      = 1UL << 9,  // Page Global Enable (CR4 bit 7)
    CPU_FEATURE_APIC     = 1UL << 10, // APIC support
    CPU_FEATURE_X2APIC   = 1UL << 11, // x2APIC support
    CPU_FEATURE_ACPI     = 1UL << 12, // Thermal control MSRs for ACPI
    CPU_FEATURE_SYSCALL  = 1UL << 13, // SYSCALL and SYSRET instructions
    CPU_FEATURE_MTRR     = 1UL << 14, // Memory Type Range Registers
    CPU_FEATURE_PAT      = 1UL << 15, // Page Attribute Table
    CPU_FEATURE_PSE36    = 1UL << 16, // 36-bit Page Size Extension
    CPU_FEATURE_MMX      = 1UL << 17, // MMX instructions
    CPU_FEATURE_MMXEXT   = 1UL << 18, // Extended MMX instructions
    CPU_FEATURE_SSE      = 1UL << 19, // Streaming SIMD Extensions (SSE)
    CPU_FEATURE_SSE2     = 1UL << 20, // SSE2 instructions
    CPU_FEATURE_SSE3     = 1UL << 21, // SSE3 instructions
    CPU_FEATURE_SSSE3    = 1UL << 22, // Supplemental SSE3 instructions
    CPU_FEATURE_SSE4_1   = 1UL << 23, // SSE4.1 instructions
    CPU_FEATURE_SSE4_2   = 1UL << 24, // SSE4.2 instructions
    CPU_FEATURE_AVX      = 1UL << 25, // Advanced Vector Extensions (AVX)
    CPU_FEATURE_AVX2     = 1UL << 26, // AVX2 instruction set
    CPU_FEATURE_FMA3     = 1UL << 27, // Fused Multiply-Add (FMA3)
    CPU_FEATURE_FMA4     = 1UL << 28, // 4-operand FMA instructions
    CPU_FEATURE_RDRAND   = 1UL << 29, // RDRAND instruction
    CPU_FEATURE_RDSEED   = 1UL << 30, // RDSEED instruction
    CPU_FEATURE_RDTSCP   = 1UL << 31, // RDTSCP instruction
    CPU_FEATURE_VMX      = 1UL << 32, // Virtual Machine eXtensions
    CPU_FEATURE_GBP      = 1UL << 33, // Support for 1GB pages
    CPU_FEATURE_CX8      = 1UL << 34, // CMPXCHG8B instruction
    CPU_FEATURE_CX16     = 1UL << 35, // CMPXCHG16B instruction
    CPU_FEATURE_CMOV     = 1UL << 36, // Conditional move instructions
    CPU_FEATURE_FXSR     = 1UL << 37, // FXSAVE and FXRSTOR instructions (CR4 bit 9)
    CPU_FEATURE_SVM      = 1UL << 38, // Secure Virtual Machine
    CPU_FEATURE_AES      = 1UL << 39, // AES instructions
    CPU_FEATURE_LA57     = 1UL << 40, // 5-Level Paging (57 address bits)
    CPU_FEATURE_BMI1     = 1UL << 41, // BMI1 instructions
    CPU_FEATURE_BMI2     = 1UL << 42, // BMI2 instructions
    CPU_FEATURE_XSAVE    = 1UL << 43, // XSAVE instructions
};

typedef struct {
    char vendor[13];
    char brand[49];
    uint32_t family;
    uint32_t model;
    uint32_t stepping;
    uint64_t flags;
    uint32_t max_extended;
    uint8_t maxphysaddr;
    uint8_t maxvirtaddr;
} cpuinfo_t;

int cpuid_feature(uint64_t);
int cpuid_maxphysaddr();
int cpuid_maxvirtaddr();
int cpuid_tsc_invariant();
void cpuid_init();
