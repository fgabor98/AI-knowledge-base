#!/usr/bin/env bash
# Build a debug kernel for the labs from a kernel source tree.
set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
src=${1:?usage: build-kernel.sh KERNEL_SOURCE OUTPUT_DIRECTORY}
out=${2:?usage: build-kernel.sh KERNEL_SOURCE OUTPUT_DIRECTORY}
mkdir -p "$out"
out=$(cd "$out" && pwd)

make -C "$src" O="$out" x86_64_defconfig kvm_guest.config
"$src/scripts/kconfig/merge_config.sh" -m -O "$out" "$out/.config" "$here/debug.config"
make -C "$src" O="$out" olddefconfig
make -C "$src" O="$out" -j"$(nproc)" bzImage modules

echo "Kernel:  $out/arch/x86/boot/bzImage"
echo "Modules: make KDIR=$out"
