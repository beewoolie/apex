/* Linking loader script for APEX Boot Loader */


OUTPUT_ARCH(arm)
EXTERN(entry)
ENTRY(entry)

SECTIONS
{
	. = APEX_VMA;
	APEX_VMA_ENTRY = .;
	APEX_VMA_COPY_START = .;
	.entry : {
		*(.entry)
	}
	.reset : {
		*(.reset)
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
	.vector : {	/* Exception vector support, if there is any */
		APEX_VMA_VECTOR_START = .;
		*(.vector.0)
		*(.vector.1)
		APEX_VMA_VECTOR_END = .;
		*(.vector)
	}
	. = ALIGN (4);
	APEX_DATA_START = .;
	.data . : {
		*(.data)
	}
	.service . : {
		APEX_SERVICE_START = .;
		*(.service.0)
		*(.service.1)
		*(.service.2)
		*(.service.3)
		*(.service.4)
		*(.service.5)
		*(.service.6)
		*(.service.7)
		APEX_SERVICE_END = .;
	}
	.driver . : {
		APEX_DRIVER_START = .;
		*(.driver.0)
		*(.driver.1)
		*(.driver.2)
		*(.driver.3)
		*(.driver.4)
		*(.driver.5)
		*(.driver.6)
		*(.driver.7)
		APEX_DRIVER_END = .;
	}
	.atag . : {
		APEX_ATAG_START = .;
		*(.atag.0)
		*(.atag.1)
		*(.atag.2)
		*(.atag.3)
		*(.atag.4)
		*(.atag.5)
		*(.atag.6)
		*(.atag.7)
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
	.pcm.data . : {
		*(.pcm.data)
	}
	APEX_DATA_END = .;
	APEX_VMA_COPY_END = .;
	. = ALIGN (8);
	.bss . : {
		APEX_VMA_BSS_START = .;
		*(.bss)
		APEX_VMA_BSS_END = .;
		. = ALIGN (4);
		*(.ethernet.bss)
		*(.clcdc.bss)
	}
	. += 8192;
	APEX_VMA_STACK_START = .;
	. = ALIGN (4);
	. += 8192;
	APEX_VMA_IRQSTACK_START = .;
	. = ALIGN (4);
	APEX_VMA_END = .;
}
