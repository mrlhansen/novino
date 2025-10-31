#include <kernel/x86/ioports.h>
#include <kernel/x86/cpuid.h>
#include <kernel/debug.h>
#include <string.h>

#define PAT_MSR 0x277
#define PAT_RESET_VALUE  0x0007040600070406
#define PAT_CONFIG_VALUE 0x0007050100070406

static void pat_list(uint64_t pat, char *str)
{
    const char *name[] = {
        "UC", "WC", "??", "??", "WT", "WP", "WB", "UC-"
    };

    str[0] = '\0';

    for(int i = 0; i < 8; i++)
    {
        if(i > 0)
        {
            strcat(str, " ");
        }
        strcat(str, name[pat & 7]);
        pat >>= 8;
    }
}

void pat_init()
{
    uint64_t pat;
    char str[32];

    if(cpuid_feature(CPU_FEATURE_PAT) == 0)
    {
        kp_panic("pat", "not supported");
    }

    pat = read_msr(PAT_MSR);
    pat_list(pat, str);
    kp_info("pat", "old configuration (0-7): %s", str);

    pat = PAT_CONFIG_VALUE;
    write_msr(PAT_MSR, pat);
    pat_list(pat, str);
    kp_info("pat", "new configuration (0-7): %s", str);
}

void pat_load()
{
    if(cpuid_feature(CPU_FEATURE_PAT))
    {
        write_msr(PAT_MSR, PAT_CONFIG_VALUE);
    }
}
