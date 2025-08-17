#include <kernel/x86/ioports.h>
#include <kernel/x86/cpuid.h>
#include <kernel/mem/mtrr.h>
#include <kernel/debug.h>

static uint8_t cap_vcnt;
static uint8_t cap_fix;
static uint8_t cap_wc;
static uint8_t cap_smrr;
static uint64_t mask;

static char *types[] = {
    "uncacheable",
    "write-combining",
    "reserved",
    "reserved",
    "write-through",
    "write-protected",
    "write-back"
};

static char *yesno[] = {
    "no",
    "yes"
};

void mtrr_write_default(uint32_t type, uint32_t fixed, uint32_t enabled)
{
    uint64_t reg;
    reg = (enabled << 11) | (fixed << 10) | type;
    write_msr(MTRR_DEF_TYPE, reg);
}

void mtrr_read_variable(uint8_t num, mtrr_t *m)
{
    uint64_t pb, pm;

    pb = read_msr(MTRR_PHYS_BASE(num));
    pm = read_msr(MTRR_PHYS_MASK(num));

    m->type = (pb & 0xFF);
    m->base = ((pb >> 12) & mask);
    m->mask = ((pm >> 12) & mask);
    m->valid = ((pm >> 11) & 0x01);

    m->start = (m->base << 12);
    m->length = ~m->mask;
    m->length = ((m->length & mask) << 12) | 0xFFF;
}

void mtrr_write_variable(uint8_t num, mtrr_t *m)
{
    uint64_t pb, pm;

    m->base = ((m->start >> 12) & mask);
    m->mask = ~m->length;
    m->mask = ((m->mask >> 12) & mask);

    pb = ((m->base << 12) | m->type);
    pm = ((m->mask << 12) | (m->valid << 11));

    write_msr(MTRR_PHYS_BASE(num), pb);
    write_msr(MTRR_PHYS_MASK(num), pm);
}

void mtrr_init()
{
    uint64_t reg;
    uint8_t e, fe, type;
    mtrr_t m;

    // Mask for physical address space
    mask = cpuid_maxphysaddr();
    mask = (1UL << mask) - 1;
    mask = mask >> 12;

    // Read capabilities
    reg = read_msr(MTRR_CAP);
    cap_vcnt = (reg & 0xFF);
    cap_fix = ((reg >> 8) & 0x01);
    cap_wc = ((reg >> 10) & 0x01);
    cap_smrr = ((reg >> 11) & 0x01);

    // Default memory type
    reg = read_msr(MTRR_DEF_TYPE);
    e = ((reg >> 11) & 0x01);
    fe = ((reg >> 10) & 0x01);
    type = (reg & 0xFF);

    // Log info
    kp_info("mtrr", "variable: %d, fixed: %s, write-combining: %s, smrr: %s", cap_vcnt, yesno[cap_fix], yesno[cap_wc], yesno[cap_smrr]);
    kp_info("mtrr", "default: %s, fixed enabled: %s, enabled: %s", types[type], yesno[fe], yesno[e]);

    // Read variable
    for(int n = 0; n < cap_vcnt; n++)
    {
        mtrr_read_variable(n, &m);
        if(m.valid)
        {
            kp_info("mtrr", "reg%d: start: %#lx, end: %#lx, type: %s", n, m.start, m.start+m.length, types[m.type]);
        }
    }
}
