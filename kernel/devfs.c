#include <string.h>
#include "kernel.h"

struct dev_file {
	struct file file;
	struct dev *dev;
	int mode;
	uint32_t pointer;
};

static int devfs_mount(struct fs *this, struct dev *dev, uint32_t addr);
static int devfs_unmount(struct fs *this);
static int devfs_open(struct fs *this, char *name, int mode, struct file **_fpp);
static int devfs_close  (struct file *_fp);
static int devfs_read   (struct file *_fp, uint8_t *buf, size_t size);
static int devfs_write  (struct file *_fp, uint8_t *buf, size_t size);
static int devfs_seek   (struct file *_fp, int offset, int whence);

struct dev_fs {
	struct fs fs;
} dev_fs = {
	{
	.mount = devfs_mount,
	.unmount = devfs_unmount,
	.open = devfs_open,
	.close = devfs_close,
	.read = devfs_read,
	.write = devfs_write,
	.seek = devfs_seek,
	}
};

static int devfs_mount(struct fs *this, struct dev *dev, uint32_t addr)
{
	return 0;
}

static int devfs_unmount(struct fs *this)
{
	return 0;
}

static int devfs_open(struct fs *this, char *name, int mode, struct file **_fpp)
{
	int i;
	char fullname[16];

	if(mode & O_APPEND)
		return -1;

	for(i = 0; i < NR_DEVICE; i++) {
		snprintf(fullname, sizeof(fullname), "%s%d",
		         g_dev_vector[i]->drv->name,
		         g_dev_vector[i]->minor);
		if(strncmp(fullname, name, strlen(fullname)) == 0)
			break;
	}
	if(i == NR_DEVICE)
		return -1;

	if(g_dev_vector[i]->drv->attach(g_dev_vector[i]) != 0)
		return -1;

	struct dev_file *fp = (struct dev_file *)kmalloc(sizeof(struct dev_file));
	fp->file.fs = this;
	fp->dev = g_dev_vector[i];
	fp->pointer = 0;
	fp->mode = mode;
	*_fpp = fp;

	return 0;
}

static int devfs_close  (struct file *_fp)
{
	struct dev_file *fp = (struct dev_file *)_fp;
	kfree(fp);
	return 0;
}

static int devfs_read   (struct file *_fp, uint8_t *buf, size_t size)
{
	struct dev_file *fp = (struct dev_file *)_fp;

	if((fp->mode & 1) != O_RDONLY)
		return -1;

	int retval = fp->dev->drv->read(fp->dev, fp->pointer, buf, size);
	if(retval >= 0) {
		fp->pointer += retval;
		return retval;
	}
	return retval;
}

static int devfs_write  (struct file *_fp, uint8_t *buf, size_t size)
{
	struct dev_file *fp = (struct dev_file *)_fp;

	if(((fp->mode & 1) != O_WRONLY) && ((fp->mode & 2) != O_RDWR))
		return -1;

	int retval = fp->dev->drv->write(fp->dev, fp->pointer, buf, size);
	if(retval >= 0) {
		fp->pointer += retval;
		return retval;
	}
	return retval;
}

static int devfs_seek   (struct file *_fp, int offset, int whence)
{
	struct dev_file *fp = (struct dev_file *)_fp;
	switch(whence) {
	case SEEK_SET:
		fp->pointer = offset;
		break;
	case SEEK_CUR:
		fp->pointer += offset;
		break;
	default:
		return -1;
	}
	return fp->pointer;
}
