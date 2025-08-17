#include <loader/multiboot2.h>
#include <loader/bootstruct.h>
#include <loader/common.h>
#include <string.h>

uint64_t kernel_address;
uint32_t kernel_size;
uint64_t symtab_address;
uint32_t symtab_size;
uint64_t kernel_entry;
bootstruct_t bs;

// Temporary Storage  : 0x2000000 (32MB)
// Loader Address     : 0x1000000 (16MB)
// Kernel Address     : 0x0100000 (1MB)
// Grub Modules       : 0x0100000 (1MB)

void register_region(uint64_t da, uint32_t ds, uint64_t sa, uint32_t ss)
{
    void *dp = (void*)(da + 0x2000000);
    void *sp = (void*)sa;
    memset(dp, 0, ds);
    memcpy(dp, sp, ss);
}

void move_regions()
{
    void *dp = (void*)bs.kernel_address;
    void *sp = (void*)(bs.kernel_address + 0x2000000);
    uint32_t size = bs.end_address - bs.kernel_address;
    memcpy(dp, sp, size);
}

uint64_t page_align(uint64_t value)
{
    if(value & ALIGN_TEST)
    {
        value &= ALIGN_MASK;
        value += PAGE_SIZE;
    }
    return value;
}

uint64_t next_address(uint64_t size)
{
    static uint64_t next = 0;
    uint64_t current = next;
    next = next + page_align(size);
    return current;
}

void parse_tag_cmdline(void *ptr)
{
    struct multiboot_tag_string *tag = ptr;
    strscpy(bs.cmdline, tag->string, sizeof(bs.cmdline));
}

void parse_tag_boot_loader_name(void *ptr)
{
    struct multiboot_tag_string *tag = ptr;
    strscpy(bs.signature, tag->string, sizeof(bs.signature));
}

void parse_tag_acpi(void *ptr)
{
    struct multiboot_tag_old_acpi *tag = ptr;
    uint64_t size, address;

    size = tag->size - sizeof(*tag);
    address = next_address(size);
    bs.rsdp_address = address;
    bs.rsdp_size = size;

    register_region(address, size, (uint64_t)tag->rsdp, size);
}

void parse_tag_mmap(void *ptr)
{
    struct multiboot_tag_mmap *tag = ptr;
    struct multiboot_mmap_entry *mmap = tag->entries;
    uint64_t address, mem_upper;
    uint32_t count, size, mem_lower;

    size = tag->size - sizeof(*tag);
    count = size / tag->entry_size;
    mem_upper = 0;
    mem_lower = 0;

    for(int i = 0; i < count; i++)
    {
        if(mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            if(mmap[i].addr < 0x100000)
            {
                mem_lower += mmap[i].len;
            }
            else
            {
                mem_upper += mmap[i].len;
            }
        }
    }

    address = next_address(size);
    bs.mmap_address = address;
    bs.mmap_size = size;
    bs.lower_memory = mem_lower;
    bs.upper_memory = mem_upper;

    register_region(address, size, (uint64_t)mmap, size);
}

void parse_tag_framebuffer(void *ptr)
{
    struct multiboot_tag_framebuffer *tag = ptr;

    if(tag->common.framebuffer_type == 1)
    {
        bs.lfb_mode = 1;
        bs.lfb_address = tag->common.framebuffer_addr;
        bs.lfb_width = tag->common.framebuffer_width;
        bs.lfb_height = tag->common.framebuffer_height;
        bs.lfb_pitch = tag->common.framebuffer_pitch;
        bs.lfb_bpp = tag->common.framebuffer_bpp;
        bs.lfb_red_index = tag->framebuffer_red_field_position;
        bs.lfb_red_mask = tag->framebuffer_red_mask_size;
        bs.lfb_green_index = tag->framebuffer_green_field_position;
        bs.lfb_green_mask = tag->framebuffer_green_mask_size;
        bs.lfb_blue_index = tag->framebuffer_blue_field_position;
        bs.lfb_blue_mask = tag->framebuffer_blue_mask_size;
    }
    else if(tag->common.framebuffer_type == 2)
    {
        bs.lfb_mode = 0;
        bs.lfb_address = tag->common.framebuffer_addr;
        bs.lfb_width = tag->common.framebuffer_width;
        bs.lfb_height = tag->common.framebuffer_height;
        bs.lfb_pitch = 0;
        bs.lfb_bpp = 0;
        bs.lfb_red_index = 0;
        bs.lfb_red_mask = 0;
        bs.lfb_green_index = 0;
        bs.lfb_green_mask = 0;
        bs.lfb_blue_index = 0;
        bs.lfb_blue_mask = 0;
    }
    else
    {
        asm("hlt");
    }
}

void parse_tag_module(void *ptr)
{
    struct multiboot_tag_module *tag = ptr;
    uint64_t size, address;

    size = (tag->mod_end - tag->mod_start);
    address = next_address(size);
    bs.initrd_address = address;
    bs.initrd_size = size;

    register_region(address, size, tag->mod_start, size);
}

