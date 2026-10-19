# The common advice "CC ?= gcc" - does it set CC?
CC ?= gcc

show:
	@echo 'CC=$(CC)  origin=$(origin CC)'
