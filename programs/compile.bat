@echo off
SET PATH=%PATH%;C:\Users\Daniel\Downloads\i686-elf-tools-windows\bin
@echo on

i686-elf-gcc -ffreestanding -m32 -nostdlib -c -O0 "Test.c" -o "bin\Test.obj"

i686-elf-ld -T linker.ld -nostdlib "bin\Test.obj" -o "bin\Test.bin" --oformat binary

pause