# A hard-coded cross prefix, as many vendor Makefiles write it.
CROSS_COMPILE ?=
CC := $(CROSS_COMPILE)gcc

show:
	@echo 'CC=$(CC)  origin=$(origin CC)'
