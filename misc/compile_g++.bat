@echo off
SET PATH=%PATH%;C:\Users\Daniel\Downloads\nasm-2.16.01
@echo on

nasm -f elf64 "src\boot.asm" -o "bin\boot.obj"
nasm -f elf64 "src\i686\isr.asm" -o "bin\isr.obj"
nasm -f elf64 "src\i686\x86.asm" -o "bin\x86.obj"

@echo off
SET PATH=%PATH%;C:\Users\Daniel\Downloads\x86_64-elf-tools-windows\bin\
@echo on

x86_64-elf-g++ -ffreestanding -fno-exceptions -fno-rtti -nostdlib -c "src\kernel.cpp" -o "bin\kernel.obj"
x86_64-elf-g++ -ffreestanding -fno-exceptions -fno-rtti -nostdlib -c "src\i686\idt.cpp" -o "bin\idt.obj"
x86_64-elf-g++ -ffreestanding -fno-exceptions -fno-rtti -nostdlib -c "src\i686\paging.cpp" -o "bin\paging.obj"
x86_64-elf-g++ -ffreestanding -fno-exceptions -fno-rtti -nostdlib -c "src\i686\pic.cpp" -o "bin\pic.obj"

x86_64-elf-ld -T linker.ld -nostdlib "bin\boot.obj" "bin\x86.obj" "bin\pic.obj" "bin\kernel.obj" "bin\paging.obj" "bin\idt.obj" "bin\isr.obj" -o "bin\OS.bin" --oformat binary

pause