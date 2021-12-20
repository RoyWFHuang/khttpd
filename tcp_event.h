#ifndef KTCP_EVE_H
#define KTCP_EVE_H

#define NAME "vpoll"
#define VPOLL_IOC_MAGIC '^'
#define VPOLL_IO_ADDEVENTS _IO(VPOLL_IOC_MAGIC, 1)
#define VPOLL_IO_DELEVENTS _IO(VPOLL_IOC_MAGIC, 2)
#define EPOLLALLMASK ((__force __poll_t) 0x0fffffff)


#define MAX_SZ 16
#define QUEUE_MASK (MAX_SZ - 1)
int data_vpoll_init(void);

void data_vpoll_exit(void);

#endif



