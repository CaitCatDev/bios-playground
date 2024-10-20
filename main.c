typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define PCI_CODE_TYPE_X86_PC_COMPAT 0x00

#define PCI_VENDOR_INTEL 0x8086
#define PCI_DEVICE_INTEL_82441 0x1237
#define PCI_DEVICE_INTEL_Q35_MCH 0x29c0

#define PCI_ROM_SIGNATURE 0x52494350
#define OPT_ROM_SIGNATURE 0xaa55

#define INTEL_Q35_MCH_PAM 0x90
#define INTEL_82441_PAM 0x59

#define PCI_CONFIG_ADDRESS 0xcf8
#define PCI_CONFIG_DATA 0xcfc

#define PCI_CLASS_DISPLAY 0x3
#define PCI_SUBCLASS_VGA 0x00

#define PCI_ADDR_TO_VFA(b, d, f) (b << 8) | (d << 3) | f

#define COM1_PORT 0x3f8
#define COM_DISABLE_INTS 0x00
#define COM_DLAB_LATCH 0x80
#define COM_8BIT_CHARS 0x03
#define COM_FIFO_ENABLE 0xc7
#define COM_IRQS_ENABLE 0x0f

typedef struct rom_header {
	u16 signature;
	u8 size;
	u8 init_vector[4];
	u8 reserved[17];
	u16 pcioffset;
	u16 pnpoffset;
}__attribute__((packed)) rom_header_t;

typedef struct pci_data {
	u32 signature;
  u16 vendor;
  u16 device;
  u16 vitaldata;
  u16 dlen;
  u8 drevision;
  u8 class_lo;
  u16 class_hi;
  u16 ilen;
  u16 irevision;
  u8 type;
  u8 indicator;
	u16 reserved;
}__attribute__((packed)) pci_data_t;

