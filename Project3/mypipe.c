#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

//DEBUG switch
#define pipe_DEBUG 1

#define MODULE_NAME "mypipe"
//buffer memory size
#define BUFFER_SIZE (1 << 28)
//major device number
#define MAJOR_NUM 0
//name of memory mapping
#define CHARM "charmem"
//name of device
#define dev_name "pipe_test"
//port
static struct dev
{
    struct cdev cdevs;
    struct semaphore sem;
    wait_queue_head_t pip_q; //wait_queue
    int t_flag; //flag of block
    char t_buffer[BUFFER_SIZE + 1];
    char *t_read, *t_write, *t_end;
} pipe;
int major = MAJOR_NUM;
static struct class *pipe_class;
static int pipe_open(struct inode *inode, struct file *flip);
static ssize_t pipe_read(struct file *, char *, size_t,
                          loff_t *);
static ssize_t pipe_write(struct file *, const char *,
                           size_t, loff_t *);
static int pipe_release(struct inode *inode, struct file *filp);
//interface of pipe operations
struct file_operations pipe_fops =
    {
        .open = pipe_open,
        .write = pipe_write,
        .read = pipe_read,
        .release = pipe_release,
};

static int pipe_init(void)
{
    int ret = 0, err = 0;
    dev_t dev = MKDEV(major, 0);
#ifdef pipe_DEBUG
    printk("DEV pipe init\n");
#endif
    if (major)
    {
        ret = register_chrdev_region(dev, 1, CHARM);
    }
    else
    {
        //device register
        ret = alloc_chrdev_region(&dev, 0, 1, CHARM);
        // major = MAJOR(dev_num);
    }
    //alloc failed
    if (ret < 0)
    {
#ifdef pipe_DEBUG
        printk(KERN_ERR ": DEV: allocate dev num failed, errno%d\n", ret);
#endif
        return ret;
    }
#ifdef pipe_DEBUG
    printk(KERN_INFO "allocated major device number: %d\n", MAJOR(dev));
#endif
    if (IS_ERR((void *)(unsigned long long)dev))
    {
        ret = PTR_ERR((void *)(unsigned long long)dev);
#ifdef pipe_DEBUG
        printk(KERN_ERR "device_create() failed\n");
#endif
    }
    pipe.cdevs.owner = THIS_MODULE;
    // Associate cdev with fop.
    cdev_init(&pipe.cdevs, &pipe_fops);
    // Add device to cdev_map table.
    err = cdev_add(&pipe.cdevs, dev, 1);
    if (err)
    {
#ifdef pipe_DEBUG
        printk(KERN_ERR "Add cdev_t failed Erro %d\n", err);
#endif
    }
    else
    {
#ifdef pipe_DEBUG
        printk("pipe register Success!\n");
#endif
        sema_init(&pipe.sem, 1); //init signal
        init_waitqueue_head(&pipe.pip_q); //init wait queue
        pipe.t_read = pipe.t_buffer; //read buffer
        pipe.t_write = pipe.t_buffer; //write buffer
        pipe.t_end = pipe.t_buffer + BUFFER_SIZE;
        pipe.t_flag = 0; //queue wake up flag
    }
    // Create device class
    pipe_class = class_create(THIS_MODULE, dev_name);
    // Create device node.
    device_create(pipe_class, NULL, dev, NULL, dev_name);
    if (IS_ERR(pipe_class))
    {
        ret = PTR_ERR(pipe_class);
#ifdef pipe_DEBUG
        printk(KERN_ERR "class_create() failed\n");
#endif
    }
    return 0;
}

//open the device
static int pipe_open(struct inode *inode, struct file *flip)
{
    try_module_get(THIS_MODULE); //module count add
#ifdef pipe_DEBUG
    printk("CharDev already opened!\n");
#endif
    return (0);
}

//Op of reading
static ssize_t pipe_read(struct file *filp, char *buf, size_t len,
                          loff_t *ppos)
{
    //device block
    if (wait_event_interruptible(pipe.pip_q, pipe.t_flag != 0))
    {
        return -ERESTARTSYS;
    }
    if (down_interruptible(&pipe.sem))
    {
        return -ERESTARTSYS;
    }
    pipe.t_flag = 0;

#ifdef pipe_DEBUG
    printk("Start to dev read!\n");
    printk("Read buffer is %c\n", *pipe.t_read);
#endif
    if (pipe.t_read < pipe.t_write)
        len = min(len, (size_t)(pipe.t_end - pipe.t_read));
#ifdef pipe_DEBUG
    printk("Read data length is %lu\n", len);
#endif
    //copy data to the user space
    if (copy_to_user(buf, pipe.t_read, len))
    {
#ifdef pipe_DEBUG
        printk(KERN_ALERT "Read buffer copy failed!\n");
#endif
        up(&pipe.sem);
        return -EFAULT;
    }
    //copy success
#ifdef pipe_DEBUG
    printk("Read buffer is %s\n", pipe.t_buffer);
#endif
    pipe.t_read = pipe.t_read + len;
    if (pipe.t_read == pipe.t_end)
        pipe.t_read = pipe.t_buffer; //cover overflow
    up(&pipe.sem);
    return len;
}

//Op of writing
static ssize_t pipe_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
    if (down_interruptible(&pipe.sem))
    {
        return -ERESTARTSYS;
    }
    if (pipe.t_read <= pipe.t_write)
        len = min(len, (size_t)(pipe.t_end - pipe.t_write));
    else
        len = min(len, (size_t)(pipe.t_read - pipe.t_write - 1));
#ifdef pipe_DEBUG
    printk("Write data len is %lu\n", len);
#endif
    if (copy_from_user(pipe.t_write, buf, len)) //Copy user data to the device
    {
        up(&pipe.sem); //
        return -EFAULT;
    }
#ifdef pipe_DEBUG
    printk("Write buffer is %s\n", pipe.t_buffer);
    printk("Buffer len is %lu\n", strlen(pipe.t_buffer));
#endif
    pipe.t_write = pipe.t_write + len;
    if (pipe.t_write == pipe.t_end)
        pipe.t_write = pipe.t_buffer; //cover overflow
    up(&pipe.sem); //Semaphore switch
    pipe.t_flag = 1; //flag for reading(1), start Op of reading
    wake_up_interruptible(&pipe.pip_q);
    return len;
}

static int pipe_release(struct inode *inode, struct file *filp)
{
    module_put(THIS_MODULE);
#ifdef pipe_DEBUG
    printk("Release dev pipe_class!\n");
#endif
    return (0);
}

//DRV exit
//port name clear
//memory clear
static void pipe_exit(void)
{
#ifdef pipe_DEBUG
    printk("DEV pipe start to clean up\n");
#endif
    device_destroy(pipe_class, MKDEV(major, 0));
    cdev_del(&pipe.cdevs);
    //class destroy
    class_destroy(pipe_class);
    //device unregistered
    unregister_chrdev_region(MKDEV(major, 0), 1);
#ifdef pipe_DEBUG
    printk(KERN_WARNING "pipe drv exit finished!\n");
#endif
}
//info about the module
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zmt1999");

//discription for the module
MODULE_DESCRIPTION("Write And Read pipe for test");
module_init(pipe_init);
module_exit(pipe_exit);
