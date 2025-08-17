#include <kernel/mem/e820.h>
#include <kernel/mem/slmm.h>
#include <kernel/mem/heap.h>
#include <kernel/mem/pmm.h>
#include <kernel/mem/vmm.h>
#include <kernel/debug.h>
#include <string.h>

#define INDEX(a)     ((a)/64)
#define OFFSET(a)    ((a)%64)
#define ADDRESS(a,b) (((a)*64+b)*PAGE_SIZE)
#define FRAME(a)     ((a)/PAGE_SIZE)

static uint64_t *bitmap;
static uint64_t bitmap_size;
static uint32_t max_index;
static uint32_t cur_index;

static uint32_t dma_addr;
static uint32_t dma_size;
static uint8_t dma_max;
static uint8_t dma_num;

static uint32_t free_pages;
static uint32_t num_pages;

void pmm_free_frame(uint64_t address)
{
    uint32_t frame, index, offset;
    uint64_t bit;

    frame = FRAME(address);
    index = INDEX(frame);
    offset = OFFSET(frame);
    bit = (1UL << offset);

    if(index >= max_index)
    {
        return;
    }

    if(cur_index > index)
    {
        cur_index = index;
    }

    if((bitmap[index] & bit) == 0)
    {
        bitmap[index] |= bit;
        free_pages++;
    }
}

void pmm_set_frame(uint64_t address)
{
    uint32_t frame, index, offset;
    uint64_t bit;

    frame = FRAME(address);
    index = INDEX(frame);
    offset = OFFSET(frame);
    bit = (1UL << offset);

    if(index >= max_index)
    {
        return;
    }

    if(bitmap[index] & bit)
    {
        bitmap[index] &= ~bit;
        free_pages--;
    }
}

uint64_t pmm_alloc_frame()
{
    uint64_t i, j;
    uint64_t address;

    for(i = cur_index; i < max_index; i++)
    {
        if(bitmap[i])
        {
            j = __builtin_ctzl(bitmap[i]);
            address = ADDRESS(i,j);
            pmm_set_frame(address);
            cur_index = i;
            return address;
        }
    }

    return 0;
}

uint64_t pmm_alloc_frames(uint32_t num, uint32_t align)
{
    uint64_t i, j, bit;
    uint32_t count = 0;
    uint64_t start = 0;

    if(num == 0)
    {
        return 0;
    }

    if(align == 0)
    {
        return 0;
    }
    else
    {
        align *= PAGE_SIZE;
    }

    for(i = cur_index; i < max_index; i++)
    {
        for(j = 0; j < 64; j++)
        {
            bit = (1UL << j);

            if(bitmap[i] & bit)
            {
                if(count == 0)
                {
                    start = ADDRESS(i,j);
                }

                if(start % align)
                {
                    count = 0;
                }
                else
                {
                    count++;
                }
            }
            else
            {
                count = 0;
            }

            if(count == num)
            {
                pmm_set_reserved(start, count * PAGE_SIZE);
                return start;
            }
        }
    }

    return 0;
}

uint64_t pmm_alloc_isa_dma()
{
    uint64_t addr = 0;

    if(dma_num < dma_max)
    {
        dma_num++;
        addr = dma_addr;
        dma_addr += dma_size;
    }

    return addr;
}

void pmm_set_available(uint64_t start, uint64_t size)
{
    uint64_t end = start + size;
    uint64_t addr = start;

    // Ignore small chunks
    if(size < 0x100000)
    {
        return;
    }

    // Align end address
    end &= ALIGN_MASK;

    // Align start address
    if(addr & ALIGN_TEST)
    {
        addr &= ALIGN_MASK;
        addr += PAGE_SIZE;
    }

    // Mark as available
    while(addr < end)
    {
        pmm_free_frame(addr);
        addr += PAGE_SIZE;
    }
}

void pmm_set_reserved(uint64_t start, uint64_t size)
{
    uint64_t end = start + size;
    uint64_t addr = start;

    // Align start address
    addr &= ALIGN_MASK;

    // Align end address
    if(end & ALIGN_TEST)
    {
        end &= ALIGN_MASK;
        end += PAGE_SIZE;
    }

    // Mark as reserved
    while(addr < end)
    {
        pmm_set_frame(addr);
        addr += PAGE_SIZE;
    }
}

void pmm_init()
{
    // Size of bitmap
    bitmap_size = e820_report_end(1);
    bitmap_size &= ALIGN_MASK;
    bitmap_size /= PAGE_SIZE;
    max_index = INDEX(bitmap_size);
    bitmap_size /= sizeof(uint64_t);

    // Allocate bitmap
    bitmap = kzalloc(bitmap_size);

    // Find available memory
    cur_index = 0;
    free_pages = 0;
    e820_report_available();
    num_pages = free_pages;
    slmm_report_reserved(1);

    // Set ISA DMA region
    dma_addr = 0x60000;
    dma_size = 0x10000;
    dma_max = 4;
    dma_num = 0;

    // Log info
    kp_info("pmm", "created %d kb bitmap, controlling %d pages", bitmap_size/1024, bitmap_size*8);
    kp_info("pmm", "free memory: %d pages (%d kb)", free_pages, free_pages*(PAGE_SIZE/1024));
}
