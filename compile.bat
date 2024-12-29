@echo off
SET PATH=%PATH%;C:\Users\Daniel\Downloads\nasm-2.16.01
@echo on

nasm -f elf64 "src\boot.asm" -o "bin\boot.obj"
nasm -f elf64 "src\i686\x86.asm" -o "bin\x86.obj"
nasm -f elf64 "src\i686\isr.asm" -o "bin\isr.obj"

@echo off
SET PATH=%PATH%;C:\Users\Daniel\Downloads\x86_64-elf-tools-windows\bin
@echo on

x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "src\kernel.c" -o "bin\kernel.obj"
x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "src\stdlib.c" -o "bin\stdlib.obj"
x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "src\memory.c" -o "bin\memory.obj"
x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "src\stdio.c" -o "bin\stdio.obj"
x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "src\pci.c" -o "bin\pci.obj"
x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "src\ahci.c" -o "bin\ahci.obj"
x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "src\xhci.c" -o "bin\xhci.obj"
x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "src\i686\paging.c" -o "bin\paging.obj"
x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "src\i686\idt.c" -o "bin\idt.obj"
x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "src\i686\pic.c" -o "bin\pic.obj"
x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "src\i686\apic.c" -o "bin\apic.obj"

x86_64-elf-ld -T linker.ld -nostdlib "bin\boot.obj" "bin\x86.obj" "bin\kernel.obj" "bin\stdio.obj" "bin\memory.obj" "bin\stdlib.obj" "bin\pci.obj" "bin\ahci.obj" "bin\xhci.obj" "bin\paging.obj" "bin\idt.obj" "bin\isr.obj" "bin\pic.obj" "bin\apic.obj" -o "bin\kernel_x86_64.bin" --oformat binary

pause