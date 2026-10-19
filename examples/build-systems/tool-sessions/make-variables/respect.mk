# Only pick a compiler when nobody else did: neither the environment
# nor the command line nor an earlier makefile.
CROSS_COMPILE ?=
ifeq ($(origin CC),default)
CC := $(CROSS_COMPILE)gcc
endif

show:
	@echo 'CC=$(CC)  origin=$(origin CC)'
