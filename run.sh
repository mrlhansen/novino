#!/bin/bash

# import variables
source env.sh

# create and boot iso
mkdir -p sysroot/boot/grub
cp -p loader/kernel.bin sysroot/boot

cat > sysroot/boot/grub/grub.cfg << EOF
set timeout=0
menuentry "Novino" {
    multiboot2 /boot/kernel.bin
    module2 /boot/initrd.tar
    set gfxpayload=800x600x32
#    set gfxpayload=text
    boot
}
EOF

grub-mkrescue -o novino.iso sysroot
qemu-system-x86_64 -no-reboot -debugcon stdio -cdrom novino.iso -smp 4 -machine q35 \
    -device qemu-xhci,id=xhci \
    -device usb-hub,bus=xhci.0,port=1,id=a \
    -device usb-hub,bus=xhci.0,port=1.4,id=b \
    -device usb-kbd,bus=xhci.0,port=1.4.3,id=c \
    -device usb-mouse,bus=xhci.0

# device_add usb-kbd,bus=xhci.0,port=1.4.1,id=d
# device_add usb-kbd,bus=xhci.0,port=5,id=x
# device_del x

# QEMU Options
# ----------------------------------------------------------
# Hard Disk   : -drive file=ext2hd.img,format=raw -boot d
# XHCI        : -device qemu-xhci,id=xhci -device usb-hub,bus=xhci.0,port=4 -device usb-mouse,bus=xhci.0 -device usb-kbd,bus=xhci.0,port=4.4
# Q35 Chipset : -machine q35
# Network     : -netdev tap,id=mynet0,ifname=tap0,script=no,downscript=no -device rtl8139,netdev=mynet0

# menuentry "Novino" {
#     search --no-floppy --fs-uuid --set=root f8930de6-e51f-49ad-aac1-e59504f2045b
#     multiboot2 /hansen/Work/novino/sysroot/boot/kernel.bin
#     set gfxpayload=1920x1080x32
#     boot
# }
