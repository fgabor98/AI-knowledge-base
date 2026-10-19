# The top level delegates to lib/ the way kernel, U-Boot, and SDK makefiles do.
all:
	$(MAKE) -C lib
	make -C lib
