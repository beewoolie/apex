/* ld script for apex


*/


OUTPUT_ARCH(arm)
ENTRY(reset)
SECTIONS
{
	APEX_LMA_START = .;
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
	.data : {
		APEX_DATA_START = .;
		*(.data)
		APEX_DATA_END = .;
	}
	APEX_LMA_END = .;
	.bss : {
		APEX_BSS_START = .;
		*(.bss)
		APEX_BSS_END = .;
	}
	. += 8192;
	APEX_STACK_START = .;
}