static inline void out8(u16 port, u8 data) {
    __asm__ volatile("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

static inline u8 in8(u16 port) {
    u8 data;
    __asm__ volatile("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void out16(u16 port, u16 data) {
    __asm__ volatile("outw %w0, %w1" : : "a" (data), "Nd" (port));
}

static inline u16 in16(u16 port) {
    u16 data;
    __asm__ volatile("inw %w1, %w0" : "=a" (data) : "Nd" (port));
    return data;
}

static inline void out32(u16 port, u32 data) {
    __asm__ volatile("outl %0, %w1" : : "a" (data), "Nd" (port));
}

static inline u32 in32(u16 port) {
    u32 data;
    __asm__ volatile("inl %w1, %0" : "=a" (data) : "Nd" (port));
    return data;
}

void pci_write_data(u32 bus, u32 device, u32 function, u32 offset, u32 data) {
	u32 address = (1<<31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xfc);

	out32(PCI_CONFIG_ADDRESS, address);

	out32(PCI_CONFIG_DATA, data);
}

u32 pci_read_data(u8 bus, u8 device, u32 function, u32 offset) {
	u32 address = (1<<31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xfc);

	out32(PCI_CONFIG_ADDRESS, address);

	return in32(PCI_CONFIG_DATA);
}

u16 pci_find_device(u8 class, u8 subclass) {
	u32 data;
	for(u32 bus = 0; bus < 256; bus++) {
		for(u32 dev = 0; dev < 32; dev++) {
			data = pci_read_data(bus, dev, 0, 0x8);
			if((data >> 24) == class && ((data >> 16) & 0xff) == subclass) {
				return PCI_ADDR_TO_VFA(bus, dev, 0);
			}
		}
	}

	/*dev not found*/
	return 0xffff;
}


/* This spaghetti wouldn't fly in a actual PC we would have to init
 * the SERIAL port and possibly a superIO or other controllering chip
 * But we run in a VM
 */
void init_serial(u16 base) {
	out8(base + 1, COM_DISABLE_INTS);
	out8(base + 3, COM_DLAB_LATCH);
	out8(base, 3);
	out8(base + 1, 0x00); /*Set the baud*/
	out8(base + 3, COM_8BIT_CHARS);
	out8(base + 2, COM_FIFO_ENABLE);
	out8(base + 4, COM_IRQS_ENABLE);
}

char read_serial(u16 base) {

   return in8(base);
}

void write_serial(u16 base, char ch) {

	out8(base, ch);
}

void puts_serial(u16 base, const char *str) {
	for(; *str; str++) {
		write_serial(base, *str);
	}
}

#warning "This code is intented to be run int a QEMU VM only it may work in other VMs or even on some hardware but please don't run this on bare metal"

void *mcpy(void *dst, void *src, u32 bytes) {
	u8 *dst8 = dst;
	u8 *src8 = src;

	for(u32 i = 0; i < bytes; i++) {
		dst8[i] = src8[i];
	}

	return dst;
}

void unlock_host_bus_memory(u32 bus, u32 device, u32 func, u32 offset) {
	u32 pam_lower, pam_upper;

	/*PCI function will align offset*/
	pam_lower = pci_read_data(bus, device, func, offset);
	pam_upper = pci_read_data(bus, device, func, offset+4);

	if(offset & 0x3) { /*440fx*/
		pam_lower = 0x33330000;
		pam_upper = 0x3333;
	} else {
		pam_upper = 0x33;
		pam_lower = 0x33333300;
	}
	
	pci_write_data(bus, device, func, offset, pam_lower);
	pci_write_data(bus, device, func, offset+4, pam_upper);
}

int upper_meminit() {
	u32 data;
	for(u32 bus = 0; bus < 256; bus++) {
		for(u32 dev = 0; dev < 32; dev++) {
			data = pci_read_data(bus, dev, 0, 0x0);
			if((data & 0xffff) == PCI_VENDOR_INTEL) {
				switch((data >> 16) & 0xffff) {
					case PCI_DEVICE_INTEL_82441:
						puts_serial(COM1_PORT, "440FX Host Bus Found\r\n");
						unlock_host_bus_memory(bus, dev, 0, INTEL_82441_PAM);
						return 0;
					case PCI_DEVICE_INTEL_Q35_MCH:
						puts_serial(COM1_PORT, "Q35 Host Bus Found\r\n");
						unlock_host_bus_memory(bus, dev, 0, INTEL_Q35_MCH_PAM);
						return 0;
					default:
						puts_serial(COM1_PORT, "Unknown Intel Device\r\n");
				}
			}
		}
	}

	return 1;
}

int pci_load_vga_rom(u16 bdf) {
	u32 bus, dev, func, data;
	rom_header_t *rom;
	pci_data_t *pci_data;
	func = bdf & 0x7;
	dev = (bdf >> 3) & 0x1f;
	bus = (bdf >> 8);
	
	data = pci_read_data(bus, dev, func, 0x4);
	data |= 3; /*Enable MMIO and PIO*/

	pci_write_data(bus, dev, func, 0x4, data);

	pci_write_data(bus, dev, func, 0x30, 0xe0001);

	rom = (void*)0xe0000;
	if(rom->signature != OPT_ROM_SIGNATURE) {
		puts_serial(COM1_PORT, "VGA Card found but Option ROM type unknown\r\n");
		return 1;
	}

	pci_data = (void*)0xe0000 + rom->pcioffset;
	/* We should also check the checksum but eeee to much work and it's
	 * fine for a VM
	 */
	if(pci_data->signature != PCI_ROM_SIGNATURE) {
		puts_serial(COM1_PORT, "Unable To Determine ROM code Type\r\n");
		return 1;
	}

	if(pci_data->type != PCI_CODE_TYPE_X86_PC_COMPAT) {
		puts_serial(COM1_PORT, "Unknown Code Type On ROM\r\n");
		return 1;
	}


	/*Alles GUT*/
	mcpy((void*)0xc0000, (void*)0xe0000, rom->size * 512);

	/*Unmap the ROM now we have it in RAM*/
	pci_write_data(bus, dev, func, 0x30, 0x0);

	return 0;
}

void call_rom(u16 bdf);
void set_vga_mode();
extern const char _bios_mesg;

void puts(const char *str, u16 y, u16 x) {
	volatile char *vga = (void*)0xb8000 + x + y * 160;
	for(u32 i = 0;str[i];i++) {
		vga[(i * 2)] = str[i];
	}
}

int cmain() {
	u16 bdf;
	init_serial(COM1_PORT);
	
	puts_serial(COM1_PORT, &_bios_mesg);
	puts_serial(COM1_PORT, "\r\n");

	upper_meminit();

	bdf = pci_find_device(PCI_CLASS_DISPLAY, PCI_SUBCLASS_VGA);
	if(bdf == 0xffff) {
		puts_serial(COM1_PORT, "There isn't a VGA device connected\r\n");
		return 0xdeadbeef;
	}

	if(pci_load_vga_rom(bdf)) {
		return 0xdeadbeef;
	}
	call_rom(bdf);
	set_vga_mode();

	puts(&_bios_mesg, 0, 0);
	puts("Hello VGA World", 1, 0);

	return 0xcafebabe;
}
