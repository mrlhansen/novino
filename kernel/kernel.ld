OUTPUT_FORMAT("elf64-x86-64")
ENTRY(kstub)
phys = 0xFFFFFFFF80100000;
SECTIONS
{
	.text phys : ALIGN(4K)
	{
		code = .;
		*(.text)
	}

	.rodata : ALIGN(4K)
	{
		rodata = .;
		*(.rodata*)
		*(.eh_frame)
	}

	.data : ALIGN(4K)
	{
		data = .;
		*(.data)
	}

	.bss : ALIGN(4K)
	{
		bss = .;
		*(.bss)
		*(COMMON)
	}

	end = .;
}
