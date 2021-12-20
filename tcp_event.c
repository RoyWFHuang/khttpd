#include "tcp_event.h"
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>


static int major = -1;
static struct cdev vpoll_cdev;
static struct class *vpoll_class = NULL;

#if 1


// #define MAX_SZ 4096
typedef struct _tmsg {
    char *msg;
    int len;
} tmsg;

// typedef struct _tring_buf {
//     char *buf[MAX_SZ];
//     int head;
//     int tail;
// } tring_buf;

struct vpoll_data {
    wait_queue_head_t wqh;
    // __poll_t events;
    __poll_t events[MAX_SZ];
    char *buf[MAX_SZ];
    int head, tail;
};
struct vpoll_data *vpoll_data;

static int init_queue(void)
{
    vpoll_data = kmalloc(sizeof(struct vpoll_data), GFP_KERNEL);
    if (!vpoll_data)
        return -ENOMEM;
    memset(vpoll_data->events, 0, sizeof(__poll_t));
    vpoll_data->head = 0;
    vpoll_data->tail = 0;
    init_waitqueue_head(&vpoll_data->wqh);
    // file->private_data = vpoll_data;
    return 0;
}

static int vpoll_open(struct inode *inode, struct file *file)
{
    // struct vpoll_data *vpoll_data =
    //     kmalloc(sizeof(struct vpoll_data), GFP_KERNEL);
    // if (!vpoll_data)
    //     return -ENOMEM;
    // memset(vpoll_data->events, 0, sizeof(__poll_t));
    // vpoll_data->head = 0;
    // vpoll_data->tail = 0;
    // init_waitqueue_head(&vpoll_data->wqh);
    file->private_data = vpoll_data;
    return 0;
}

static int vpoll_release(struct inode *inode, struct file *file)
{
    // struct vpoll_data *vpoll_data = file->private_data;
    // kfree(vpoll_data);
    return 0;
}

static long vpoll_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct vpoll_data *vpoll_data = file->private_data;
    __poll_t events = arg & EPOLLALLMASK;
    long res = 10;
    int idx = 0;
    spin_lock_irq(&vpoll_data->wqh.lock);
    if (0 != vpoll_data->events[vpoll_data->tail]) {
        spin_unlock_irq(&vpoll_data->wqh.lock);
        return -EINVAL;
    }
    idx = vpoll_data->tail;
    vpoll_data->tail = (vpoll_data->tail + 1) & QUEUE_MASK;
    // vpoll_data->tail ++;
    spin_unlock_irq(&vpoll_data->wqh.lock);

    switch (cmd) {
    case VPOLL_IO_ADDEVENTS:
        vpoll_data->events[idx] |= events;
        vpoll_data->buf[idx] = vmalloc(10);
        memcpy(vpoll_data->buf[idx], "eventadd", 8);
        break;
    case VPOLL_IO_DELEVENTS:
        vpoll_data->events[idx] &= ~events;
        vpoll_data->buf[idx] = vmalloc(10);
        memcpy(vpoll_data->buf[idx], "eventdel", 8);
        break;
    default:
        res = -EINVAL;
    }
    if (res >= 0) {
        // res = vpoll_data->events[vpoll_data->tail];
        if (waitqueue_active(&vpoll_data->wqh)) {
            // WWW(&vpoll_data->wqh, vpoll_data->events);
            wake_up_poll(&vpoll_data->wqh, events);
            // wake_up_locked_poll(&vpoll_data->wqh, vpoll_data->eventss);
        }
    }
    // spin_unlock_irq(&vpoll_data->wqh.lock);
    return res;
}

static __poll_t vpoll_poll(struct file *file, struct poll_table_struct *wait)
{
    struct vpoll_data *vpoll_data = file->private_data;
    __poll_t events;
    poll_wait(file, &vpoll_data->wqh, wait);
    events = READ_ONCE(vpoll_data->events[vpoll_data->head]);
    // if (0 != events)
    // {
    //     // vpoll_data->events[vpoll_data->head] &= ~events;
    //     // kfree(vpoll_data->buf[vpoll_data->head]);
    //     // vpoll_data->head +=1;
    // }
    return events;
}
#else
struct vpoll_data {
    wait_queue_head_t wqh;
    __poll_t events;
};

