#pragma once

#include <kernel/types.h>

typedef uint64_t elf64_addr;
typedef uint16_t elf64_half;
typedef uint64_t elf64_off;
typedef uint32_t elf64_word;
typedef uint64_t elf64_xword;
typedef int64_t  elf64_sxword;
typedef uint8_t  elf64_byte;

// header
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

// program header
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

// section header
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

// relocation table
typedef struct {
    elf64_addr  r_offset;
    elf64_xword r_info;
} elf64_rel;

// relocation table with addend
typedef struct {
    elf64_addr r_offset;
    elf64_xword r_info;
    elf64_sxword r_addend;
} elf64_rela;

// symbol table
typedef struct {
    elf64_word st_name;
    elf64_byte st_info;
    elf64_byte st_other;
    elf64_half st_shndx;
    elf64_addr st_value;
    elf64_xword st_size;
} elf64_sym;

// extract symbol and type from rela.r_info
#define ELF64_R_SYM(i)     ((i) >> 32)
#define ELF64_R_TYPE(i)    ((i) & 0xffffffff)

// program header flags
#define PF_EXEC  1
#define PF_WRITE 2
#define PF_READ  4

// relocation types
#define R_X86_64_64           1
#define R_X86_64_64_JUMP_SLOT 7
#define R_X86_64_RELATIVE     8

// image type
#define ET_NONE 0
#define ET_REL  1
#define ET_EXEC 2
#define ET_DYN  3
#define ET_CORE 4

// machine type
#define EM_NONE   0x00
#define EM_X86    0x03
#define EM_AMD64  0x3E

// segment types stored in image header
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_SHLIB   5
#define PT_PHDR    6

// dynamic table entries
#define DT_NULL     0
#define DT_NEEDED   1
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_HASH     4
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_INIT     12

// section table types
#define SHT_NULL     0
#define SHT_PROGBITS 1
#define SHT_SYMTAB   2
#define SHT_STRTAB   3
#define SHT_RELA     4
#define SHT_HASH     5
#define SHT_DYNAMIC  6
#define SHT_NOTE     7
#define SHT_NOBITS   8
#define SHT_DYNSYM   11

// section attributes
#define SHF_WRITE 1
#define SHF_ALLOC 2

// file version
#define EV_NONE    0
#define EV_CURRENT 1

// elf magic values
#define EI_MAG0    0  // 0x7f
#define EI_MAG1    1  // E
#define EI_MAG2    2  // L
#define EI_MAG3    3  // F
#define EI_CLASS   4  // 1 = 32bit, 2 = 64bit
#define EI_DATA    5  // 1 = lsb, 2 = msb
#define EI_VERSION 6  // should be equal to EV_CURRENT

// elf class
#define ELFCLASSNONE 0
#define ELFCLASS32   1
#define ELFCLASS64   2

// endianness
#define ELFDATANONE  0
#define ELFDATA2LSB  1
#define ELFDATA2MSB  2
