ENTRY(_start)

MEMORY {
	ROM (rx) : org = 4096M - 256K, len = 256K
}

/*So like this is sort of scuffed we are loaded in just below 4G end
 *But we do this to work around Linker tools being annoying to use
 *But it's fine for 16bit code so long as we don't grow larger than
 *this one segment
 */

SECTIONS {
	_bios_segment =	0xffff0000;
	.head : {
		*(.head)
	} > ROM

	.text 0xffff8000 : {
		/*Oki this is an afront to linker scripts but it works*/
		*(.text)
		*(.data)
		*(.rodata.*)
	}

	.reset 0xfffffff0 : {
		*(.reset)
	}
}
