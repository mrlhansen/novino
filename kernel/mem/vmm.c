#include <kernel/mem/vmm.h>
#include <kernel/mem/pmm.h>
#include <kernel/mem/e820.h>
#include <kernel/mem/slmm.h>
#include <kernel/debug.h>
#include <kernel/errno.h>
#include <string.h>

static int paging = 0;
static uint64_t pml4_phys;
static uint64_t pml4_virt;

#define shift(a) ((uint64_t)(a) >> 12)
#define virt2phys(a) ((uint64_t)(a) - 0xFFFFFFFF80000000)

extern char code;
extern char rodata;
extern char data;

static uint64_t alloc_frame()
{
    uint64_t addr;

    if(!paging)
    {
        addr = slmm_kmalloc(PAGE_SIZE, 1);
        addr = virt2phys(addr);
    }
    else
    {
        addr = pmm_alloc_frame();
    }

    return addr;
}

static void init_pde(pde_t *pde, int num, int mode, int ps)
{
    // Default PDE configuration
    pde[0].present = 0;
    pde[0].write = 1;
    pde[0].mode = mode;
    pde[0].pwt = 0;
    pde[0].pcd = 0;
    pde[0].accessed = 0;
    pde[0].zero = 0;
    pde[0].ps = ps;
    pde[0].avl = 0;
    pde[0].next_base = 0;
    pde[0].nx = 0;

    // Set all entries
    for(int i = 1; i < num; i++)
    {
        pde[i] = pde[0];
    }
}

static int alloc_pde(pde_t *pde, int ix)
{
    uint64_t phys;
    phys = alloc_frame();

    if(phys == 0)
    {
        return -ENOMEM;
    }

    pde[ix].present = 1;
    pde[ix].next_base = shift(phys);

    return 0;
}

static void link_pde(pde_t *pde, int ix, void *next)
{
    pde[ix].present = 1;
    pde[ix].next_base = shift(next);
}

static void init_pte(pte_t *pte, int num, int mode)
{
    // Default PTE configuration
    pte[0].present = 0;
    pte[0].write = 1;
    pte[0].mode = mode;
    pte[0].pwt = 0;
    pte[0].pcd = 0;
    pte[0].accessed = 0;
    pte[0].dirty = 0;
    pte[0].pat = 0;
    pte[0].global = 0;
    pte[0].avl = 0;
    pte[0].phys_addr = 0;
    pte[0].nx = 1;

    // Set all entries
    for(int i = 1; i < num; i++)
    {
        pte[i] = pte[0];
    }
}

static void link_pte(pte_t *pte, int ix, uint64_t phys)
{
    pte[ix].present = 1;
    pte[ix].avl = AVL_MAPPED;
    pte[ix].phys_addr = shift(phys);
}

static void free_pde_recurse(uint64_t virt, int level)
{
    uint64_t child;
    pde_t *pde;
    pte_t *pte;
    int max;

    pde = (pde_t*)virt;
    pte = (pte_t*)virt;
    max = ((level == 4) ? 256 : 512);
    virt = ((virt << 9) | 0xFFFF000000000000);

    for(int ix = 0; ix < max; ix++)
    {
        if(level == 1)
        {
            if(pte->avl == AVL_ALLOCATED)
            {
                pmm_free_frame(pte->phys_addr << 12);
            }
            pte++;
        }
        else
        {
            if(pde->present)
            {
                child = (virt | (ix << 12));
                free_pde_recurse(child, level-1);
                pmm_free_frame(pde->next_base << 12);
            }
            pde++;
        }
    }
}

