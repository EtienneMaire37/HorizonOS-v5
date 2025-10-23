#!/usr/bin/env bash
set -euo pipefail

IMG="horizonos.img"
MOUNTPOINT=$(mktemp -d)
ROOTDIR="./root"
GRUB_TARGET="i386-pc"

parted --script "$IMG" mklabel msdos
parted --script "$IMG" mkpart primary fat32 1MiB 100%
parted --script "$IMG" set 1 boot on

LOOPDEV=$(losetup -Pf --show "$IMG")
mkfs.vfat -F 32 "${LOOPDEV}p1" -n GRUBDISK

mount "${LOOPDEV}p1" "$MOUNTPOINT"

cp -rT "$ROOTDIR" "$MOUNTPOINT"

grub-install --target=$GRUB_TARGET --boot-directory="$MOUNTPOINT/boot" --modules="normal part_msdos fat" --force "$LOOPDEV"

sync

umount "$MOUNTPOINT"
losetup -d "$LOOPDEV"
rmdir "$MOUNTPOINT"

echo "Created image $IMG"