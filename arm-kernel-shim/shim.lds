/* Linking loader script for APEX Boot Loader */

OUTPUT_ARCH(arm)
EXTERN(start)
ENTRY(start)

SECTIONS
{
	. = 0x1000;  /* Actually, this code must be PIC */
	.start : {
		*(.start)
        }
	. = ALIGN (4);
	.rodata : {
		*(.rodata)
	}
	. = ALIGN (4);
	.boot : {
		*(.boot)
	}
	. = ALIGN (4);
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
