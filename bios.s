.code16

.global call_rom
.global set_vga_mode
.global _bios_mesg
.global _start

.section .head,"a"
_bios_mesg: .asciz "Catbios V1.00"
.align 16

.section .text,"ax"

ivt:
	.word 0x3ff
	.long 0x000

PNP_INSTALL:
	.ascii "$PnP"
	.byte 0x10
	.byte 0x21
	.word 0x00
	.byte 0x2f
	.long 0x00
	.word 0xf000
	.word 0xf000
	.word 0xf000
	.long 0xf000
	.long 0x00
	.word 0xf000
	.long 0xf000

_GDT:
	.null:
	.quad 0x00
	.code16:
	.word 0xffff
	.word 0x0000
	.byte 0x0f
	.byte 0x9a
	.byte 0x8f
	.byte 0x00
	.data16:
	.word 0xffff
	.word 0x0000
	.byte 0x01
	.byte 0x92
	.byte 0x8f
	.byte 0x00
	.code32:
	.word 0xffff
	.word 0x0000
	.byte 0x00
	.byte 0x9a
	.byte 0xcf
	.byte 0x00
	.data32:
	.word 0xffff
	.word 0x0000
	.byte 0x00
	.byte 0x92
	.byte 0xcf
	.byte 0x00
	.code64:
	.word 0x0000
	.word 0x0000
	.byte 0x00
	.byte 0x9a
	.byte 0x20
	.byte 0x00
	.data64:
	.word 0x0000
	.word 0x0000
	.byte 0x00
	.byte 0x92
	.byte 0x00
	.byte 0x00	
.end:

_GDTR:
	.word 0x37
	.long _GDT

_init16:
	cli
	cld
	mov %cs,%ax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%ss
	/*TODO could use the 0xf000:0x0000 shadow map of the BIOS*/
	lgdtl _GDTR
	
	mov %cr0,%eax
	or $1,%al
	mov %eax,%cr0

	jmpl $0x18,$_init32

.code32
_init32:
	mov $0x20,%ax
	mov %ax,%ds
	mov %ax,%ss
	mov %ax,%es
	
	mov $0x7c00,%sp
	
	call cmain

_halt_forever:
	cli
	hlt
	jmp _halt_forever

call_rom:
	push %ebp
	mov %esp,%ebp
	
	pushal

	movl 0x28(%ebp),%ecx

	//Encode A far Jump
	jmpl $0x08,$call_rom_16 - 0xffff0000

set_vga_mode:
	push %ebp
	mov %esp,%ebp
	
	pushal

	//Encode A far Jump
	jmpl $0x08,$set_vga_mode_16 - 0xffff0000


call_rom_ret:
	mov $0x20,%ax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%ss
	
	popal
	mov %ebp,%esp
	pop %ebp
	ret

.code16
set_vga_mode_16:
	mov $0x10,%ax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%ss

	mov %cr0,%eax
	xor $1,%eax
	mov %eax,%cr0
	
	jmp $0xf000,$set_vga_real - 0xffff0000

set_vga_real:
	mov $0xf000,%ax
	mov %ax,%ds
	mov %ax,%es
	xor %ax,%ax
	mov %ax,%ss

	mov $0x3,%ax
	int $0x10

	mov %cr0,%eax
	or $1,%eax
	mov %eax,%cr0
	jmpl $0x18,$call_rom_ret


call_rom_16:
	mov $0x10,%ax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%ss

	mov %cr0,%eax
	xor $1,%eax
	mov %eax,%cr0
	/*We should probably do some linker magic here*/
	jmp $0xf000,$call_rom_real - 0xffff0000

call_rom_real:
	xor %ax,%ax
	mov %ax,%ss
	mov $0xf000,%ax
	mov %ax,%es
	mov %ax,%ds
	
	mov %cx,%ax
	mov $PNP_INSTALL,%di
	
	call $0xc000,$3

	mov %cr0,%eax
	or $1,%eax
	mov %eax,%cr0
	jmpl $0x18,$call_rom_ret


.section .reset, "ax"
_start:
        .byte  0xe9
        .int   _init16 - ( . + 2 )
        .align 16, 0xff
