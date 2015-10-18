#include "cpu.h"
#include "global.h"

#define PCI_CONFIG_ADDR      0xCF8
#define PCI_CONFIG_DATA      0xCFC
#define PCI_BAR_0            0x10

struct pci_device_conf {
	uint16_t vendor_id;
	uint16_t device_id;
	uint16_t command;
	uint16_t status;
	uint8_t rev;
	uint8_t prog_if;
	uint8_t sub_class;
	uint8_t class_code;
	uint8_t cache_line_size;
	uint8_t latency_timer;
	uint8_t header_type;
	uint8_t bist;

	//if header_type&7f == 0
	uint32_t bars[6];
	uint32_t reserved[2];
	uint32_t romaddr;
	uint32_t reserved2[2];
	uint8_t intr_line;
	uint8_t intr_pin;
	uint8_t min_grant;
	uint8_t max_latency;
	uint8_t data[192];
} __attribute__((packed));

#define MAKE_BDF(bus,dev,func) (uint32_t)(((bus)<<8)|((dev)<<3)|(func))
#define GET_BUS(x)  (((x)>>8)&0xff)
#define GET_DEV(x)  (((x)>>3)&0x1f)
#define GET_FUNC(x)  ((x)&0x7)

#define PCI_CONF_ADDR(bus,dev,func,reg)  (0x80000000|(MAKE_BDF(bus, dev, func)<<8)|((uint32_t)(reg & 0xfc)))

struct pci_device {
	uint32_t bdf;//bus, device, func
	struct pci_device_conf hdr;
};

static int nr_pci_device = 0;
static struct pci_device pci_devices[16];/*XXX*/

static uint32_t pci_read(uint32_t addr){
	outportl(PCI_CONFIG_ADDR, addr);
	return inportl(PCI_CONFIG_DATA);
}

static void pci_write(uint32_t addr, uint32_t data){
	outportl(PCI_CONFIG_ADDR, addr);
	outportl(PCI_CONFIG_DATA, data);
}

static void pci_device_read_config(struct pci_device_conf *hdr, uint32_t bus, uint32_t dev, uint32_t func)
{
	uint32_t *p = (uint32_t *)hdr;
	uint8_t reg;
	for(reg = 0; reg < 0x40; reg++) {
		*p=pci_read(PCI_CONF_ADDR(bus, dev, func, reg));
		p++;
	}
}

static void pci_bus_scan(uint32_t bus)
{
	struct pci_device_conf yyy;
	uint32_t i, j;

	for(i = 0; i < 0x20; i++){
		pci_device_read_config(&yyy, bus, i, 0);

		if(yyy.vendor_id == 0xFFFF)
			continue;
#if VERBOSE
		printk("PCI: bus %d slot %d func %d, VID=0x%04x PID=0x%04x\r\n", bus, i, 0, yyy.vendor_id, yyy.device_id);
#endif
		pci_devices[nr_pci_device].bdf = MAKE_BDF(bus, i, 0);
		pci_devices[nr_pci_device].hdr = yyy;
		nr_pci_device++;

		if((yyy.header_type & 0x80) != 0) {
			//This device has multiple functions
			for(j = 1; j < 8; j++) {
				pci_device_read_config(&yyy, bus, i, j);

				if(yyy.vendor_id == 0xFFFF)
					continue;
#if VERBOSE
				printk("     bus %d slot %d func %d, VID=0x%04x PID=0x%04x\r\n", bus, i, j, yyy.vendor_id, yyy.device_id);
#endif
				pci_devices[nr_pci_device].bdf = MAKE_BDF(bus, i, j);
				pci_devices[nr_pci_device].hdr = yyy;

				nr_pci_device++;
			}
		}
	}
}

static uint32_t _pci_get_bar_addr(uint32_t bus, uint32_t dev)
{
	return pci_read(PCI_CONF_ADDR(bus, dev, 0, PCI_BAR_0));
}

static uint32_t _pci_get_bar_size(uint32_t bus, uint32_t dev)
{
	uint32_t old = _pci_get_bar_addr(bus, dev);
	outportl(PCI_CONFIG_DATA, 0xFFFFFFFF);
	uint32_t size = inportl(PCI_CONFIG_DATA);
	outportl(PCI_CONFIG_DATA, old);
	return (~size)+1;
}

static uint32_t get_bdf_by_vendor_and_device(uint16_t vendor, uint16_t device) {
	int i;
	for(i = 0; i < nr_pci_device; i++) {
		if(pci_devices[i].hdr.vendor_id == vendor && pci_devices[i].hdr.device_id == device)
			return pci_devices[i].bdf;
	}

	return 0;
}

void pci_init()
{
	uint32_t i;
	for(i = 0; i < 0xFF; i++) {
		if(pci_read(PCI_CONF_ADDR(i, 0, 0, 0)) == 0xFFFFFFFF)
			continue;

		pci_bus_scan(i);
	}
}

uint32_t pci_get_bar_addr(uint16_t vendor, uint16_t device)
{
	uint32_t bdf = get_bdf_by_vendor_and_device(vendor, device);
	if(bdf)
		return _pci_get_bar_addr(GET_BUS(bdf), GET_DEV(bdf));

	return -1;
}

uint32_t pci_get_bar_size(uint16_t vendor, uint16_t device)
{
	uint32_t bdf = get_bdf_by_vendor_and_device(vendor, device);
	if(bdf)
		return _pci_get_bar_size(GET_BUS(bdf), GET_DEV(bdf));

	return -1;
}

uint8_t pci_get_intr_line(uint16_t vendor, uint16_t device)
{
	int i;
	for(i = 0; i < nr_pci_device; i++) {
		if(pci_devices[i].hdr.vendor_id == vendor && pci_devices[i].hdr.device_id == device)
			return pci_devices[i].hdr.intr_line;
	}

	return 0xff;
}