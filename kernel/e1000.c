#include <stddef.h>
#include <string.h>
#include "kernel.h"
#include "cpu.h"

#define LOWORD(l) ((uint16_t)(l))
#define HIWORD(l) ((uint16_t)(((uint32_t)(l) >> 16) & 0xFFFF))
#define LOBYTE(w) ((uint8_t)(w))
#define HIBYTE(w) ((uint8_t)(((uint16_t)(w) >> 8) & 0xFF))

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

struct rdesc{
	volatile uint32_t paddrLo;
	volatile uint32_t paddrHi;
	volatile uint16_t length;
	volatile uint16_t checksum;
	volatile uint8_t status;
#define RXD_STA_DD 1
#define RXD_STA_EOP (1<<1)

	volatile uint8_t errors;
	volatile uint16_t special;
}__attribute__((packed));

struct tdesc{
	volatile uint32_t paddrLo;
	volatile uint32_t paddrHi;
	volatile uint16_t length;
	volatile uint8_t cso;
	volatile uint8_t cmd;
#define TXD_CMD_EOP		1
#define TXD_CMD_IFCS	(1<<1)
#define TXD_CMD_RS		(1<<3)

	volatile uint8_t sta;
#define TXD_STA_DD 1
#define TXD_STA_EOP (1<<1)

	volatile uint8_t css;
	volatile uint16_t special;
}__attribute__((packed));

#define REG_CTRL	0x0000	//Device Control
	#define CTRL_SLU		(1<<6)
	#define CTRL_ASDE       (1<<5)
	#define CTRL_RST        (1<<26)
#define REG_STATUS	0x0008	//Device Status
#define REG_EECD	0x0010	//EEPROM Control
#define REG_EERD	0x0014	//EEPROM Read
	#define EE_MAC_ADDR_0 0x00
	#define EE_MAC_ADDR_1 0x01
	#define EE_MAC_ADDR_2 0x02
#define REG_ICR		0x00c0	//Interrupt Cause Read
#define REG_IMS		0x00d0	//Interrupt Mask Set/Read
	#define IMS_LSC		(1<<2) //Link Status Change
	#define IMS_RXT0    (1<<7) //Receiver Timer Interrupt
	#define IMS_TXDW     1
	#define IMS_TXQE    (1<<1)
#define REG_IMC		0x00d8	//Interrupt Mask Clear
#define REG_RCTL	0x0100	//Receive Control
	#define RCTL_EN			(1<<1)	//receive enable
	#define RCTL_MPE        (1<<4)  //Multicast Promiscuous Enabled
#define REG_RDBAL	0x2800	//Receive Descriptor Base Low
#define REG_RDBAH	0x2804	//Receive Descriptor Base High
#define REG_RDLEN	0x2808	//Receive Descriptor Length
#define REG_RDH		0x2810	//Receive Descriptor Head
#define REG_RDT		0x2818	//Receive Descriptor Tail
#define REG_RAL     0x5400  //Receive Address Low
#define REG_RAH     0x5404  //Receive Address High
	#define RAH_AV        (1<<31)  //Address Valid
#define REG_TCTL	0x0400	//Transmit Control
	#define TCTL_EN			(1<<1)	//transmit enable
	#define TCTL_PSP		(1<<3)	//pad short packets
	#define TCTL_CT_SHIFT	4		//indicate the re_transmission times
	#define TCTL_COLD_SHIFT	12		//don't know what's this meaning, but fellow the manual
#define REG_TDBAL	0x3800	//Transmit Descriptor Base Low
#define	REG_TDBAH	0x3804	//Transmit Descriptor Base High
#define REG_TDLEN	0x3808	//Transmit Descriptor Length
#define REG_TDH		0x3810	//Transmit Descriptor Head
#define REG_TDT		0x3818	//Transmit Descriptor Tail

#define E1000_DEVICE_ID 0x100e
#define E1000_VENDOR_ID 0x8086

#define PACKET_BUF_SIZE	2048

/*等待网络数据包的线程队列*/
static struct wait_queue *wq_nic = NULL;

struct e1000_dev {
	uint8_t mac[6];

	uint32_t mmio_addr;
	uint32_t irq;

	uint32_t      tx_desc_cnt;
	struct tdesc *tx_desc_ring;		//pointer to the transmit descripors
	uint8_t      *tx_buf_ring;

	uint32_t      rx_desc_cnt;
	struct rdesc *rx_desc_ring;		//pointer to the recieve descriptors
	uint8_t      *rx_buf_ring;
};

static struct e1000_dev g_e1000;


static uint32_t e1000_reg_read(uint32_t reg)
{
	if(reg >= 0x1ffff)
		return 0;
	uint32_t value;
	value = *(volatile uint32_t *)(g_e1000.mmio_addr + reg);
	return value;
}

static void e1000_reg_write(uint32_t reg, uint32_t value)
{
	if(reg >= 0x1ffff)
		return;
	*(volatile uint32_t *)(g_e1000.mmio_addr + reg) = value;
}

