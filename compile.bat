@echo off
SET PATH=%PATH%;C:\Users\Daniel\Downloads\nasm-2.16.01
@echo on

nasm -f elf "src\boot.asm" -o "bin\boot.obj"
nasm -f elf "src\i686\x86.asm" -o "bin\x86.obj"
nasm -f elf "src\i686\isr.asm" -o "bin\isr.obj"

@echo off
SET PATH=%PATH%;C:\Users\Daniel\Downloads\i686-elf-tools-windows\bin
@echo on

i686-elf-gcc -ffreestanding -m32 -nostdlib -c -O0 "src\kernel.c" -o "bin\kernel.obj"
i686-elf-gcc -ffreestanding -m32 -nostdlib -c -O0 "src\stdio.c" -o "bin\stdio.obj"
i686-elf-gcc -ffreestanding -m32 -nostdlib -c -O0 "src\pci.c" -o "bin\pci.obj"
i686-elf-gcc -ffreestanding -m32 -nostdlib -c -O0 "src\ahci.c" -o "bin\ahci.obj"
i686-elf-gcc -ffreestanding -m32 -nostdlib -c -O0 "src\paging.c" -o "bin\paging.obj"
i686-elf-gcc -ffreestanding -m32 -nostdlib -c -O0 "src\i686\idt.c" -o "bin\idt.obj"
i686-elf-gcc -ffreestanding -m32 -nostdlib -c -O0 "src\i686\pic.c" -o "bin\pic.obj"
i686-elf-gcc -ffreestanding -m32 -nostdlib -c -O0 "src\i686\apic.c" -o "bin\apic.obj"

i686-elf-ld -T linker.ld -nostdlib "bin\boot.obj" "bin\x86.obj" "bin\kernel.obj" "bin\stdio.obj" "bin\pci.obj" "bin\ahci.obj" "bin\paging.obj" "bin\idt.obj" "bin\isr.obj" "bin\pic.obj" "bin\apic.obj" -o "bin\OS.bin" --oformat binary

pause