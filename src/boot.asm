[bits 16]

KERNEL_LOCATION equ 0x7e00

section .entry

extern __bss_start
extern __end

global entry

entry:

; Save the booting disk
mov [BOOT_DISK], dl

mov ax, 0 ; Set segments to 0
mov ds, ax
mov es, ax
mov bp, 0x7000
mov sp, bp

;Enable A-20
call enable_A20

; Copy the code from sector 2 to memory location 0x7e00
mov bx, KERNEL_LOCATION
mov dh, 16

mov ah, 0x02 ; Mode: read sectors from drive
mov al, dh   ; Sectors to read count
mov ch, 0x00 ; Cylinder
mov dh, 0x00 ; Head
mov cl, 0x02 ; Sector
mov dl, dl ; Drive
int 0x13            ; TODO: no error management!

;mov ah, 0x0E
;mov al, 'G'
;int 0x10

; Setup protected mode
cli
lgdt [GDT_descriptor]
mov eax, cr0
or eax, 1
mov cr0, eax


jmp CODE_SEG:start_protected_mode  ; Make far jump, to flush the pipeline
jmp $


[bits 32]
start_protected_mode:

    mov ax, DATA_SEG        ; 16 = index of data segment ,  which has the base 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax 
    mov ebp, 0x7000	    ; Update our stack position so it is right
    mov esp, ebp            ; at the top of the free space.

    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    mov al, 0
    cld
    rep stosb

    ; Push bootDrive parameter on the stack
    xor edx, edx
    mov dl, [BOOT_DISK]
    push edx

    extern kmain

    call kmain
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



BOOT_DISK: db 0

; Some defines
CODE_SEG equ GDT_code - GDT_start ; 8
DATA_SEG equ GDT_data - GDT_start ; 16


GDT_descriptor:
    dw GDT_end - GDT_start - 1
    dd GDT_start


GDT_start:
    GDT_null:
        dd 0x0
        dd 0x0

    GDT_code:
        dw 0xffff       ;Limit 0-15
        dw 0x0000       ;Base 0-15
        db 0x00         ;Base 16-23
        db 0b10011010   ;Access byte
        db 0b11001111   ;Flags, Limit 16-19
        db 0x00         ;Base 24-31

    GDT_data:
        dw 0xffff
        dw 0x0000
        db 0x00
        db 0b10010010
        db 0b11001111
        db 0x00

GDT_end:



times 510-($-$$) db 0              
dw 0xaa55