#define EEPROM_ADDR_SHIFT	8
#define EEPROM_DATA_SHIFT	16
#define EEPROM_READ_START	0x0001
#define EEPROM_READ_DONE	0x0010
static uint16_t e1000_eeprom_read(uint32_t mmio_addr, uint8_t index){
	uint32_t result;
	e1000_reg_write(REG_EERD, EEPROM_READ_START | (index << EEPROM_ADDR_SHIFT));
	do{
		result = e1000_reg_read(REG_EERD);
	} while ((result&EEPROM_READ_DONE) == 0);
	return (uint16_t)(result >> EEPROM_DATA_SHIFT);
}

static void e1000_init_txbuf(int n)
{
	int i;
	g_e1000.tx_desc_cnt = n;
	uint8_t *mem = aligned_kmalloc(g_e1000.tx_desc_cnt*(sizeof(struct tdesc)+PACKET_BUF_SIZE), 16);
	                memset(mem, 0, g_e1000.tx_desc_cnt*(sizeof(struct tdesc)+PACKET_BUF_SIZE));

	g_e1000.tx_desc_ring = (struct tdesc *)(mem);
	g_e1000.tx_buf_ring = (uint8_t *)(g_e1000.tx_desc_ring+g_e1000.tx_desc_cnt);
	for (i = 0; i < g_e1000.tx_desc_cnt; i++){
		g_e1000.tx_desc_ring[i].paddrLo = vtop((uint32_t)(g_e1000.tx_buf_ring+i*PACKET_BUF_SIZE));
	}
	e1000_reg_write(REG_TDBAL, vtop((uint32_t)(g_e1000.tx_desc_ring)));
	e1000_reg_write(REG_TDBAH, 0);
	e1000_reg_write(REG_TDLEN, sizeof(struct tdesc)*g_e1000.tx_desc_cnt);
	e1000_reg_write(REG_TDH, 0);
	e1000_reg_write(REG_TDT, 0);
}

static void e1000_init_rxbuf(int n)
{
	int i;
	g_e1000.rx_desc_cnt = n;
	uint8_t *mem = aligned_kmalloc(g_e1000.rx_desc_cnt*(sizeof(struct rdesc)+PACKET_BUF_SIZE), 16);
	                memset(mem, 0, g_e1000.rx_desc_cnt*(sizeof(struct rdesc)+PACKET_BUF_SIZE));

	g_e1000.rx_desc_ring = (struct rdesc *)(mem);
	g_e1000.rx_buf_ring = (uint8_t *)(g_e1000.rx_desc_ring+g_e1000.rx_desc_cnt);
	for (i = 0; i < g_e1000.rx_desc_cnt; i++){
		g_e1000.rx_desc_ring[i].paddrLo = vtop((uint32_t)(g_e1000.rx_buf_ring+i*PACKET_BUF_SIZE));
	}
	e1000_reg_write(REG_RDBAL, vtop((uint32_t)(g_e1000.rx_desc_ring)));
	e1000_reg_write(REG_RDBAH, 0);
	e1000_reg_write(REG_RDLEN, sizeof(struct rdesc)*g_e1000.rx_desc_cnt);
	e1000_reg_write(REG_RDH, 0);
	e1000_reg_write(REG_RDT, 0);
}

static void e1000_recv()
{
	uint32_t tail = e1000_reg_read(REG_RDT);
	while(g_e1000.rx_desc_ring[tail].status & RXD_STA_DD) {

		uint8_t *buf = &g_e1000.rx_buf_ring[tail*PACKET_BUF_SIZE];
		uint16_t length = g_e1000.rx_desc_ring[tail].length;

		if(0) {
			int i;
			printk("rx_tail : %d", tail);

			for (i = 0; i < length; i++){
				if(i%16==0)
					printk("\r\n");
				printk("%02x ", buf[i]);
			}
			printk("\r\n");
		}

		g_e1000.rx_desc_ring[tail].status = 0;

		tail = (tail+1) % g_e1000.rx_desc_cnt;

		e1000_reg_write(REG_RDT, tail);
	}
}

static void isr_e1000(uint32_t irq, struct context *ctx)
{
	uint32_t icr = e1000_reg_read(REG_ICR);

	if(icr & IMS_LSC) {
		e1000_reg_write(REG_CTRL, e1000_reg_read(REG_CTRL) | CTRL_SLU);
	}

	if(icr & IMS_RXT0) {
		//e1000_recv();
		wake_up(&wq_nic, 1);
	}
}

void e1000_getmac(uint8_t mac[])
{
	uint16_t tmp = e1000_eeprom_read(g_e1000.mmio_addr, EE_MAC_ADDR_0);
	mac[0] = LOBYTE(tmp);
	mac[1] = HIBYTE(tmp);
	tmp = e1000_eeprom_read(g_e1000.mmio_addr, EE_MAC_ADDR_1);
	mac[2] = LOBYTE(tmp);
	mac[3] = HIBYTE(tmp);
	tmp = e1000_eeprom_read(g_e1000.mmio_addr, EE_MAC_ADDR_2);
	mac[4] = LOBYTE(tmp);
	mac[5] = HIBYTE(tmp);
}

