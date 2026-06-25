---
status: draft
reviewed: false
domain: build-systems
difficulty: intermediate
reviewer: null
last_reviewed: null
---

# Install Rules and Staging

## What Problem Does This Solve?

Building a binary is not enough. Embedded Linux software must be installed into a predictable filesystem layout so it can be packaged into a root filesystem, SDK, update bundle, or release artifact.

Install rules and staging directories answer:

- where does the executable go?
- where do libraries, headers, services, and config files go?
- how can packaging install files without writing into the developer host system?
- how does the build output become part of a root filesystem image?

## Core Concepts

- install rule
- install prefix
- `DESTDIR`
- staging directory
- root filesystem layout
- runtime files vs development files
- package split
- permissions and ownership
- generated install manifest

## Mental Model

Separate three locations:

```text
build directory    -> temporary compiler and linker outputs
staging directory  -> install tree used for packaging
target rootfs      -> final runtime filesystem on the device
```

The build directory is not the root filesystem. The staging directory is not necessarily the root filesystem either. It is an intermediate install tree that lets package tools collect files safely.

## Syntax / API / Mechanism

Common Make variables:

```make
PREFIX ?= /usr
DESTDIR ?=
```

Install target:

```make
.PHONY: install
install: app
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 app $(DESTDIR)$(PREFIX)/bin/app
```

Stage install:

```sh
make DESTDIR="$PWD/stage" PREFIX=/usr install
```

CMake install rule:

```cmake
install(TARGETS app RUNTIME DESTINATION bin)
```

CMake stage install:

```sh
DESTDIR="$PWD/stage" cmake --install build --prefix /usr
```

Inspect staged files:

```sh
find stage -type f -print
```

## Minimal Example

Makefile:

```make
APP := hello
PREFIX ?= /usr
DESTDIR ?=

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

$(APP): hello.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY: install clean
install: $(APP)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(APP) $(DESTDIR)$(PREFIX)/bin/$(APP)

clean:
	rm -f $(APP)
```

Build and stage:

```sh
make
make DESTDIR="$PWD/stage" PREFIX=/usr install
find stage -type f -print
```

Expected output:

```text
stage/usr/bin/hello
```

## Real-World Example

A small daemon may install:

```text
/usr/sbin/sensor-daemon
/etc/sensor-daemon.conf
/usr/lib/systemd/system/sensor-daemon.service
/usr/share/doc/sensor-daemon/README
```

Make install rules:

```make
PREFIX ?= /usr
SYSCONFDIR ?= /etc
SYSTEMD_UNITDIR ?= /usr/lib/systemd/system
DESTDIR ?=

.PHONY: install
install: sensor-daemon
	install -d $(DESTDIR)$(PREFIX)/sbin
	install -m 0755 sensor-daemon $(DESTDIR)$(PREFIX)/sbin/sensor-daemon
	install -d $(DESTDIR)$(SYSCONFDIR)
	install -m 0644 sensor-daemon.conf $(DESTDIR)$(SYSCONFDIR)/sensor-daemon.conf
	install -d $(DESTDIR)$(SYSTEMD_UNITDIR)
	install -m 0644 sensor-daemon.service $(DESTDIR)$(SYSTEMD_UNITDIR)/sensor-daemon.service
```

Packaging systems can override these variables to match their filesystem policy.

## Common Scenarios

### `PREFIX` vs `DESTDIR`

`PREFIX` describes the runtime install prefix:

```text
/usr
/usr/local
/opt/vendor
```

`DESTDIR` describes a temporary staging root:

```text
/home/user/project/stage
/tmp/package-root
```

Together:

```text
DESTDIR=/tmp/package-root
PREFIX=/usr
installed file -> /tmp/package-root/usr/bin/app
runtime path   -> /usr/bin/app
```

Do not bake `DESTDIR` into runtime config files.

### Host Install vs Package Install

Avoid this while developing:

```sh
sudo make install
```

Prefer:

```sh
make DESTDIR="$PWD/stage" install
```

This avoids polluting the host and lets you inspect exactly what the package would contain.

### Runtime Files vs Development Files

Runtime package:

```text
/usr/bin/app
/usr/lib/libfoo.so.1
/etc/app.conf
```

Development package:

```text
/usr/include/foo.h
/usr/lib/libfoo.so
/usr/lib/pkgconfig/foo.pc
```

Embedded distributions often split these to keep the runtime image small.

### systemd Units

Install service files into the path expected by the target distribution. On many systems:

```text
/usr/lib/systemd/system/
```

Some distributions use:

```text
/lib/systemd/system/
```

Do not guess; follow the target distro, Yocto layer, Buildroot package, or SDK convention.

### Yocto and Buildroot

Yocto recipes usually install into `${D}` during `do_install`. Buildroot packages install into target/staging directories controlled by package infrastructure.

The project install rule should be conventional enough that these frameworks can call it without patching.

## Common Mistakes

- Using `sudo make install` while testing install rules.
- Installing directly into `/usr` from a package build.
- Confusing `PREFIX` and `DESTDIR`.
- Hard-coding `/usr/local` for target software.
- Forgetting config files, service units, udev rules, or helper scripts.
- Installing development headers into a runtime-only package.
- Losing executable permissions by copying files without `install -m`.
- Generating paths with duplicate slashes or missing directory creation.

## Debugging Checklist

- Stage install into an empty directory.
- Run `find stage -type f -o -type l`.
- Check file modes with `ls -l`.
- Check runtime paths do not contain the staging prefix.
- Check service files reference installed paths.
- Check shared libraries and symlinks are present.
- Check whether headers and `.pc` files belong in runtime or development packages.
- Test install rules after a clean build.

## Related Topics

- [Native Linux Userspace Builds](native-linux-userspace-builds.md)
- [Make Variables and Pattern Rules](make-variables-and-pattern-rules.md)
- [CMake Basics](cmake-basics.md)
- [Build Systems for Embedded Linux](embedded-linux-roadmap.md)

## References

- GNU Make manual
- CMake install documentation
- Filesystem Hierarchy Standard
- Yocto Project Development Tasks Manual
- Buildroot manual
