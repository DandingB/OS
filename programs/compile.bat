@echo off
SET PATH=%PATH%;C:\Users\Daniel\Downloads\x86_64-elf-tools-windows\bin
@echo on

x86_64-elf-gcc -ffreestanding -nostdlib -c -O0 "Test.c" -o "bin\Test.obj"

x86_64-elf-ld -T linker.ld -nostdlib "bin\Test.obj" -o "bin\Test.bin" --oformat binary

pause