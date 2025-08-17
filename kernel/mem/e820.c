#include <kernel/mem/e820.h>
#include <kernel/mem/pmm.h>
#include <kernel/debug.h>
#include <string.h>

static e820_map_t e820;

// ACPI specs, Table 14-1, page 475
static char* e820_types[] = {
    "Available",
    "Reserved",
    "ACPI Reclaim",
    "ACPI NVS Memory",
    "Unuseable",
    "Disabled"
};

void e820_report_available()
{
    for(int i = 0; i < e820.num_regions; i++)
    {
        if(e820.region[i].type == E820_AVAILABLE)
        {
            pmm_set_available(e820.region[i].start, e820.region[i].length);
        }
    }
}

uint64_t e820_report_end(int avail)
{
    if(avail)
    {
        return e820.end_avail;
    }
    return e820.end_total;
}

void e820_init(size_t addr, size_t size)
{
    e820_region_t *region;
    uint64_t end;

    e820.end_total = 0;
    e820.end_avail = 0;
    e820.num_regions = size / sizeof(e820_region_t);
    e820.region = (e820_region_t*)addr;

    for(int i = 0; i < e820.num_regions; i++)
    {
        region = e820.region + i;
        end = region->start + region->length;

        if(region->type > 6)
        {
            region->type = E820_RESERVED; // mark unknown entries as reserved
        }

        if(end > e820.end_total)
        {
            e820.end_total = end;
        }

        if(end > e820.end_avail && region->type == E820_AVAILABLE)
        {
            e820.end_avail = end;
        }

        kp_info("e820", "%#016lx - %#016lx (%d kb, %s)", region->start, end, (region->length / 1024), e820_types[region->type-1]);
    }
}
