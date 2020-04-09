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
	.name = "zero",
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
	.name = "null",
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
