#ifndef LOADER_COMMON_H
#define LOADER_COMMON_H

#include <stdint.h>

#define ALIGN_TEST 0xFFF
#define ALIGN_MASK 0xFFFFFFFFFFFFF000
#define PAGE_SIZE  0x1000

extern uint64_t kernel_elf_size;
extern uint64_t kernel_elf_address;
extern char end;

typedef struct {
    uint64_t address;
    uint32_t size;
    uint32_t length;
    char name[];
} symtab_t;

typedef uint64_t elf64_addr;
typedef uint16_t elf64_half;
typedef uint64_t elf64_off;
typedef uint32_t elf64_word;
typedef uint64_t elf64_xword;
typedef int64_t  elf64_sxword;
typedef uint8_t  elf64_byte;

typedef struct {
    elf64_byte e_ident[16];
    elf64_half e_type;
    elf64_half e_machine;
    elf64_word e_version;
    elf64_addr e_entry;
    elf64_off  e_phoff;
    elf64_off  e_shoff;
    elf64_word e_flags;
    elf64_half e_ehsize;
    elf64_half e_phentsize;
    elf64_half e_phnum;
    elf64_half e_shentsize;
    elf64_half e_shnum;
    elf64_half e_shstrndx;
} elf64_ehdr;

typedef struct {
    elf64_word  p_type;
    elf64_word  p_flags;
    elf64_off   p_offset;
    elf64_addr  p_vaddr;
    elf64_addr  p_paddr;
    elf64_xword p_filesz;
    elf64_xword p_memsz;
    elf64_xword p_align;
} elf64_phdr;

typedef struct {
    elf64_word  sh_name;
    elf64_word  sh_type;
    elf64_xword sh_flags;
    elf64_addr  sh_addr;
    elf64_off   sh_offset;
    elf64_xword sh_size;
    elf64_word  sh_link;
    elf64_word  sh_info;
    elf64_xword sh_addralign;
    elf64_xword sh_entsize;
} elf64_shdr;

typedef struct {
    elf64_word  st_name;
    elf64_byte  st_info;
    elf64_byte  st_other;
    elf64_half  st_shndx;
    elf64_addr  st_value;
    elf64_xword st_size;
} elf64_sym;

#define PT_LOAD     1
#define SHT_SYMTAB  2
#define STT_FUNC    2

#define ELF64_ST_TYPE(info) ((info) & 0xf)

#endif
