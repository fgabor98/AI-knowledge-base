# Project-required flags appended to whatever the caller provides.
CFLAGS ?= -O2
CFLAGS += -Wall -Wextra

show:
	@echo 'CFLAGS=$(CFLAGS)  origin=$(origin CFLAGS)'
