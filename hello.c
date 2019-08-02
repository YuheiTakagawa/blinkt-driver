#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/mutex.h>

#define NODE_NAME "hellodev"

#define GPIODATA 23
#define GPIOCLOCK 24

#define DEVNUM 9
#define MAJORNUM 240

struct cdev cdev[DEVNUM];
dev_t dev = MKDEV(MAJORNUM, 0);

static DEFINE_MUTEX(mtx);

static char state = 0x0;
static int data_size = 0;
static int read_count = 0;

void send_1byte(int pin, char x);
void send_1bit(int pin, char x);
void change_brightness(int pin, char x);

static int hello_open(struct inode *inode, struct file *file){
	int minor = iminor(inode);
	printk("open %d\n", minor);
	return 0;
}

static int hello_release(struct inode *inode, struct file *file){
	printk("release\n");
	return 0;
}

static ssize_t hello_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos){
	int len;
	int minor = iminor(file->f_path.dentry->d_inode);
	char k_buf = 0x0;

	if(read_count){
		return 0;
	}

	if(count > data_size) 
		len = data_size;
	else
		len = count;

	if (mutex_lock_interruptible(&mtx) != 0) {
		return 0;
	}	

	if (minor == 0)
		k_buf = state;
	else
		k_buf = state >> (minor - 1) & 1;

	mutex_unlock(&mtx);

	read_count += copy_to_user(buf, &k_buf, sizeof(k_buf));
	return len;
}

static ssize_t hello_write(struct file *file, const char __user *buf, size_t count, loff_t *f_pos){
	int minor = iminor(file->f_path.dentry->d_inode);
	char k_buf = 0x0;


	if(copy_from_user(&k_buf, buf, count) != 0){
		return 0;
	}

	data_size = count;
	if (data_size == 0){
		return 0;
	}
	
	if (mutex_lock_interruptible(&mtx) != 0) {
		return 0;
	}	

	if (minor == 0){
		state = k_buf;
	}else{
		if(k_buf == 1)
			state = state|(1<<(minor-1));
		else if(k_buf == 0)
			state = state&~(1<<(minor-1));
	}
	change_brightness(GPIODATA, state);

	mutex_unlock(&mtx);
	return count;
}

static struct file_operations hello_fops = {
	.owner = THIS_MODULE,
	.open = hello_open,
	.read = hello_read,
	.write = hello_write,
	.release = hello_release,
};

void send_1bit(int pin, char val){
	gpio_set_value(pin, val);
	gpio_set_value(GPIOCLOCK, 1);
	ndelay(500);
	gpio_set_value(GPIOCLOCK, 0);
	ndelay(500);
}

void send_1byte(int pin, char val){
	int i;
	for(i = 0; i < 8; i++){
		send_1bit(pin, (val>>i)&1);
	}
}


void change_brightness(int pin, char x){
	char brightness = 1 | 0xe0;
	char blue = 0xf0;
	char green = 0xf0;
	char red = 0xf0;
	int i;

	for(i = 0; i < 32; i++){
		send_1bit(GPIODATA, 0);
	}

	for(i = 0; i < 8; i++){
		if((x>>i)&1){
			send_1byte(GPIODATA, brightness);
			send_1byte(GPIODATA, blue);
			send_1byte(GPIODATA, green);
			send_1byte(GPIODATA, red);
		}else{
			send_1byte(GPIODATA, 0x0 | 0xe0);
			send_1byte(GPIODATA, 0x0);
			send_1byte(GPIODATA, 0x0);
			send_1byte(GPIODATA, 0x0);
	
		}
	}

	for(i = 0; i < 32; i++){
		send_1bit(GPIODATA, 1);
	}	
}


static int __init hello_init(void){
	int i;

	printk(KERN_INFO "Hello modules\n");

	register_chrdev_region(dev, DEVNUM, "hello");

	for (i = 0; i < DEVNUM; i++){
		cdev_init(&cdev[i], &hello_fops);
		cdev[i].owner = THIS_MODULE;
		cdev_add(&cdev[i], MKDEV(MAJORNUM, i), 1);
	}

	if (!gpio_is_valid(GPIODATA)){
		printk(KERN_INFO "GPIO invaid %d\n", GPIODATA);
		return -ENODEV;
	}

	if (!gpio_is_valid(GPIOCLOCK)){
		printk(KERN_INFO "GPIO invalid %d\n", GPIOCLOCK);
		return -ENODEV;
	}

	gpio_request(GPIODATA, "sysfs");
	gpio_request(GPIOCLOCK, "sysfs");
	
	gpio_direction_output(GPIODATA, 0);
	gpio_direction_output(GPIOCLOCK, 0);

	return 0;
}

static void __exit hello_exit(void){
	int i;
	for(i = 0; i < DEVNUM; i++){
		cdev_del(&cdev[i]);
	}
	unregister_chrdev_region(dev, DEVNUM);

	change_brightness(GPIODATA, 0x0);
	
	gpio_free(GPIODATA);

	printk(KERN_INFO "Bye World\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Yuhei Takagawa");

