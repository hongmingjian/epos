#include <string.h>
#include "kernel.h"

static int common_attach(struct dev *dp)
{
	return 0;
}

static void common_detach(struct dev *dp)
{
	return;
}

static int common_readwrite(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size)
{
	return -1;
}

static int common_poll(struct dev *dp, short events)
{
	return -1;
}

static int common_ioctl(struct dev *dp, uint32_t cmd, void *arg)
{
	return -1;
}

static int zero_read(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size)
{
	memset(buf, 0, buf_size);
	return buf_size;
}

static int zero_poll(struct dev *dp, short events)
{
	return 1;
}

static struct driver zero_driver = {
	.major = "zero",
	.attach = common_attach,
	.detach = common_detach,
	.read = zero_read,
	.write = common_readwrite,
	.poll = zero_poll,
	.ioctl = common_ioctl
};

struct zero_dev {
	struct dev dev;
} zero_dev = {
	{
	.drv = &zero_driver,
	.minor = 0
	}
};
/*****************************************************************************/
static int null_write(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size)
{
	return buf_size;
}

static struct driver null_driver = {
	.major = "null",
	.attach = common_attach,
	.detach = common_detach,
	.read = common_readwrite,
	.write = null_write,
	.poll = zero_poll,
	.ioctl = common_ioctl
};

struct null_dev {
	struct dev dev;
} null_dev = {
	{
	.drv = &null_driver,
	.minor = 0
	}
};
/*****************************************************************************/
static int uart_attach(struct dev *dp)
{
	void (*pfn)(uint32_t);

	switch(dp->minor) {
	case 0:
		pfn = init_uart0;
		break;
	case 1:
		pfn = init_uart1;
		break;
	default:
		return -1;
	}

	pfn(115200);

	return 0;
}

static int uart_read(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size)
{
	int (*pfn)();

	switch(dp->minor) {
	case 0:
		pfn = uart0_getc;
		break;
	case 1:
		pfn = uart1_getc;
		break;
	default:
		return -1;
	}

	uint8_t *oldbuf = buf;
	while(buf_size) {
		*buf=pfn();
		buf++;
		buf_size--;
	}

	return buf-oldbuf;
}

static int uart_write(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size)
{
	void (*pfn)(int);

	switch(dp->minor) {
	case 0:
		pfn = uart0_putc;
		break;
	case 1:
		pfn = uart1_putc;
		break;
	default:
		return -1;
	}

	uint8_t *oldbuf = buf;
	while(buf_size) {
		pfn(*buf);
		buf++;
		buf_size--;
	}

	return buf-oldbuf;
}

static int uart_poll(struct dev *dp, short events)
{
	int retval = POLLOUT;
	if(events & POLLIN) {
		int (*pfn)();

		switch(dp->minor) {
		case 0:
			pfn = uart0_hasc;
			break;
		case 1:
			pfn = uart1_hasc;
			break;
		default:
			return -1;
		}

		if(pfn())
			retval |= POLLIN;
	}

	return retval;
}

static int uart_ioctl(struct dev *dp, uint32_t cmd, void *arg)
{
	return -1;
}

static struct driver uart_driver = {
	.major = "uart",
	.attach = uart_attach,
	.detach = common_detach,
	.read = uart_read,
	.write = uart_write,
	.poll = uart_poll,
	.ioctl = uart_ioctl
};

struct uart_dev0 {
	struct dev dev;
} uart_dev0 = {
	{
	.drv = &uart_driver,
	.minor = 0
	}
};

struct uart_dev1 {
	struct dev dev;
} uart_dev1 = {
	{
	.drv = &uart_driver,
	.minor = 1
	}
};
/*****************************************************************************/
static int led_attach(struct dev *dp);
static int led_ioctl(struct dev *dp, uint32_t cmd, void *arg);

static struct driver led_driver = {
	.major = "led",
	.attach = led_attach,
	.detach = common_detach,
	.read = common_readwrite,
	.write = common_readwrite,
	.poll = common_poll,
	.ioctl = led_ioctl
};

struct led_dev {
	struct dev dev;
	int gpio;
	int active_low;
} led_dev = {
	{
	.drv = &led_driver,
	.minor = 0
	},
	//16, 1//1B
    //47, 0//1B+(V1.2), 2B(V1.1)
	29, 1//3B+
};

static int led_attach(struct dev *dp)
{
	struct led_dev *ldp = (struct led_dev *)dp;
	gpio_reg_t *grt = (gpio_reg_t *)(MMIO_BASE_VA+GPIO_REG);

	uint32_t *p = (uint32_t *)(((uint32_t)(&grt->gpfsel0))+(ldp->gpio/10)*4);

	int bit=(ldp->gpio%10)*3;

	uint32_t old = *p;
	old &= ~(7<<bit);
	old |= (1<<bit); //output
	*p = old;

	return 0;
}

static int led_ioctl(struct dev *dp, uint32_t cmd, void *arg)
{
	struct led_dev *ldp = (struct led_dev *)dp;
	gpio_reg_t *grt = (gpio_reg_t *)(MMIO_BASE_VA+GPIO_REG);
	uint32_t *p;

	if(cmd)
		cmd = 1;

	if(ldp->active_low)
		cmd ^= 1;

	p = (uint32_t *)((uint32_t)(cmd?(&grt->gpset0):(&grt->gpclr0))+(ldp->gpio/32)*4);
	*p = 1<<(ldp->gpio&0x1f);

	return -1;
}
