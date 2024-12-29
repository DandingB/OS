[bits 16]

extern __bss_start
extern __end

section .entry

%define PAGE_PRESENT    (1 << 0)
%define PAGE_WRITE      (1 << 1)

KERNEL_LOCATION equ 0x7E00
CODE_SEG equ GDT_code - GDT ; 8
DATA_SEG equ GDT_data - GDT ; 16

global entry
entry:
    mov ax, 0 ; Set segments to 0
    mov ds, ax
    mov es, ax
    mov bp, 0x7000
    mov sp, bp

    mov [BOOT_DISK], dl

    ; Load kernel code
    mov bx, KERNEL_LOCATION ; Load address
    mov dh, 128  ; Sectors to read count
    mov ah, 0x02 ; Mode: read sectors from drive
    mov al, dh   ; Sectors to read count
    mov ch, 0x00 ; Cylinder
    mov dh, 0x00 ; Head
    mov cl, 0x02 ; Sector
    mov dl, dl   ; Drive
    int 0x13            ; TODO: no error management!

    ;Enable A-20
    call enable_A20

    mov di, 0x1000  ; Page table address

    ; Zero out the 16KiB buffer.
    push di         ; REP STOSD alters DI.
    mov ecx, 0x1000 ; Number of times
    xor eax, eax
    cld
    rep stosd
    pop di

    ; Build paging structures. These are only temporary and should be overwritten by the kernel asap.
    ; Build Page Map Table level 4
    lea eax, [es:di + 0x1000]
    or eax, PAGE_PRESENT | PAGE_WRITE 
    mov [es:di], eax

    ; Build the Page Directory Pointer Table.
    lea eax, [es:di + 0x2000]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 0x1000], eax

    ; Build the Page Directory.
    lea eax, [es:di + 0x3000]
    or eax, PAGE_PRESENT | PAGE_WRITE
    mov [es:di + 0x2000], eax
    
    push di
    mov eax, PAGE_PRESENT | PAGE_WRITE

    ; Build the Page Table.
.LoopPageTable:
    mov [es:di + 0x3000], eax
    add eax, 0x1000
    add di, 8
    cmp eax, 0x200000
    jb .LoopPageTable

    pop di
    
    
    ; Enable PAE and PGE
    mov eax, cr4
    or eax, 0b10100000
    mov cr4, eax

    ; Enable LME and Syscall extensions
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x00000101
    wrmsr

    ; Set address of PML4
    lea edx, [es:di]
    mov cr3, edx

    ; Enable paging and protected mode
    mov ebx, cr0
    or ebx,0x80000001
    mov cr0, ebx                    

    ; Disable interrupts and Load GDT descriptor
    cli
    lgdt [GDT_descriptor]

     ; Load CS with 64 bit segment and flush the instruction cache
    jmp CODE_SEG:LongMode
    jmp $

enable_A20:
    cli
 
    call    a20wait
    mov     al,0xAD
    out     0x64,al
 
    call    a20wait
    mov     al,0xD0
    out     0x64,al
 
    call    a20wait2
    in      al,0x60
    push    eax
 
    call    a20wait
    mov     al,0xD1
    out     0x64,al
 
    call    a20wait
    pop     eax
    or      al,2
    out     0x60,al
 
    call    a20wait
    mov     al,0xAE
    out     0x64,al
 
    call    a20wait
    sti
    ret
 
a20wait:
    in      al,0x64
    test    al,2
    jnz     a20wait
    ret
 
a20wait2:
    in      al,0x64
    test    al,1
    jz      a20wait2
    ret





[bits 64]      
LongMode:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x7000	    ; Update our stack position so it is right
    mov esp, ebp        ; at the top of the free space.

    ; Set TSS
    mov ax, (5 * 8) | 0 ; fifth 8-byte selector, symbolically OR-ed with 0 to set the RPL (requested privilege level).
	ltr ax

    ; Clear bss section (uninitialized static variables) to zeroes
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov eax, 0
    cld
    rep stosb ; rep: ecx = number of times, stosb: edi = start location

    ; Push boot disk parameter on the stack
    xor rdi, rdi
    mov dl, [BOOT_DISK]

    extern kernel_entry
    call kernel_entry
    jmp $




; Global Descriptor Table  ; Padding to make the "address of the GDT" field aligned on a 4-byte boundary
ALIGN 4 
GDT_descriptor:
    dw GDT_end - GDT - 1
    dd GDT

GDT:
    GDT_null:
        dq 0x0000000000000000             ; Null Descriptor - should be present.

    GDT_code:    
        dw 0xffff       ;Limit 0-15
        dw 0x0000       ;Base 0-15
        db 0x00         ;Base 16-23
        db 0b10011010   ;Access byte (DPL level 0)
        db 0b00101111   ;Flags (set 64-bit code), Limit 16-19  
        db 0x00         ;Base 24-31

    GDT_data:
        dw 0xffff
        dw 0x0000
        db 0x00
        db 0b10010010
        db 0b11001111
        db 0x00

    GDT_data_user:
        dw 0xffff
        dw 0x0000
        db 0x00
        db 0b11110010
        db 0b11001111
        db 0x00

    GDT_code_user:    
        dw 0xffff       ;Limit 0-15
        dw 0x0000       ;Base 0-15
        db 0x00         ;Base 16-23
        db 0b11111010   ;Access byte (DPL level 3)
        db 0b00101111   ;Flags (set 64-bit code), Limit 16-19  
        db 0x00         ;Base 24-31


    GDT_TSS:    
        dw 0x0068        ;Limit 0-15 (Sizeof TSS)
        dw TSS           ;Base 0-15
        db 0x00          ;Base 16-23
        db 0b10001001    ;Access byte
        db 0b00100000    ;Flags (set 64-bit code), Limit 16-19  
        db 0x00          ;Base 24-31
        dd 0x00000000   
        dd 0x00000000   
GDT_end:

TSS:
    dd 0x00000000
    dq 0x0000000000070000 ; RSP0
    dq 0x0000000000000000 ; RSP1
    dq 0x0000000000000000 ; RSP2
    dq 0x0000000000000000 ; rsvd
    dq 0x0000000000000000 ; IST1
    dq 0x0000000000000000 ; IST2
    dq 0x0000000000000000 ; IST3
    dq 0x0000000000000000 ; IST4
    dq 0x0000000000000000 ; IST5
    dq 0x0000000000000000 ; IST6
    dq 0x0000000000000000 ; IST7
    dq 0x0000000000000000 ; rsvd
    dd 0x00000000
TSS_end:

BOOT_DISK: db 0


times 510-($-$$) db 0              
dw 0xAA55
