# An intermediate single-directory Makefile for a small C tool.
APP     := board-tool
VERSION ?= 1.0
BUILD   ?= build

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

# Respect a CC from the environment or command line (Yocto, Buildroot, SDKs).
CROSS_COMPILE ?=
ifeq ($(origin CC),default)
CC := $(CROSS_COMPILE)gcc
endif

# Callers own CPPFLAGS/CFLAGS/LDFLAGS/LDLIBS; the project's required flags
# live in separate variables so a caller value never removes them.
CFLAGS      ?= -O2 -g
KB_CPPFLAGS := -Iinclude
KB_CFLAGS   := -std=c17 -Wall -Wextra
DEPFLAGS     = -MMD -MP

PREFIX ?= /usr
bindir ?= $(PREFIX)/bin

.DELETE_ON_ERROR:
.PHONY: all clean install

all: $(BUILD)/$(APP)

$(BUILD)/$(APP): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(KB_CPPFLAGS) $(CPPFLAGS) $(KB_CFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $@

# Target-specific variable: only version.o sees the version define.
$(BUILD)/version.o: KB_CPPFLAGS += -DTOOL_VERSION='"$(VERSION)"'

# Make compares timestamps, not command lines. The stamp file is rewritten
# (while the Makefile is read) only when VERSION changes, so
# "make VERSION=2.1" rebuilds version.o and nothing else.
ifeq ($(filter clean print-%,$(MAKECMDGOALS)),)
$(shell mkdir -p $(BUILD) && echo '$(VERSION)' | cmp -s - $(BUILD)/version.stamp || echo '$(VERSION)' > $(BUILD)/version.stamp)
endif
$(BUILD)/version.o: $(BUILD)/version.stamp

install: $(BUILD)/$(APP)
	install -D -m 0755 $< $(DESTDIR)$(bindir)/$(APP)

clean:
	rm -rf $(BUILD)

# print-VAR: show a variable's value and where it came from.
print-%:
	@echo '$*=$($*)  [origin: $(origin $*)]'

-include $(DEPS)
