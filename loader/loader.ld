OUTPUT_FORMAT("elf64-x86-64")
ENTRY(start)
phys = 0x1000000;
SECTIONS
{
	.text phys : ALIGN(4K)
	{
		text = .;
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