static int vpoll_open(struct inode *inode, struct file *file)
{
    struct vpoll_data *vpoll_data =
        kmalloc(sizeof(struct vpoll_data), GFP_KERNEL);
    if (!vpoll_data)
        return -ENOMEM;
    vpoll_data->events = 0;
    init_waitqueue_head(&vpoll_data->wqh);
    file->private_data = vpoll_data;
    return 0;
}

static int vpoll_release(struct inode *inode, struct file *file)
{
    struct vpoll_data *vpoll_data = file->private_data;
    kfree(vpoll_data);
    return 0;
}

static long vpoll_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct vpoll_data *vpoll_data = file->private_data;
    __poll_t events = arg & EPOLLALLMASK;
    long res = 10;
    spin_lock_irq(&vpoll_data->wqh.lock);
    switch (cmd) {
    case VPOLL_IO_ADDEVENTS:
        vpoll_data->events |= events;
        break;
    case VPOLL_IO_DELEVENTS:
        vpoll_data->events &= ~events;
        break;
    default:
        res = -EINVAL;
    }
    if (res >= 0) {
        res = vpoll_data->events;
        if (waitqueue_active(&vpoll_data->wqh))
            // WWW(&vpoll_data->wqh, vpoll_data->events);
            // wake_up_poll(&vpoll_data->wqh, vpoll_data->events);
            wake_up_locked_poll(&vpoll_data->wqh, vpoll_data->events);
    }
    spin_unlock_irq(&vpoll_data->wqh.lock);
    return res;
}

static __poll_t vpoll_poll(struct file *file, struct poll_table_struct *wait)
{
    struct vpoll_data *vpoll_data = file->private_data;

    poll_wait(file, &vpoll_data->wqh, wait);
    __poll_t events = READ_ONCE(vpoll_data->events);
    vpoll_data->events &= ~events;
    return events;
}

#endif


static ssize_t vpoll_read(struct file *file,
                          char __user *buf,
                          size_t count,
                          loff_t *ppos)
{
    unsigned int read;
    int ret;
    __poll_t events;

    struct vpoll_data *vpoll_data = file->private_data;

    events = READ_ONCE(vpoll_data->events[vpoll_data->head]);
    read = strlen(vpoll_data->buf[vpoll_data->head]);
    ret = copy_to_user(buf, vpoll_data->buf[vpoll_data->head], read);
    vpoll_data->events[vpoll_data->head] &= ~events;
    vfree(vpoll_data->buf[vpoll_data->head]);
    printk("vpoll_data->head = %d\n", vpoll_data->head);
    vpoll_data->head = (vpoll_data->head + 1) & QUEUE_MASK;

    return ret ? ret : read;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = vpoll_open,
    .release = vpoll_release,
    .unlocked_ioctl = vpoll_ioctl,
    .poll = vpoll_poll,  // for epoll_ctl
    .read = vpoll_read,
};

static char *vpoll_devnode(struct device *dev, umode_t *mode)
{
    if (!mode)
        return NULL;

    *mode = 0666;
    return NULL;
}

int data_vpoll_init(void)
{
    int ret;
    struct device *dev;

    if ((ret = alloc_chrdev_region(&major, 0, 1, NAME)) < 0)
        return ret;
    vpoll_class = class_create(THIS_MODULE, NAME);
    if (IS_ERR(vpoll_class)) {
        ret = PTR_ERR(vpoll_class);
        goto error_unregister_chrdev_region;
    }
    vpoll_class->devnode = vpoll_devnode;
    dev = device_create(vpoll_class, NULL, major, NULL, NAME);
    if (IS_ERR(dev)) {
        ret = PTR_ERR(dev);
        goto error_class_destroy;
    }
    cdev_init(&vpoll_cdev, &fops);
    if ((ret = cdev_add(&vpoll_cdev, major, 1)) < 0)
        goto error_device_destroy;

    printk(KERN_INFO NAME ": loaded\n");
    init_queue();
    return 0;

error_device_destroy:
    device_destroy(vpoll_class, major);
error_class_destroy:
    class_destroy(vpoll_class);
error_unregister_chrdev_region:
    unregister_chrdev_region(major, 1);
    return ret;
}

void data_vpoll_exit(void)
{
    kfree(vpoll_data);
    device_destroy(vpoll_class, major);
    cdev_del(&vpoll_cdev);
    class_destroy(vpoll_class);
    unregister_chrdev_region(major, 1);
    printk(KERN_INFO NAME ": unloaded\n");
}

// module_init(vpoll_init);
// module_exit(vpoll_exit);