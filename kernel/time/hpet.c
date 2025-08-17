#include <kernel/time/hpet.h>
#include <kernel/time/time.h>
#include <kernel/acpi/acpi.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/vmm.h>
#include <kernel/x86/irq.h>
#include <kernel/debug.h>
#include <kernel/errno.h>

static uint64_t hpet_address;
static uint64_t tick_freq;
static uint32_t tick_period;
static uint8_t legacy_routing;
static uint8_t counter_size;
static uint8_t timer_count;
static uint8_t revision;
static hpet_timer_t *timer;

static uint64_t hpet_read(uint16_t reg)
{
    return *(volatile uint64_t*)(hpet_address + reg);
}

static void hpet_write(uint16_t reg, uint64_t value)
{
    *(volatile uint64_t*)(hpet_address + reg) = value;
}

static void hpet_stop()
{
    uint64_t value;

    value = hpet_read(HPET_CNFG);
    value = (value & ~3UL);

    hpet_write(HPET_CNFG, value);
}

static void hpet_start()
{
    uint64_t value;
    value = hpet_read(HPET_CNFG);

    if(legacy_routing)
    {
        value |= 0x03;
    }
    else
    {
        value |= 0x01;
    }

    hpet_write(HPET_CNFG, value);
}

static int hpet_system_timer()
{
    uint64_t value, freq;
    hpet_timer_t *tm;
    hpet_ccr_t *ccr;
    uint16_t offset;
    int gsi, vector;

    // Default values
    tm = 0;
    gsi = -1;

    // Timer with periodic support
    for(int n = 0; n < timer_count; n++)
    {
        if(timer[n].free && timer[n].periodic)
        {
            tm = timer+n;
            tm->free = 0;
            break;
        }
    }

    if(tm == 0)
    {
        return -ENXIO;
    }

    // Read CCR
    offset = (0x20 * tm->id);
    value = hpet_read(HPET_TM0_CCR + offset);
    ccr = (hpet_ccr_t*)&value;

    // Periodic mode
    ccr->int_type_cnf = 0;
    ccr->int_enb_cnf = 1;
    ccr->type_cnf = 1;
    ccr->val_set_cnf = 1;
    ccr->fsb_en_cnf = 0;

    // IRQ routing
    if(legacy_routing)
    {
        if(tm->id == 0)
        {
            gsi = irq_translate(0);
        }
        else if(tm->id == 1)
        {
            gsi = irq_translate(8);
        }
    }

    if(gsi < 0)
    {
        if(tm->route & (1 << 0))
        {
            gsi = 0;
        }
        else if(tm->route & (1 << 2))
        {
            gsi = 2;
        }
        else if(tm->route & (1 << 8))
        {
            gsi = 8;
        }
    }

    if(gsi < 0)
    {
        kp_warn("hpet", "Cannot route IRQ");
        return -EFAIL;
    }
    else
    {
        ccr->int_route_cnf = gsi;
    }

    // Setup IRQ handler
    vector = irq_alloc_gsi_vector(gsi);
    irq_request(vector, timer_handler, 0);

    // Write configuration
    hpet_write(HPET_TM0_CCR + offset, value);
    freq = (tick_freq / TIMER_FREQUENCY);
    value = hpet_read(HPET_CNT);
    hpet_write(HPET_TM0_CVR + offset, value + freq);
    hpet_write(HPET_TM0_CVR + offset, freq);

    // Success
    return 0;
}

uint64_t hpet_timestamp()
{
    uint64_t ts, rm;

    ts = hpet_read(HPET_CNT);
    rm = (ts % tick_freq);
    ts = TIME_NS * (ts / tick_freq);
    rm = (TIME_US * rm) / (tick_freq / TIME_MS);
    ts = ts + rm;

    return ts;
}

int hpet_init()
{
    uint64_t address, value;
    hpet_cap_t *cap;
    hpet_ccr_t *ccr;

    address = acpi_hpet_address();
    if(address == 0)
    {
        return -ENXIO;
    }
    else
    {
        hpet_address = vmm_phys_to_virt(address);
    }

    // Read capabilities
    value = hpet_read(HPET_CAP);
    cap = (hpet_cap_t*)&value;

    revision = cap->rev_id;
    tick_period = cap->counter_clk_period;
    legacy_routing = cap->leg_route_cap;
    timer_count = cap->num_tim_cap + 1;
    counter_size = cap->count_size_cap;
    tick_freq = TIME_FS / tick_period;

    // Log info
    kp_info("hpet", "mmio: %#lx, %d comparators, %s routing", address, timer_count, (legacy_routing ? "legacy" : "native"));
    kp_info("hpet", "%d-bit main counter, %lu Hz, %u ns", 32*(1+counter_size), tick_freq, tick_period/1000000);

    // Allocate memory
    timer = kcalloc(timer_count, sizeof(hpet_timer_t));
    if(timer == 0)
    {
        return -ENOMEM;
    }

    // Check all timers
    for(int n = 0; n < timer_count; n++)
    {
        value = hpet_read(HPET_TM0_CCR + (0x20 * n));
        ccr = (hpet_ccr_t*)&value;

        timer[n].id = n;
        timer[n].free = 1;
        timer[n].route = ccr->int_route_cap;
        timer[n].fsb = ccr->fsb_int_del_cap;
        timer[n].size = ccr->size_cap;
        timer[n].periodic = ccr->per_int_cap;
    }

    // Disable HPET
    hpet_stop();

    // Reset main counter
    hpet_write(HPET_CNT, 0);

    // Setup system timer
    if(hpet_system_timer())
    {
        return -EFAIL;
    }

    // Enable HPET
    hpet_start();

    // Success
    return 0;
}
