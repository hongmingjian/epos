#include <string.h>
#include "kernel.h"

#define min(x,y) (((x)>(y))?(y):(x))

static int fb_attach(struct dev *dp);
static void fb_detach(struct dev *dp);
static int fb_read(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size);
static int fb_write(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size);
static int fb_poll(struct dev *dp, short events);
static int fb_ioctl(struct dev *dp, uint32_t cmd, void *arg);

static struct driver fb_driver = {
	.major = "fb",
	.attach = fb_attach,
	.detach = fb_detach,
	.read = fb_read,
	.write = fb_write,
	.poll = fb_poll,
	.ioctl = fb_ioctl
};

struct fb_dev {
	struct dev dev;
	int XResolution, YResolution;
	int BytesPerScanLine;
	int BitsPerPixel;
	uint8_t *FrameBuffer;
	int FrameBufferSize;
} fb_dev = {
	{
	.drv = &fb_driver,
	.minor = 0
	}
};

/**
 * https://www.raspberrypi.org/forums/viewtopic.php?t=155825
 */
static int fb_attach(struct dev *dp)
{
	struct fb_dev *dev = (struct fb_dev *)dp;

    uint32_t  __attribute__((aligned(16))) msg[40] =
    {
        sizeof(msg),            // Message size
        MAILBOX_REQUEST,        // Request/Response

        TAG_SET_PHYSICAL_WIDTH_HEIGHT,
        8,                      // # bytes of buffer
        0,
        1920,                   // width
        1280,                    // height

        TAG_SET_VIRTUAL_WIDTH_HEIGHT,
        8,                      // # bytes of buffer
        0,
        1920,                   // width
        1280,                    // height

        TAG_SET_COLOUR_DEPTH,
        4,                      // # bytes of buffer
        0,
        32,                     // depth

        TAG_GET_PHYSICAL_WIDTH_HEIGHT,
        8,                      // # bytes of buffer
        0,
        0,                      // width
        0,                      // height

        TAG_GET_VIRTUAL_WIDTH_HEIGHT,
        8,                      // # bytes of buffer
        0,
        0,                      // width
        0,                      // height

        TAG_GET_COLOUR_DEPTH,
        4,                      // # bytes of buffer
        0,
        0,                      // depth

        TAG_ALLOCATE_FRAMEBUFFER,
        8,                      // # bytes of buffer
        0,
        PAGE_SIZE,              // in: alignment, out: address
        0,                      // size

        TAG_GET_PITCH,
        4,                      // # bytes of buffer
        0,
        0,                      // pitch

        0,                      // End Tag
    };

    if(0 == mailbox_write_read(CHANNEL_TAGS, vtop((uint32_t)&msg[0])|0xc0000000)) {
	    uint32_t vram;
	    dev->XResolution = msg[19];
	    dev->YResolution = msg[20];
	    dev->BytesPerScanLine = msg[38];
	    dev->BitsPerPixel = msg[29];
	    vram = msg[33] & 0x3fffffff;
	    dev->FrameBufferSize = msg[34];

	    struct vmzone *z = page_alloc(PAGE_ROUNDUP(dev->FrameBufferSize)/PAGE_SIZE,
	                                  PROT_READ|PROT_WRITE, 0, MAP_PRIVATE, NULL, 0);
	    if(z == NULL) {
			uint32_t  __attribute__((aligned(16))) msg[6] =
			{
				sizeof(msg),            // Message size
				MAILBOX_REQUEST,        // Request/Response

				TAG_RELEASE_FRAMEBUFFER,// Tag
				0,                      // # bytes of buffer
				0,

				0,                      // End Tag
			};
			mailbox_write_read(CHANNEL_TAGS, vtop((uint32_t)&msg[0]));
			printk("Failed to accomodate the frame buffer\r\n");
			return -1;
	    }
	    page_map(z->base, vram, z->limit/PAGE_SIZE, L2E_V|L2E_W);
	    dev->FrameBuffer = z->base;

	    printk("%s%d: %dx%dx%d %d 0x%08x->0x%08x(0x%08x) %d\r\n",
	           dp->drv->major, dp->minor, dev->XResolution,
	           dev->YResolution, dev->BitsPerPixel,
	           dev->BytesPerScanLine, dev->FrameBuffer,
	           vram, msg[33], dev->FrameBufferSize);
    } else {
        printk("Failed to allocate a frame buffer\r\n");
        return -1;
    }

	return 0;
}

static void fb_detach(struct dev *dp)
{
	struct fb_dev *dev = (struct fb_dev *)dp;

	page_unmap(dev->FrameBuffer, dev->FrameBufferSize/PAGE_SIZE);
    page_free(dev->FrameBuffer, dev->FrameBufferSize/PAGE_SIZE);

    uint32_t  __attribute__((aligned(16))) msg[6] =
    {
        sizeof(msg),            // Message size
        MAILBOX_REQUEST,        // Request/Response

        TAG_RELEASE_FRAMEBUFFER,// Tag
        0,                      // # bytes of buffer
        0,

        0,                      // End Tag
    };
    mailbox_write_read(CHANNEL_TAGS, vtop((uint32_t)&msg[0]));
	return;
}

static int fb_read(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size)
{
	struct fb_dev *dev = (struct fb_dev *)dp;

	if(addr > dev->FrameBufferSize)
		return -1;
	if(buf_size == 0)
		return 0;

	int nread = min(buf_size, dev->FrameBufferSize-addr);
	memcpy(buf, dev->FrameBuffer+addr, nread);
	return nread;
}

static int fb_write(struct dev *dp, uint32_t addr, uint8_t *buf, size_t buf_size)
{
	struct fb_dev *dev = (struct fb_dev *)dp;

	if(addr > dev->FrameBufferSize)
		return -1;
	if(buf_size == 0)
		return 0;
	
	int nwrite = min(buf_size, dev->FrameBufferSize-addr);
	memcpy(dev->FrameBuffer+addr, buf, nwrite);
	return nwrite;
}

static int fb_poll(struct dev *dp, short events)
{
	return POLLIN|POLLOUT;
}

static int fb_ioctl(struct dev *dp, uint32_t cmd, void *arg)
{
	return -1;
}
