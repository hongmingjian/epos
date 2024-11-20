#include <stddef.h>
#include "cpu.h"
#include "kernel.h"

#define PCI_CONFIG_ADDR      0xCF8
#define PCI_CONFIG_DATA      0xCFC
#define PCI_BAR_0            0x10

struct pci_device_header {
	uint16_t vendor_id;
	uint16_t product_id;
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
} __attribute__((packed));

// Standard Header
//  header_type&7f == 0
struct pci_device_std {
	struct pci_device_header hdr;

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

#define MAKE_BSF(bus,slot,func) (uint32_t)(((bus)<<8)|((slot)<<3)|(func))
#define GET_BUS(x)  (((x)>>8)&0xff)
#define GET_SLOT(x)  (((x)>>3)&0x1f)
#define GET_FUNC(x)  ((x)&0x7)

#define PCI_CONF_ADDR(bus,slot,func,reg) \
        (0x80000000|(MAKE_BSF(bus,slot,func)<<8)|((uint32_t)(reg&0xfc)))

struct pci_device {
	uint32_t bsf;//bus, slot, func
	struct pci_device_std conf;
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

#pragma GCC push_options
#pragma GCC optimize("O0")
static void pci_device_read_config(struct pci_device_std *conf,
                                   uint32_t bus, uint32_t slot, uint32_t func)
{
	uint32_t *p = (uint32_t *)conf;
	uint32_t reg;
	for(reg = 0; reg < 0x40; reg+=4) {
		*p=pci_read(PCI_CONF_ADDR(bus, slot, func, reg));
		p++;
	}
}
#pragma GCC pop_options

static void pci_bus_scan(uint32_t bus)
{
	struct pci_device_std yyy = {0};
	uint32_t slot, func;

	for(slot = 0; slot < 0x20; slot++){
		pci_device_read_config(&yyy, bus, slot, 0);

		if(yyy.hdr.vendor_id == 0xFFFF)
			continue;

		/*XXX - 忽略PCI-to-PCI/CardBus bridge*/
		if(yyy.hdr.header_type & 0x7f)
			continue;
#if VERBOSE
		printk("PCI: bus %d slot %d func %d, VID=0x%04x PID=0x%04x\r\n",
				bus, slot, 0, yyy.hdr.vendor_id, yyy.hdr.product_id);
#endif
		pci_devices[nr_pci_device].bsf = MAKE_BSF(bus, slot, 0);
		pci_devices[nr_pci_device].conf = yyy;
		nr_pci_device++;

		if((yyy.hdr.header_type & 0x80) != 0) {
			//This device has multiple functions
			for(func = 1; func < 8; func++) {
				pci_device_read_config(&yyy, bus, slot, func);

				if(yyy.hdr.vendor_id == 0xFFFF)
					continue;

				/*XXX - 忽略PCI-to-PCI/CardBus bridge*/
				if(yyy.hdr.header_type & 0x7f)
					continue;
#if VERBOSE
				printk("     bus %d slot %d func %d, VID=0x%04x PID=0x%04x\r\n",
						bus, slot, func, yyy.hdr.vendor_id, yyy.hdr.product_id);
#endif
				pci_devices[nr_pci_device].bsf = MAKE_BSF(bus, slot, func);
				pci_devices[nr_pci_device].conf = yyy;

				nr_pci_device++;
			}
		}
	}
}

static struct pci_device *get_device(uint16_t vendor, uint16_t product)
{
	int i;
	for(i = 0; i < nr_pci_device; i++) {
		if(pci_devices[i].conf.hdr.vendor_id  == vendor &&
           pci_devices[i].conf.hdr.product_id == product)
			return &pci_devices[i];
	}

	return NULL;
}

uint32_t pci_get_bar_addr(uint16_t vendor, uint16_t product/*, uint32_t index*/)
{
	struct pci_device *dev = get_device(vendor, product);
	if(dev != NULL) {
		return (dev->conf.bars[0/*+index*/]) & (~0xf);
	}
	return -1;
}

uint32_t pci_get_bar_size(uint16_t vendor, uint16_t product/*, uint32_t index*/)
{
	struct pci_device *dev = get_device(vendor, product);
	if(dev != NULL) {
		uint32_t addr = PCI_CONF_ADDR(GET_BUS(dev->bsf),
									  GET_SLOT(dev->bsf),
									  GET_FUNC(dev->bsf),
									  PCI_BAR_0/*+index*4*/);
		uint32_t old = pci_read(addr);
		pci_write(addr, 0xFFFFFFFF);
		uint32_t size = pci_read(addr);
		pci_write(addr, old);
		return ~(size & (~0xf)) + 1;
	}
	return -1;
}

uint8_t pci_get_intr_line(uint16_t vendor, uint16_t product)
{
	struct pci_device *dev = get_device(vendor, product);
	if(dev != NULL)
		return dev->conf.intr_line;

	return 0xff;
}

void pci_init()
{
	uint32_t bus;
	nr_pci_device = 0;

	for(bus = 0; bus < 0x100; bus++) {
		if(pci_read(PCI_CONF_ADDR(bus, 0, 0, 0)) == 0xFFFFFFFF)
			continue;

		pci_bus_scan(bus);
	}
}
