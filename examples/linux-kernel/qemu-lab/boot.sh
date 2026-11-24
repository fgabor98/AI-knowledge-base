#!/usr/bin/env bash
# Boot the lab kernel with a BusyBox shell and a shared directory on /mnt.
# Everything lives in an initramfs: nothing on the host disk is touched
# except through the shared directory.
set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
out=${1:?usage: boot.sh KERNEL_OUTPUT [SHARED_DIRECTORY] [extra kernel arguments]}
share=$(realpath "${2:-$here/../small-labs}")
extra=${3:-}
busybox=${BUSYBOX:-/bin/busybox}

if ldd "$busybox" >/dev/null 2>&1; then
	echo "$busybox is dynamically linked; install busybox-static" >&2
	exit 1
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/root/bin"
cp "$busybox" "$work/root/bin/busybox"
cat > "$work/root/init" <<'INIT'
#!/bin/busybox sh
/bin/busybox mkdir -p /proc /sys /dev /mnt /tmp /sbin /usr/bin /usr/sbin
/bin/busybox --install -s
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev
mount -t debugfs debugfs /sys/kernel/debug
mount -t tracefs tracefs /sys/kernel/tracing
mount -t 9p -o trans=virtio,version=9p2000.L host /mnt
cd /mnt
echo "Shared directory is /mnt. Type 'poweroff -f' to leave."
setsid cttyhack sh
poweroff -f
INIT
chmod +x "$work/root/init"
(cd "$work/root" && find . | cpio -o -H newc --quiet | gzip) > "$work/initrd.gz"

accel=()
[[ -w /dev/kvm ]] && accel=(-enable-kvm -cpu host)

qemu-system-x86_64 "${accel[@]}" -smp 4 -m 2G -nographic -no-reboot \
	-kernel "$out/arch/x86/boot/bzImage" \
	-initrd "$work/initrd.gz" \
	-append "console=ttyS0 panic=-1 nokaslr $extra" \
	-virtfs "local,path=$share,mount_tag=host,security_model=none,id=host" \
	-device edu -nic none -s