void parse_all_tags(uint8_t *tagptr)
{
    struct multiboot_tag *tag;
    uint32_t size, r;

    tagptr = (tagptr + 8);
    tag = (struct multiboot_tag*)tagptr;

    while(tag->type)
    {
        r = (tag->size % 8);
        size = r ? (tag->size + 8 - r) : tag->size;

        switch(tag->type)
        {
            case MULTIBOOT_TAG_TYPE_CMDLINE:
                parse_tag_cmdline(tagptr);
                break;
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
                parse_tag_boot_loader_name(tagptr);
                break;
            case MULTIBOOT_TAG_TYPE_MODULE:
                parse_tag_module(tagptr);
                break;
            case MULTIBOOT_TAG_TYPE_MMAP:
                parse_tag_mmap(tagptr);
                break;
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
                parse_tag_framebuffer(tagptr);
                break;
            case MULTIBOOT_TAG_TYPE_ACPI_OLD:
                parse_tag_acpi(tagptr);
                break;
            case MULTIBOOT_TAG_TYPE_ACPI_NEW:
                parse_tag_acpi(tagptr);
                break;
            default:
                break;
        }

        tagptr += size;
        tag = (struct multiboot_tag*)tagptr;
    }
}

void handle_kernel(uint64_t address, uint32_t size)
{
    uint64_t vaddr, paddr;
    uint32_t filesz, memsz;
    elf64_ehdr *ehdr;
    elf64_phdr *phdr;
    elf64_shdr *shdr;

    // Set variables
    kernel_address = 0;
    kernel_size = 0;
    symtab_address = 0;
    symtab_size = 0;

    // Sections
    ehdr = (elf64_ehdr*)address;
    phdr = (elf64_phdr*)(address + ehdr->e_phoff);
    shdr = (elf64_shdr*)(address + ehdr->e_shoff);
    kernel_entry = ehdr->e_entry;

    // Find loadable sections
    for(int i = 0; i < ehdr->e_phnum; i++)
    {
        if(phdr->p_type == PT_LOAD)
        {
            if(kernel_address == 0)
            {
                kernel_address = phdr->p_vaddr;
            }

            vaddr = phdr->p_vaddr;
            paddr = address + phdr->p_offset;
            memsz = page_align(phdr->p_memsz);
            filesz = phdr->p_filesz;

            kernel_size = vaddr + memsz - kernel_address;
            register_region(vaddr, memsz, paddr, filesz);
        }
        phdr++;
    }

    next_address(kernel_address + kernel_size);

    // Build symbol table
    paddr = (uint64_t)&end;
    symtab_t *symtab = (void*)paddr;

    elf64_sym *sym = 0;
    char *strtab;
    int count;

    uint64_t prev = 0;
    uint64_t curr;
    int fcount = 0;
    int pos;

    for(int i = 0; i < ehdr->e_shnum; i++)
    {
        if(shdr[i].sh_type == SHT_SYMTAB)
        {
            pos = shdr[i].sh_link;
            strtab = (char*)(address + shdr[pos].sh_offset);
            count = shdr[i].sh_size / sizeof(elf64_sym);
            sym = (elf64_sym*)(address + shdr[i].sh_offset);
            break;
        }
    }

    if(sym == 0)
    {
        return;
    }

    for(int j = 0; j < count; j++)
    {
        if(ELF64_ST_TYPE(sym[j].st_info) == STT_FUNC)
        {
            fcount++;
        }
    }

    for(int i = 0; i < fcount; i++)
    {
        curr = ~0UL;

        for(int j = 0; j < count; j++)
        {
            if(ELF64_ST_TYPE(sym[j].st_info) == STT_FUNC)
            {
                if(sym[j].st_value > prev && sym[j].st_value < curr)
                {
                    curr = sym[j].st_value;
                    pos = j;
                }
            }
        }

        symtab->address = sym[pos].st_value;
        symtab->size = sym[pos].st_size;
        strcpy(symtab->name, strtab + sym[pos].st_name);
        symtab->length = sizeof(symtab_t) + strlen(symtab->name) + 1;

        symtab_size += symtab->length;
        symtab = (void*)(paddr + symtab_size);

        prev = curr;
    }

    symtab_address = next_address(symtab_size);
    register_region(symtab_address, symtab_size, paddr, symtab_size);
}

void kmain(uint8_t *tagptr)
{
    // Kernel information
    handle_kernel(kernel_elf_address, kernel_elf_size);

    // Bootstruct
    uint64_t size = sizeof(bootstruct_t);
    uint64_t addr = next_address(size);
    memset(&bs, 0, size);

    // Parse information
    parse_all_tags(tagptr);

    // Fill remaining information
    bs.kernel_address = kernel_address;
    bs.kernel_size = kernel_size;
    bs.symtab_address = symtab_address;
    bs.symtab_size = symtab_size;
    bs.end_address = next_address(0);
    register_region(addr, size, (uint64_t)&bs, size);

    // Copy all data
    move_regions();

    // Jump to kernel
    void (*kernel)(uint64_t);
    kernel = (void*)kernel_entry;
    kernel(addr);

    // We should never return here
    while(1);
}
