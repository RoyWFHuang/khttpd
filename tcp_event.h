#ifndef KTCP_EVE_H
#define KTCP_EVE_H

#include <linux/poll.h>

#define DEFAULT_NAME "vpoll"
#define VPOLL_IOC_MAGIC '^'
#define VPOLL_IO_ADDEVENTS _IO(VPOLL_IOC_MAGIC, 1)
#define VPOLL_IO_DELEVENTS _IO(VPOLL_IOC_MAGIC, 2)
#define EPOLLALLMASK ((__force __poll_t) 0x0fffffff)


#define MAX_SZ (8192 << 2)
#define QUEUE_MASK (MAX_SZ - 1)
int data_vpoll_init(void);

void data_vpoll_exit(void);

long set_event_and_data(__poll_t event,
                        unsigned char *data,
                        unsigned int data_len);

#endif



