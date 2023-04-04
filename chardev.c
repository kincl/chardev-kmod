/*  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 *
 *  Copyright (C) 2001 by Peter Jay Salzman
 *
 *  08/02/2006 - Updated by Rodrigo Rubira Branco <rodrigo@kernelhacking.com>
 *  2023 - From https://tldp.org/LDP/lkmpg/2.4/html/x579.html and modified by Jason Kincl
 */

/* Kernel Programming */
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>  /* for put_user */
#include <asm/errno.h>

/*  Prototypes - this would normally go in a .h file */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "hello"   /* device name as it appears in /proc/devices */
#define CLASS_NAME "chardev"  /* device class */
#define BUF_LEN 80            /* Max length of the message from the device */

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple char device");
MODULE_VERSION("0.1");

/* Global variables are declared as static, so are global within the file. */

static int majorNumber;                  /* Major number assigned to our device driver */
static struct class*  charClass  = NULL; /* Class struct pointer */
static struct device* charDevice = NULL; /* Device struct pointer */

static int counter = 0;      /* counts the number of opens */
static int Device_Open = 0;  /* Is device open?  Used to prevent multiple
                                        access to the device */
static char msg[BUF_LEN];    /* The msg the device will give when asked */
static char *msg_Ptr;

static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};


/* Functions */

int init_module(void)
{
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber < 0) {
     printk("Registering the character device failed with %d\n", majorNumber);
     return majorNumber;
   }
   printk("chardev: assigned major number %d\n", majorNumber);

   charClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(charClass)){
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk("chardev: Failed to register device class\n");
      return PTR_ERR(charClass);
   }
   printk("chardev: device class registered successfully\n");

   charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(charDevice)){
      class_destroy(charClass);
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk("chardev: Failed to create the device\n");
      return PTR_ERR(charDevice);
   }

   printk("chardev: device created correctly\n");
   printk("chardev: try to cat and echo to /dev/hello\n");

   return 0;
}


void cleanup_module(void)
{
   /* Unregister the device */
   device_destroy(charClass, MKDEV(majorNumber, 0));
   class_unregister(charClass);
   class_destroy(charClass);
   unregister_chrdev(majorNumber, DEVICE_NAME);
}


/* Methods */

/* Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
   if (Device_Open) return -EBUSY;

   Device_Open++;
   sprintf(msg,"I already told you %d times Hello world!\n", counter++);
   msg_Ptr = msg;

   return SUCCESS;
}


/* Called when a process closes the device file */
static int device_release(struct inode *inode, struct file *file)
{
   Device_Open --;     /* We're now ready for our next caller */

   return 0;
}


/* Called when a process, which already opened the dev file, attempts to
   read from it.
*/
static ssize_t device_read(struct file *filp,
   char *buffer,    /* The buffer to fill with data */
   size_t length,   /* The length of the buffer     */
   loff_t *offset)  /* Our offset in the file       */
{
   /* Number of bytes actually written to the buffer */
   int bytes_read = 0;

   /* If we're at the end of the message, return 0 signifying end of file */
   if (*msg_Ptr == 0) return 0;

   /* Actually put the data into the buffer */
   while (length && *msg_Ptr)  {

        /* The buffer is in the user data segment, not the kernel segment;
         * assignment won't work.  We have to use put_user which copies data from
         * the kernel data segment to the user data segment. */
         put_user(*(msg_Ptr++), buffer++);

         length--;
         bytes_read++;
   }

   /* Most read functions return the number of bytes put into the buffer */
   return bytes_read;
}


/*  Called when a process writes to dev file: echo "hi" > /dev/hello */
static ssize_t device_write(struct file *filp,
   const char *buff,
   size_t len,
   loff_t *off)
{
   printk ("<1>Sorry, this operation isn't supported.\n");
   return -EINVAL;
}