static pte_t *get_page(uint64_t addr, int make)
{
    uint64_t ix4, ix3, ix2, ix1;
    pde_t *pl4, *pl3, *pl2;
    pte_t *pl1;

    ix4 = ((addr >> 39) & 0x1FF);
    ix3 = ((addr >> 30) & 0x1FF);
    ix2 = ((addr >> 21) & 0x1FF);
    ix1 = ((addr >> 12) & 0x1FF);

    pl4 = (pde_t*)(PL4_BASE);
    pl3 = (pde_t*)(PL3_BASE | ix4 << 12);
    pl2 = (pde_t*)(PL2_BASE | ix3 << 12 | ix4 << 21);
    pl1 = (pte_t*)(PL1_BASE | ix2 << 12 | ix3 << 21 | ix4 << 30);

    // Check PML4 entry
    if(pl4[ix4].present == 0)
    {
        if(make)
        {
            if(alloc_pde(pl4, ix4) == 0)
            {
                init_pde(pl3, 512, pl4[ix4].mode, 0);
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    // Check PDP entry
    if(pl3[ix3].present == 0)
    {
        if(make)
        {
            if(alloc_pde(pl3, ix3) == 0)
            {
                init_pde(pl2, 512, pl4[ix4].mode, 0);
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    // Check PD entry
    if(pl2[ix2].present == 0)
    {
        if(make)
        {
            if(alloc_pde(pl2, ix2) == 0)
            {
                init_pte(pl1, 512, pl4[ix4].mode);
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    // Fail for 2 MB pages
    if(pl2[ix2].ps)
    {
        return 0;
    }

    // Return the page
    asm volatile("invlpg %0" :: "m"(addr));
    return &pl1[ix1];
}

static pde2_t *get_large_page(uint64_t addr, int make)
{
    uint64_t ix4, ix3, ix2;
    pde_t *pl4, *pl3, *pl2;

    ix4 = ((addr >> 39) & 0x1FF);
    ix3 = ((addr >> 30) & 0x1FF);
    ix2 = ((addr >> 21) & 0x1FF);

    pl4 = (pde_t*)(PL4_BASE);
    pl3 = (pde_t*)(PL3_BASE | ix4 << 12);
    pl2 = (pde_t*)(PL2_BASE | ix3 << 12 | ix4 << 21);

    // Check PML4 entry
    if(pl4[ix4].present == 0)
    {
        if(make)
        {
            if(alloc_pde(pl4, ix4) == 0)
            {
                init_pde(pl3, 512, pl4[ix4].mode, 0);
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    // Check PDP entry
    if(pl3[ix3].present == 0)
    {
        if(make)
        {
            if(alloc_pde(pl3, ix3) == 0)
            {
                init_pde(pl2, 512, pl4[ix4].mode, 1);
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    // Fail for 4 KB pages
    if(pl2[ix2].ps == 0)
    {
        return 0;
    }

    // Return the page
    asm volatile("invlpg %0" :: "m"(addr));
    return (pde2_t*)&pl2[ix2];
}

static int map_large_page(uint64_t virt, uint64_t phys)
{
    pde2_t *pde;
    pde = get_large_page(virt, 1);

    if(pde == 0)
    {
        return -1;
    }

    if(pde->present)
    {
        return -1;
    }

    pde->present = 1;
    pde->avl = AVL_MAPPED;
    pde->phys_addr = (phys >> 21);

    return 0;
}

int vmm_set_caching(uint64_t virt, uint8_t type)
{
    pte_t *pte;
    pte = get_page(virt, 0);

    if(pte == 0)
    {
        return -1;
    }

    if(pte->present == 0)
    {
        return -1;
    }

    pte->pwt = ((type & 1) > 0);
    pte->pcd = ((type & 2) > 0);
    pte->pat = ((type & 4) > 0);

    return 0;
}

int vmm_set_mode(uint64_t virt, int w, int x)
{
    pte_t *pte;
    pte = get_page(virt, 0);

    if(pte == 0)
    {
        return -1;
    }

    if(pte->present == 0)
    {
        return -1;
    }

    pte->write = (w ? 1 : 0);
    pte->nx = (x ? 0 : 1);

    return 0;
}

int vmm_alloc_page(uint64_t virt)
{
    pte_t *pte;
    pte = get_page(virt, 1);

    if(pte == 0)
    {
        return -1;
    }

    if(pte->present)
    {
        return -1;
    }

    uint64_t phys;
    phys = alloc_frame();

    if(phys == 0)
    {
        return -ENOMEM;
    }

    pte->present = 1;
    pte->avl = AVL_ALLOCATED;
    pte->phys_addr = shift(phys);

    return 0;
}

int vmm_map_page(uint64_t virt, uint64_t phys)
{
    pte_t *pte;
    pte = get_page(virt, 1);

    if(pte == 0)
    {
        return -1;
    }

    if(pte->present)
    {
        return -1;
    }

    pte->present = 1;
    pte->avl = AVL_MAPPED;
    pte->phys_addr = shift(phys);

    return 0;
}

int vmm_free_page(uint64_t virt)
{
    pte_t *pte;
    pte = get_page(virt, 0);

    if(pte == 0)
    {
        return -1;
    }

    if(pte->present == 0)
    {
        return -1;
    }

    if(pte->avl == AVL_ALLOCATED)
    {
        pmm_free_frame(pte->phys_addr << 12);
    }

    init_pte(pte, 1, pte->mode);
    return 0;
}

void vmm_load_pml4(uint64_t addr)
{
    asm volatile("movq %0, %%cr3" :: "r"(addr));
}

void vmm_load_kernel_pml4()
{
    vmm_load_pml4(pml4_phys);
}

uint64_t vmm_get_kernel_pml4()
{
    return pml4_phys;
}

uint64_t vmm_get_current_pml4()
{
    uint64_t phys;
    asm volatile("mov %%cr3, %0" : "=r"(phys));
    return phys;
}

uint64_t vmm_phys_to_virt(uint64_t phys)
{
    return (IDMAP + phys);
}

uint64_t vmm_virt_to_phys(uint64_t virt)
{
    return (virt - IDMAP);
}

uint64_t vmm_create_user_space()
{
    uint64_t phys, virt;
    pde_t *new, *old;

    phys = alloc_frame();
    if(phys == 0)
    {
        return 0;
    }

    virt = vmm_phys_to_virt(phys);
    new = (pde_t*)virt;
    old = (pde_t*)pml4_virt;

    memcpy(new, old, PAGE_SIZE);
    link_pde(new, 510, (void*)phys);

    return phys;
}

void vmm_destroy_user_space()
{
    uint64_t phys;
    phys = vmm_get_current_pml4();
    free_pde_recurse(PL4_BASE, 4);
    vmm_load_kernel_pml4();
    pmm_free_frame(phys);
}

void vmm_init()
{
    uint64_t cs, rs, ws;
    uint64_t addr, end, ptc;
    pde_t *pml4, *pdp, *pd;
    pte_t *pt;

    // Create PDE levels for kernel
    pml4 = (pde_t*)alloc_frame();
    pdp = (pde_t*)alloc_frame();
    pd = (pde_t*)alloc_frame();

    init_pde(pml4, 512, KERNEL, 0);
    init_pde(pml4, 256, USER, 0);
    init_pde(pdp, 512, KERNEL, 0);
    init_pde(pd, 512, KERNEL, 0);

    link_pde(pml4, 510, pml4);
    link_pde(pml4, 511, pdp);
    link_pde(pdp, 510, pd);

    // Create remaining PDPs for kernel space
    for(int n = 256; n < 510; n++)
    {
        pdp = (pde_t*)alloc_frame();
        init_pde(pdp, 512, KERNEL, 0);
        link_pde(pml4, n, pdp);
    }

    // Memory range for kernel
    cs = virt2phys(&code);
    rs = virt2phys(&rodata);
    ws = virt2phys(&data);

    // TODO: Hardcoding the mapping of 16 PTs (32MB) is unreliable.
    // The amount needed depends on how much the SLMM uses.
    addr = 0;
    ptc = 16;

    // Map kernel memory
    for(int n = 0; n < ptc; n++)
    {
        pt = (pte_t*)alloc_frame();
        init_pte(pt, 512, KERNEL);
        link_pde(pd, n, pt);

        for(int m = 0; m < 512; m++)
        {
            link_pte(pt, m, addr);

            if(addr >= cs)
            {
                if(addr < rs)
                {
                    pt[m].nx = 0;
                }
                if(addr < ws)
                {
                    pt[m].write = 0;
                }
            }

            addr += PAGE_SIZE;
        }
    }

    // Load our new PML4
    pml4_phys = (uint64_t)pml4;
    pml4_virt = (IDMAP + pml4_phys);
    vmm_load_pml4(pml4_phys);

    // Identity map physical memory
    end = e820_report_end(0);
    addr = 0;
    while(addr < end)
    {
        map_large_page(IDMAP + addr, addr);
        addr += LARGE_PAGE_SIZE;
    }

    // Paging is enabled
    paging = 1;
}
