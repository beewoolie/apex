/* ld script for apex


*/


OUTPUT_ARCH(arm)
ENTRY(reset)
SECTIONS
{
	. = APEX_VMA;
	APEX_VMA_START = .;
	.entry : {
		*(.entry)
	}
	.bootstrap : {
		*(.bootstrap)		
	}		
	.text : {
		*(.text)
		*(.fixup)
		*(.rodata)
		*(.rodata.*)
		*(.glue_7)
		*(.glue_7t)
	}
	. = ALIGN (8);
	APEX_DATA_START = .;
	.data . : {
		*(.data)
	}
	.driver_init . : {
		APEX_DRIVER_INIT_START = .;
		*(.driver_init)
		APEX_DRIVER_INIT_END = .;
	}
	APEX_DATA_END = .;
	APEX_VMA_END = .;
	. = ALIGN (8);
	.bss . : {
		APEX_BSS_START = .;
		*(.bss)
		APEX_BSS_END = .;
	}
	. += 8192;
	APEX_STACK_START = .;
}