int e1000_init()
{
	int i;

	uint32_t baddr = pci_get_bar_addr(E1000_VENDOR_ID, E1000_DEVICE_ID);
	if(baddr == -1)
		return -1;
	uint32_t bsize = pci_get_bar_size(E1000_VENDOR_ID, E1000_DEVICE_ID);
	g_e1000.irq = pci_get_intr_line(E1000_VENDOR_ID, E1000_DEVICE_ID);

	g_e1000.mmio_addr = page_alloc(PAGE_ROUNDUP(bsize)/PAGE_SIZE, VM_PROT_RW, 0);
	page_map(g_e1000.mmio_addr, baddr, PAGE_ROUNDUP(bsize)/PAGE_SIZE, PTE_V | PTE_W);

	printk("E1000: On-board memory 0x%08x mapped to 0x%08x (%d pages)\r\n",
	       vtop(g_e1000.mmio_addr), g_e1000.mmio_addr, PAGE_ROUNDUP(bsize)/PAGE_SIZE);
	printk("E1000: IRQ=0x%02x\r\n", g_e1000.irq);

	e1000_reg_write(REG_CTRL, CTRL_RST);
    for(i=0;i < 20000;i++)
		__asm__ __volatile__("pause");

	e1000_reg_write(REG_CTRL, CTRL_SLU | CTRL_ASDE);

	e1000_getmac(&g_e1000.mac[0]);
	printk("E1000: MAC address: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
	       g_e1000.mac[0], g_e1000.mac[1], g_e1000.mac[2],
		   g_e1000.mac[3], g_e1000.mac[4], g_e1000.mac[5]);

	e1000_reg_write(REG_RAL+0,  *(uint32_t *)(&g_e1000.mac[0]));
	e1000_reg_write(REG_RAH+0, (*(uint16_t *)(&g_e1000.mac[4]))|RAH_AV);

	e1000_init_txbuf(4);
	e1000_reg_write(REG_TCTL,
		TCTL_EN | TCTL_PSP | (0xf << TCTL_CT_SHIFT) | (0x40 << TCTL_COLD_SHIFT));

	e1000_init_rxbuf(16);
	e1000_reg_write(REG_RCTL, RCTL_EN|RCTL_MPE);
    e1000_reg_write(REG_IMC, 0xffff);
    e1000_reg_write(REG_IMS, IMS_LSC|IMS_RXT0);

    g_intr_vector[g_e1000.irq] = isr_e1000;
    enable_irq(g_e1000.irq);

	return 0;
}

void e1000_send(uint8_t *pkt, uint32_t length)
{
	uint16_t tail = e1000_reg_read(REG_TDT);
	memcpy(&g_e1000.tx_buf_ring[tail*PACKET_BUF_SIZE], pkt, length);

	if(0) {
	 int i;
	 printk("tx_tail : %d", tail);
	 for (i = 0; i < length; i++){
		 if(i%16==0)
			 printk("\r\n");
		 printk("%02x ", g_e1000.tx_buf_ring[tail*PACKET_BUF_SIZE+i]);
	 }
	 printk("\r\n");
	}

	g_e1000.tx_desc_ring[tail].length = length;
	g_e1000.tx_desc_ring[tail].sta = 0;
	g_e1000.tx_desc_ring[tail].cmd = TXD_CMD_EOP | TXD_CMD_IFCS | TXD_CMD_RS;

	uint16_t oldtail = tail;
	tail = (tail + 1) % g_e1000.tx_desc_cnt;
	e1000_reg_write(REG_TDT, tail);

	while ((g_e1000.tx_desc_ring[oldtail].sta & TXD_STA_DD) == 0)
		;
}

ssize_t sys_recv(int sockfd, void *buf, size_t len, int flags)
{
    uint32_t _flags;

	/*在队列wq_nic上等待数据包到来*/
	save_flags_cli(_flags);
	sleep_on(&wq_nic);
	restore_flags(_flags);

	/*从网卡读取数据包*/
	uint32_t tail = e1000_reg_read(REG_RDT);
	if(g_e1000.rx_desc_ring[tail].status & RXD_STA_DD) {

		uint8_t *_buf = &g_e1000.rx_buf_ring[tail*PACKET_BUF_SIZE];
		uint16_t _len = g_e1000.rx_desc_ring[tail].length;

		len = min(len, _len);
		memcpy(buf, _buf, len);

		g_e1000.rx_desc_ring[tail].status = 0;

		tail = (tail+1) % g_e1000.rx_desc_cnt;

		e1000_reg_write(REG_RDT, tail);

		return len;
	}

    return -1;
}
