[bits 64]
[global outb]
[global inb]
[global outl]
[global inl]
[global memset]
[global reboot]

section .text

	outb:
		mov dx, di		; Move port into dx
		mov al, sil		; Move value into al
		out dx, al
		ret

	inb:
		mov dx, di		; Move port into dx
		xor rax, rax	; Clear rax
		in al, dx
		ret

	outl:
		mov dx, di    ; Move the lower 16 bits of ebx (port number) into dx
		mov eax, esi  ; Move the value to be written into eax
		out dx, eax   ; Write the value to the port
		ret

	inl:
		mov dx, di    ; Move the lower 16 bits of ebx (port number) into dx
		in eax, dx    ; Read a 32-bit value from the port into eax
		ret


	memset:      
		push rdi            ; proc uses rdi, so save it.

		mov rcx, rdx		; size_t num
		mov al,  sil		; int value 
		mov rdi, rdi		; void * ptr
		rep stosb 

		mov rax, rdi  ; return pointer
		pop rdi             ; restore rdi
		ret


	reboot:
		WKC:
		xor al, al
		in al, 0x64
		test al, 0x02
		jnz WKC

		mov al, 0xFC
		out 0x64, al