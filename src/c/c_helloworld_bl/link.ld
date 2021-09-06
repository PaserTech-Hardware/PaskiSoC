ENTRY(_start)

SECTIONS {
	PROVIDE(__payload_start = 0x80000000);
	PROVIDE(__readwrite_start = 0x80001000);
	PROVIDE(__stack_start = 0x80001C00);
	
	. = __payload_start;
	.start : {
		KEEP(*(.text._start));
	}
	.text : {
		*(.text*);
	}
	__readonly_start = .;
	.readonly : {
		*(.rodata*);
	}
	__readonly_end = .;
	__payload_end = .;
	
	PROVIDE(__rw_region_offset = __payload_end);
	
	. = __readwrite_start;
	.readwrite : {
		*(.data*);
		*(COMMON);
		*(.bss*);
	}
	__readwrite_end = .;
	
	. = __stack_start;
	.stackarea : {
		KEEP(*(.stack_sec));
	}
	__stack_end = .;
	
	PROVIDE(__rw_region_length = __readwrite_end - __readwrite_start);
}
