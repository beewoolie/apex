# lib1funcs-thumb1.S
#
# glue functions cribbed from GCC compiler because the compiler
# generates these symbols and assumes we'll be linking with libgcc. 
#

        .text
        .force_thumb
        .syntax unified

.globl __gnu_thumb1_case_uqi; .align 0; __gnu_thumb1_case_uqi:
	push	{r1}
        mov	r1, lr
        lsrs    r1, r1, #1
        lsls    r1, r1, #1
        ldrb    r1, [r1, r0]
        lsls    r1, r1, #1
        add     lr, lr, r1
	pop	{r1}
        bx      lr
.type __gnu_thumb1_case_uqi, %function; .size __gnu_thumb1_case_uqi, . - __gnu_thumb1_case_uqi

