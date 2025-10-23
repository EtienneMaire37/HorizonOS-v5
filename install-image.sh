#!/usr/bin/env bash
set -euo pipefail

IMG="horizonos.img"
MOUNTPOINT=$(mktemp -d)
ROOTDIR="./root"
GRUB_TARGET="i386-pc"

cleanup() 
{
    set +e
    if mountpoint -q "$MOUNTPOINT"; then
    umount "$MOUNTPOINT"
    fi
    [ -n "${LOOPPART:-}" ] && losetup -d "$LOOPPART" 2>/dev/null || true
    [ -n "${LOOPDEV:-}" ] && losetup -d "$LOOPDEV" 2>/dev/null || true
    rm -rf "$MOUNTPOINT"
}
trap cleanup EXIT

parted --script "$IMG" mklabel msdos
parted --script "$IMG" mkpart primary fat32 1MiB 100%
parted --script "$IMG" set 1 boot on

LOOPDEV=$(losetup --show -f "$IMG")
echo "Whole-disk loop: $LOOPDEV"

START_BYTE=$(parted -s "$IMG" unit B print | awk '/^[[:space:]]*1 / {print $2}' | sed 's/B$//')
if [ -z "$START_BYTE" ]; then
    echo "ERROR: failed to determine partition start. parted output:"
    parted -s "$IMG" unit B print
    exit 1
fi
echo "Partition 1 start byte: $START_BYTE"

LOOPPART=$(losetup --show -f -o "$START_BYTE" "$IMG")
echo "Partition loop: $LOOPPART"

mkfs.vfat -F 32 "$LOOPPART" -n GRUBDISK

mount "$LOOPPART" "$MOUNTPOINT"
cp -rT "$ROOTDIR" "$MOUNTPOINT"

grub-install --target="$GRUB_TARGET" --boot-directory="$MOUNTPOINT/boot" --modules="normal part_msdos fat" --force "$LOOPDEV"

sync

echo "Created image $IMG"