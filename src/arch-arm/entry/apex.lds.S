/* Linking loader script for APEX Boot Loader */

OUTPUT_ARCH(arm)
EXTERN(entry)
ENTRY(entry)

SECTIONS
{
	. = APEX_VMA;
	APEX_VMA_ENTRY = .;
	APEX_VMA_START = .;
	APEX_VMA_COPY_START = .;
	.entry : {		/* Opportunity to inject code before reset */
		*(.entry)
	}
	.reset : {		/* Reset handling code, usually first */
		*(.reset)
	}
	. = ALIGN (4);			/* should be redundant */
	.envlink : {		/* Link to $env for user-land */
		*(.envlink)
	}
	. = ALIGN (4);			/* should be redundant */
	.bootstrap : {		/* Bootstrap initialzation */
		*(.bootstrap)		
	}		
		/* Everything up to this point must fit in the first
		   4K (determined by the CPU boot rom) of NAND flash
		   so that we can bootstrap copy the loader from NAND
		   to SDRAM. */
	.vector : {	/* Exception vector support, if there is any */
		APEX_VMA_VECTOR_START = .;
		*(.vector.0)
		*(.vector.1)
		APEX_VMA_VECTOR_END = .;
		*(.vector)
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
	/* Discard data because there *must* not be mutable,
	   initialized data.  Use __rodata attribute for initialized
	   data to move it into the text section. */
	/DISCARD/ . : {
		*(.data)
	}
	. = ALIGN (4);
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
	. = ALIGN (4);
	APEX_VMA_PROBE_END = .;
	/* The next word is the location we'll be writing to when
	   probing.  See note in drv-mem.c. */
	APEX_VMA_STACKS_START = .;
	. += STACK_SIZE;
	. = ALIGN (4);
	APEX_VMA_STACK_START = .;
#if defined (CONFIG_INTERRUPTS)
	. = ALIGN (4);
	. += 4096;		/* Interrupt stack, should be configurable */
	APEX_VMA_IRQSTACK_START = .;
#endif
	APEX_VMA_STACKS_END = .;
	. = ALIGN (4);
	.bss . : {		/* Normal, zeroed BSS */
		APEX_VMA_BSS_START = .;
		*(.bss)
		. = ALIGN (4);
		*(.*.bss)	/* All other bss sections */
		APEX_VMA_BSS_END = .;
	}
	.xbss (NOLOAD) : {	/* Special BSS, non-zeroed */
#if defined (CONFIG_MMU)
		. = ALIGN (16384);
		*(.ttbl.xbss)   /* MMU translation table */
#endif
		. = ALIGN (4);
		*(.*.xbss)	/* Misc xbss sections */
		*(.pcm.xbss)    /* Audio xbss data */
	}
	APEX_VMA_END = .;
						/* Bootstrap data */
	.bss_bootstrap (0xb0000000 + 80*1024 - 4096) (NOLOAD) : {
		APEX_VMA_BOOTSTRAP_STACK_START = .;
		*(.bss_bootstrap)
	}		
}
