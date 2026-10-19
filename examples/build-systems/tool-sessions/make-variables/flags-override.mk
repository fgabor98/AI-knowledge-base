# "override" appends even to a command-line value.
CFLAGS ?= -O2
override CFLAGS += -Wall -Wextra

show:
	@echo 'CFLAGS=$(CFLAGS)  origin=$(origin CFLAGS)'
