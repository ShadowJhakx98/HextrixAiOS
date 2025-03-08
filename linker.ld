ENTRY(kernel_main)

PHDRS {
	text PT_LOAD FLAGS(5);	/* Read + Execute (RX) */
	data PT_LOAD FLAGS(6);	/* Read + Write (RW) */
	bss PT_LOAD FLAGS(6);	/* Read + Write (RW) */
}

SECTIONS {
	. = 0x100000;	/* Start at 1MB */
	
	/* Text section: read and execute */
	.text : {
		*(.text)
	} :text
	
	/* Data section: read and write */
	.data : {
		*(.data)
	} :data
	
	/* BSS section: read and write */
	.bss : {
		*(.bss)
	} :bss
}