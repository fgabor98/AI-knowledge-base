# Build, run, and sanitizer-test the standalone C examples in this directory.
#
#   make            build every example with strict warnings (-Werror)
#   make check      build and run every example
#   make sanitize   build and run every example under ASan + UBSan
#   make tsan       build and run the threaded examples under ThreadSanitizer
#   make clean      remove build output
#
# The failure case studies have their own runner and validator:
#   bash failure-case-studies/run-all.sh
#   scripts/validate-c-case-studies.sh   (from the repository root)

STRICT_CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror
SANITIZE_CFLAGS := -std=c11 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all
TSAN_CFLAGS := -std=c11 -O1 -g -fsanitize=thread
LDLIBS := -pthread
RUN_TIMEOUT := 10

BUILD := build
SOURCES := $(sort $(wildcard *.c))
NAMES := $(SOURCES:.c=)
THREADED := $(basename $(shell grep -l pthread_create $(SOURCES)))

# ThreadSanitizer needs a fixed address-space layout on kernels with high
# mmap randomization.
NO_ASLR := $(if $(shell command -v setarch),setarch $(shell uname -m) -R)

.PHONY: all check sanitize tsan clean

all: $(NAMES:%=$(BUILD)/strict/%)

check: all
	@set -e; for name in $(NAMES); do \
	    printf 'run  %s\n' "$$name"; \
	    timeout $(RUN_TIMEOUT) $(BUILD)/strict/$$name >/dev/null; \
	done

sanitize: $(NAMES:%=$(BUILD)/sanitize/%)
	@set -e; for name in $(NAMES); do \
	    printf 'run  %s (ASan+UBSan)\n' "$$name"; \
	    timeout $(RUN_TIMEOUT) $(BUILD)/sanitize/$$name >/dev/null; \
	done

tsan: $(THREADED:%=$(BUILD)/tsan/%)
	@set -e; for name in $(THREADED); do \
	    printf 'run  %s (TSan)\n' "$$name"; \
	    TSAN_OPTIONS=halt_on_error=1 $(NO_ASLR) timeout $(RUN_TIMEOUT) $(BUILD)/tsan/$$name >/dev/null; \
	done

$(BUILD)/strict/%: %.c | $(BUILD)/strict
	$(CC) $(STRICT_CFLAGS) $< -o $@ $(LDLIBS)

$(BUILD)/sanitize/%: %.c | $(BUILD)/sanitize
	$(CC) $(SANITIZE_CFLAGS) $< -o $@ $(LDLIBS)

$(BUILD)/tsan/%: %.c | $(BUILD)/tsan
	$(CC) $(TSAN_CFLAGS) $< -o $@ $(LDLIBS)

$(BUILD)/strict $(BUILD)/sanitize $(BUILD)/tsan:
	mkdir -p $@

clean:
	rm -rf $(BUILD)
