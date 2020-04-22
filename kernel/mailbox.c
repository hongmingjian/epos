#include "cpu.h"
#include "kernel.h"

uint32_t mailbox_read (MAILBOX_CHANNEL channel)
{
    if (channel > CHANNEL_GPU)  
        return 0xF;//Lower 4 bits are nonzero to indicate error

    uint32_t value;
    mailbox_reg_t *mbt = (mailbox_reg_t *)(MMIO_BASE_VA+MAILBOX_REG);
    do {
        do {
            value = mbt->status0;
        } while ((value & MAILBOX_EMPTY) != 0);
        value = mbt->read0;
    } while ((value & 0xF) != channel);
    value &= ~(0xF);//Lower 4 bits are not part of message
    return value;
}

int mailbox_write (MAILBOX_CHANNEL channel, uint32_t message)
{
    if (channel > CHANNEL_GPU)
        return -1;

    uint32_t value;
    mailbox_reg_t *mbt = (mailbox_reg_t *)(MMIO_BASE_VA+MAILBOX_REG);

    message &= ~(0xF); //Lower 4 bits
    message |= channel;// contains the channel
    do {
        value = mbt->status1;
    } while ((value & MAILBOX_FULL) != 0);
    mbt->write1 = message;
    return 0;
}

int mailbox_write_read(MAILBOX_CHANNEL channel, uint32_t message)
{
    if (channel > CHANNEL_GPU)
        return -1;
//#if defined ( RPI2 ) || defined ( RPI3 )
//	clean_data_cache();
//#endif

//	dsb();

//#if defined ( RPI2 ) || defined ( RPI3 )
//	dmb();
//#endif

	mailbox_write(channel, message);
	uint32_t value = mailbox_read(channel);

//#if defined ( RPI2 ) || defined ( RPI3 )
//	dmb();
//	invalidate_data_cache();
//#endif

//	dmb();

	return (value&0xF)?-1:0;
}
