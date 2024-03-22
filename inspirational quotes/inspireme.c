#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>

MODULE_LICENSE("GPL");

//int value to keep track of current quote
static int quote_num;
// list of inspirational quotes
    static const char *quotes[] = {
	"\"Quote 1\" - No matter what people tell you, words and ideas can change the world. - Robin Williams\n",
	"\"Quote 2\" - The best and most beautiful things in the world cannot be seen or even touched - they must be felt with the heart. - Helen Keller\n",
	"\"Quote 3\" - The best preparation for tomorrow is doing your best today. - H. Jackson Brown, Jr.\n",
	"\"Quote 4\" - Keep your face always toward the sunshine - and shadows will fall behind you. - Walt Whitman\n",
	"\"Quote 5\" - Believe you can and you're halfway there. - Theodore Roosevelt\n",
	"\"Quote 6\" - I can't change the direction of the wind, but I can adjust my sails to always reach my destination. - Jimmy Dean\n",
	"\"Quote 7\" - Try to be like the turtle - at ease in your own shell. - Bill Copeland\n",
	"\"Quote 8\" - It is never too late to be what you might have been. - George Eliot\n",
	"\"Quote 9\" - We can't help everyone, but everyone can help someone. - Ronald Reagan\n",
	"\"Quote 10\" - There is nothing impossible to him who will try. - Alexander the Great\n",
	"\"Quote 11\" - Don't judge each day by the harvest you reap but by the seeds that you plant. - Robert Louis Stevenson\n",
	"\"Quote 12\" - Change your thoughts and you change your world. - Norman Vincent Peale\n",
	"\"Quote 13\" - Shoot for the moon and if you miss you will still be among the stars. - Les Brown\n",
	"\"Quote 14\" - The best way out is always through. - Robert Frost\n",
	"\"Quote 15\" - Out of difficulties grow miracles. - Jean de la Bruyere\n",
	"\"Quote 16\" - When you have a dream, you've got to grab it and never let go. - Carol Burnett\n",
	"\"Quote 17\" - If we did all the things we are capable of, we would literally astound ourselves. - Thomas A. Edison\n",
	"\"Quote 18\" - Happiness resides not in possessions, and not in gold, happiness dwells in the soul. - Democritus\n",
	"\"Quote 19\" - God always gives His best to those who leave the choice with him. - Jim Elliot\n",
	"\"Quote 20\" - A champion is someone who gets up when he can't. - Jack Dempsey\n"
    };

// open function, prints to kernel when opened and returns 0
static int my_open(struct inode *inode, struct file *file) {
    printk(KERN_ALERT "Opening inspireme device\n");
    return 0;
}

// release function, prints to kernel when released and increments quote number
static int my_release(struct inode *inode, struct file *file) {
    quote_num++;
    printk(KERN_ALERT "Closing inspireme device\n");
    return 0;
}

// read function which uses copy to user function to print quote to userspace
static ssize_t my_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset) {
    ssize_t length = strlen(quotes[quote_num]);
    if (*offset >= length)
	return 0;
    if (size > length - *offset)
	size = length - *offset;
    if (copy_to_user(user_buffer, quotes[quote_num], size))
	return -EFAULT;
    *offset = *offset + size;
    return size;
}

// write function which returns error when attempting to write, since the file only has read permissions
static ssize_t my_write(struct file *file, const char __user *user_buffer, size_t size, loff_t *offset) {
    printk(KERN_ALERT "Cannot write to inspireme device\n");
    return -EPERM;
}

// object containing all file operations above
const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write
};

// struct for miscdevice that contains name, file_ops and the permissions the device will be assigned
struct miscdevice my_misc = {
    .name = "inspireme",
    .fops = &my_fops,
    .mode = 0444 //read-only permissions
};

// initialization function which uses the misc_register function to register and create the device "inspireme" and returns message on failure
// sudo insmod ./inspireme.ko
static int __init quote_init(void) {
    int ret;
    ret = misc_register(&my_misc);
    if (ret < 0) {
        printk(KERN_ALERT "misc registration failed\n");
        return ret;
    }
    return ret;
}

// exit function which deregisters device (sudo rmmod inspireme)
static void __exit quote_exit(void) {
    misc_deregister(&my_misc);
    printk(KERN_ALERT "Exiting inspireme module\n");
}

module_init(quote_init);
module_exit(quote_exit);
