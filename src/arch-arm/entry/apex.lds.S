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
	.service . : {
		APEX_SERVICE_START = .;
		*(.service_0)
		*(.service_1)
		*(.service_2)
		*(.service_3)
		*(.service)
		APEX_SERVICE_END = .;
	}
	.driver . : {
		APEX_DRIVER_START = .;
		*(.driver_0)
		*(.driver_1)
		*(.driver_2)
		*(.driver_3)
		*(.driver)
		APEX_DRIVER_END = .;
	}
	.command . : {
		APEX_COMMAND_START = .;
		*(.command)
		APEX_COMMAND_END = .;
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
	APEX_END = .;
}
