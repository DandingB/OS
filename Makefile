
NASM_PATH = /Users/daniel/Downloads/nasm-2.16.03
GCC_PATH = C:/Users/Daniel/Downloads/i686-elf-tools-windows/bin

NASM = $(NASM_PATH)/nasm
GCC = i686-elf-gcc
LD = $(GCC_PATH)/i686-elf-ld
FLAGS = -ffreestanding -m32 -nostdlib -c -O0

OBJ_FILES := $(wildcard bin/*.obj)

default: compile

compile:
	$(NASM) -f elf "src/boot.asm" -o "bin/boot.obj"
	$(NASM) -f elf "src/i686/x86.asm" -o "bin/x86.obj"
	$(NASM) -f elf "src/i686/isr.asm" -o "bin/isr.obj"

	$(GCC) $(FLAGS) "src/kernel.c" -o "bin/kernel.obj"
	$(GCC) $(FLAGS) "src/stdlib.c" -o "bin/stdlib.obj"
	$(GCC) $(FLAGS) "src/stdio.c" -o "bin/stdio.obj"
	$(GCC) $(FLAGS) "src/pci.c" -o "bin/pci.obj"
	$(GCC) $(FLAGS) "src/ahci.c" -o "bin/ahci.obj"
	$(GCC) $(FLAGS) "src/xhci.c" -o "bin/xhci.obj"
	$(GCC) $(FLAGS) "src/i686/paging.c" -o "bin/paging.obj"
	$(GCC) $(FLAGS) "src/i686/idt.c" -o "bin/idt.obj"
	$(GCC) $(FLAGS) "src/i686/pic.c" -o "bin/pic.obj"
	$(GCC) $(FLAGS) "src/i686/apic.c" -o "bin/apic.obj"

	$(LD) -T linker.ld -nostdlib $(OBJ_FILES) -o "bin/OS.bin" --oformat binary
