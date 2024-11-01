[bits 32]
[global outb]
[global inb]
[global outl]
[global inl]
[global memset]
[global reboot]

section .text

	outb:
		mov dx, [esp + 4]
		mov al, [esp + 8]
		out dx, al
		ret

	inb:
		mov dx, [esp + 4]
		xor eax, eax
		in al, dx
		ret

	outl:
		mov dx, [esp + 4]    ; Move the lower 16 bits of ebx (port number) into dx
		mov eax, [esp + 8]  ; Move the value to be written into eax
		out dx, eax   ; Write the value to the port
		ret

	inl:
		mov dx, [esp + 4]    ; Move the lower 16 bits of ebx (port number) into dx
		in eax, dx    ; Read a 32-bit value from the port into eax
		ret


	memset:      
		push    edi             ; proc uses edi, so save it.

		mov     ecx, [esp + 16] ; size_t num
		mov     al, [esp + 12]  ; int value 
		mov     edi, [esp + 8]  ; void * ptr
		rep     stosb 

		mov     eax, [esp + 8]  ; return pointer
		pop     edi             ; restore edi
		ret                     ; let caller adjust stack


	reboot:
		WKC:
		xor al, al
		in al, 0x64
		test al, 0x02
		jnz WKC

		mov al, 0xFC
		out 0x64, al