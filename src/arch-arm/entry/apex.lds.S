/* ld script for apex


*/


OUTPUT_ARCH(arm)
ENTRY(reset)
SECTIONS
{
	. = APEX_VMA;
	APEX_VMA_START = .;
	APEX_VMA_COPY_START = .;
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
	. = ALIGN (4);
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
		*(.service_4)
		*(.service_5)
		*(.service_6)
		*(.service_7)
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
	.atag . : {
		APEX_ATAG_START = .;
		*(.atag_0)
		*(.atag_1)
		*(.atag_2)
		*(.atag_3)
		*(.atag_4)
		*(.atag_5)
		*(.atag_6)
		*(.atag_7)
		APEX_ATAG_END = .;
	}
	.command . : {
		APEX_COMMAND_START = .;
		*(.command)
		APEX_COMMAND_END = .;
	}
	.env . : {
		APEX_ENV_START = .;
		*(.env)
		APEX_ENV_END = .;
	}
	APEX_DATA_END = .;
	APEX_VMA_COPY_END = .;
	. = ALIGN (8);
	.bss . : {
		APEX_VMA_BSS_START = .;
		*(.bss)
		APEX_VMA_BSS_END = .;
	}
	. += 8192;
	APEX_VMA_STACK_START = .;
	. = ALIGN (4);
	APEX_VMA_END = .;
}
