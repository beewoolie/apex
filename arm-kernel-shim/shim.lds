/* Linking loader script for APEX Boot Loader */

OUTPUT_ARCH(arm)
EXTERN(main)
ENTRY(start)

SECTIONS
{
	. = 0;  /* Really, this code must be PIC */
	.text : {
		*(.text)
        }
	.rodata : {
		*(.rodata)
	}
	.text : {
		*(.boot)
	}
	/* Segments we don't want */
	/DISCARD/ . : {
		*(.fixup)
		*(.glue_7)
		*(.glue_7t)
		*(.data)
		*(.bss)
	}
	. = ALIGN (4);
}
