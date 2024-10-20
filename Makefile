CC=clang
AS=clang
LD=ld.lld

CFLAGS=-m32 -nostdlib

COBJS=./bios.o ./main.o

rom.bin: rom.elf
	objcopy -O binary -j .head -j .text -j .reset --gap-fill=0x0ff rom.elf rom.bin

rom.elf: $(COBJS)
	ld.lld -nostdlib -m elf_i386 -T bios.ld $(COBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) -fno-PIC -fno-PIE -ffreestanding -mno-red-zone -c $< -o $@

.s.o:
	$(AS) $(CFLAGS) -c $< -o $@

clean:
	rm $(COBJS) rom.elf rom.bin
