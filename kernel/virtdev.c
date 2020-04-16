#include <string.h>
#include "kernel.h"

static int zero_attach(struct dev *dp)
{
	return 0;
}

static void zero_detach(struct dev *dp)
{
	return;
}

static int zero_read(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size)
{
	memset(buf, 0, buf_size);
	return buf_size;
}

static int zero_write(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size)
{
	return -1;
}

static int zero_poll(struct dev *dp, int events)
{
	return 1;
}

static int zero_ioctl(struct dev *dp, int cmd, void *arg)
{
	return -1;
}

static struct driver zero_driver = {
	.major = "zero",
	.attach = zero_attach,
	.detach = zero_detach,
	.read = zero_read,
	.write = zero_write,
	.poll = zero_poll,
	.ioctl = zero_ioctl
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
	.attach = zero_attach,
	.detach = zero_detach,
	.read = zero_write,
	.write = null_write,
	.poll = zero_poll,
	.ioctl = zero_ioctl
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

static void uart_detach(struct dev *dp)
{
	return;
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

static int uart_poll(struct dev *dp, int events)
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

static int uart_ioctl(struct dev *dp, int cmd, void *arg)
{
	return -1;
}

static struct driver uart_driver = {
	.major = "uart",
	.attach = uart_attach,
	.detach = uart_detach,
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
