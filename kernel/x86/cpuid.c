#include <kernel/x86/cpuid.h>
#include <kernel/debug.h>
#include <string.h>

static cpuinfo_t cpu;

int cpuid_feature(uint64_t flag)
{
    if(cpu.flags & flag)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int cpuid_maxphysaddr()
{
    return cpu.maxphysaddr;
}

int cpuid_maxvirtaddr()
{
    return cpu.maxvirtaddr;
}

int cpuid_tsc_invariant()
{
    uint32_t eax, ebx, ecx, edx;

    if(cpuid_feature(CPU_FEATURE_TSC) == 0)
    {
        return 0;
    }

    if(cpu.max_extended < 0x80000007)
    {
        return 0;
    }

    cpuid(0x80000007, eax, ebx, ecx, edx);

    if(edx & (1 << 8))
    {
        return 1;
    }

    return 0;
}

static void check_set_flag(uint32_t reg, uint32_t bit, uint64_t flag)
{
    if(reg & (1 << bit))
    {
        cpu.flags |= flag;
    }
}

static void getbrand(char *str, int eax, int ebx, int ecx, int edx)
{
    for(int j = 0; j < 4; j++)
    {
        str[j] = eax >> (8 * j);
        str[j+4] = ebx >> (8 * j);
        str[j+8] = ecx >> (8 * j);
        str[j+12] = edx >> (8 * j);
    }
    str[16] = '\0';
}

static void cpuid_family()
{
    uint32_t eax, ebx, ecx, edx;
    uint32_t family, model, stepping;

    cpuid(1, eax, ebx, ecx, edx);
    model = (eax >> 4) & 0x0F;
    family = (eax >> 8) & 0x0F;
    stepping = eax & 0x0F;

    if(family == 15)
    {
        model |= ((eax >> 16) & 0x0F) << 4;
        family += (eax >> 20) & 0xFF;
    }

    if(family == 6)
    {
        model |= ((eax >> 16) & 0x0F) << 4;
    }

    cpu.family = family;
    cpu.model = model;
    cpu.stepping = stepping;
}

static void cpuid_vendor()
{
    char vendor[13];
    uint32_t eax, ebx, ecx, edx;

    cpuid(0, eax, ebx, ecx, edx);
    for(int i = 0; i < 4; i++)
    {
        vendor[i] = ebx >> (8 * i);
        vendor[i+4] = edx >> (8 * i);
        vendor[i+8] = ecx >> (8 * i);
    }

    vendor[12] = '\0';
    strcpy(cpu.vendor, vendor);
}

static void cpuid_brand()
{
    char brand[49], temp[17];
    uint32_t eax, ebx, ecx, edx;

    memset(brand, 0, 49);
    for(int id = 0x80000002; id <= 0x80000004; id++)
    {
        cpuid(id, eax, ebx, ecx, edx);
        getbrand(temp, eax, ebx, ecx, edx);
        strcat(brand, temp);
    }
    strtrim(cpu.brand, brand);
}

static void cpuid_flags()
{
    uint32_t eax, ebx, ecx, edx;

    cpu.flags = 0;

    // Standard flags
    cpuid(1, eax, ebx, ecx, edx);
    check_set_flag(edx,  0, CPU_FEATURE_FPU);
    check_set_flag(edx,  1, CPU_FEATURE_VME);
    check_set_flag(edx,  2, CPU_FEATURE_DE);
    check_set_flag(edx,  3, CPU_FEATURE_PSE);
    check_set_flag(edx,  4, CPU_FEATURE_TSC);
    check_set_flag(edx,  5, CPU_FEATURE_MSR);
    check_set_flag(edx,  6, CPU_FEATURE_PAE);
    check_set_flag(edx,  7, CPU_FEATURE_MCE);
    check_set_flag(edx,  8, CPU_FEATURE_CX8);
    check_set_flag(edx,  9, CPU_FEATURE_APIC);
    check_set_flag(edx, 12, CPU_FEATURE_MTRR);
    check_set_flag(edx, 13, CPU_FEATURE_PGE);
    check_set_flag(edx, 14, CPU_FEATURE_MCA);
    check_set_flag(edx, 15, CPU_FEATURE_CMOV);
    check_set_flag(edx, 16, CPU_FEATURE_PAT);
    check_set_flag(edx, 17, CPU_FEATURE_PSE36);
    check_set_flag(edx, 23, CPU_FEATURE_MMX);
    check_set_flag(edx, 24, CPU_FEATURE_FXSR);
    check_set_flag(edx, 25, CPU_FEATURE_SSE);
    check_set_flag(edx, 26, CPU_FEATURE_SSE2);
    check_set_flag(edx, 30, CPU_FEATURE_RDRAND);
    check_set_flag(ecx,  0, CPU_FEATURE_SSE3);
    check_set_flag(ecx,  5, CPU_FEATURE_VMX);
    check_set_flag(ecx,  9, CPU_FEATURE_SSSE3);
    check_set_flag(ecx, 12, CPU_FEATURE_FMA3);
    check_set_flag(ecx, 13, CPU_FEATURE_CX16);
    check_set_flag(ecx, 19, CPU_FEATURE_SSE4_1);
    check_set_flag(ecx, 20, CPU_FEATURE_SSE4_2);
    check_set_flag(ecx, 21, CPU_FEATURE_X2APIC);
    check_set_flag(ecx, 25, CPU_FEATURE_AES);
    check_set_flag(ecx, 26, CPU_FEATURE_XSAVE);
    check_set_flag(ecx, 28, CPU_FEATURE_AVX);

    // Structured extended flags
    cpuid(7, eax, ebx, ecx, edx);
    check_set_flag(ebx,  3, CPU_FEATURE_BMI1);
    check_set_flag(ebx,  5, CPU_FEATURE_AVX2);
    check_set_flag(ebx,  8, CPU_FEATURE_BMI2);
    check_set_flag(ecx, 16, CPU_FEATURE_LA57);
    check_set_flag(ebx, 18, CPU_FEATURE_RDSEED);

    // Extended flags
    cpuid(0x80000001, eax, ebx, ecx, edx);
    check_set_flag(edx, 11, CPU_FEATURE_SYSCALL);
    check_set_flag(edx, 22, CPU_FEATURE_MMXEXT);
    check_set_flag(edx, 26, CPU_FEATURE_GBP);
    check_set_flag(edx, 26, CPU_FEATURE_RDTSCP);
    check_set_flag(ecx,  2, CPU_FEATURE_SVM);
    check_set_flag(ecx, 16, CPU_FEATURE_FMA4);
}

static void cpuid_intel()
{
    // nothing at the moment
}

static void cpuid_amd()
{
    // nothing at the moment
}

static void cpuid_maxaddr()
{
    uint32_t eax, ebx, ecx, edx;

    cpuid(0x80000000, eax, ebx, ecx, edx);
    cpu.max_extended = eax;

    if(eax < 0x80000008)
    {
        if(cpuid_feature(CPU_FEATURE_PAE))
        {
            cpu.maxphysaddr = 36;
            cpu.maxvirtaddr = 32;
        }
        else
        {
            cpu.maxphysaddr = 32;
            cpu.maxvirtaddr = 32;
        }
    }
    else
    {
        cpuid(0x80000008, eax, ebx, ecx, edx);
        cpu.maxphysaddr = (eax & 0xFF);
        cpu.maxvirtaddr = ((eax >> 8) & 0xFF);
    }
}

void cpuid_init()
{
    // Get information
    cpuid_vendor();
    cpuid_brand();
    cpuid_flags();
    cpuid_family();
    cpuid_maxaddr();

    // AMD processor
    if(strcmp(cpu.vendor, "AuthenticAMD") == 0)
    {
        cpuid_amd();
    }

    // Intel processor
    if(strcmp(cpu.vendor, "GenuineIntel") == 0)
    {
        cpuid_intel();
    }

    // Print information
    kp_info("cpuid", "vendor: %s", cpu.vendor);
    kp_info("cpuid", "brand: %s", cpu.brand);
    kp_info("cpuid", "family: %d, model: %d, stepping: %d", cpu.family, cpu.model, cpu.stepping);
    kp_info("cpuid", "physaddr: %d bits, virtaddr: %d bits", cpu.maxphysaddr, cpu.maxvirtaddr);
}
