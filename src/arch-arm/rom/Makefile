#
# Makefile for APEX loader
#

targets += apex.bin apex.srec

$(obj)/apex.bin: apex FORCE
	$(call if_changed,objcopy)

# *** FIXME: there may need to be an adjustment to the start address 
OBJCOPYFLAGS_apex.srec := -O srec
$(obj)/apex.srec: apex FORCE
	$(call if_changed,objcopy)

