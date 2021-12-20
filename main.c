#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kthread.h>
#include <linux/sched/signal.h>
#include <linux/tcp.h>
#include <linux/version.h>
#include <net/sock.h>

#include "tcp_event.h"
#include "tcp_main.h"

static int __init ktcp_serv_init(void)
{

    int err;
    if (0 != (err = ktcp_init()))
        return err;
    if (0 != (err = data_vpoll_init()))
        return err;
    return 0;
}

static void __exit ktcp_serv_exit(void)
{
    ktcp_exit();
    data_vpoll_exit();
}

module_init(ktcp_serv_init);
module_exit(ktcp_serv_exit);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("in-kernel HTTP daemon");
MODULE_VERSION("0.1");
