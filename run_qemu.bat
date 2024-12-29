@echo off

SET PATH=%PATH%;D:\Program Files\qemu
START /wait "QEMU" "qemu-system-x86_64.exe" "bin\kernel_x86_64.bin" -m 8G -drive id=hd,file=hd.img,if=none -device ahci,id=ahci -device ide-hd,drive=hd,bus=ahci.0 -device qemu-xhci -device usb-mouse