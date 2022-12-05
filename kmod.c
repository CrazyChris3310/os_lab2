#include <linux/init.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
 
#include <linux/ioctl.h>
#include <linux/pid.h>
#include <linux/pci.h>
 
#include <linux/sched/task_stack.h>
#include <linux/string.h>

#include "kmod_header.h"

#define SUCCESS 0

 
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("ioctl_driver");
MODULE_VERSION("1.0");
 
struct class *dev_class;
struct cdev my_cdev;
dev_t dev = 0;



static int device_open(struct inode *inode, struct file *file) { 
    pr_info("device_open(%p)\n", file); 
    return SUCCESS; 
} 

static int device_release(struct inode *inode, struct file *file) { 
    pr_info("device_release(%p,%p)\n", inode, file); 
    return SUCCESS; 
}

static long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long arg) { 
    static int pid = 0;
    long to_return = SUCCESS;
    static struct vm_area_struct* vma;
    static struct vm_area_struct* vma_next;
    static int is_new = 1;

    static int string_id = 0;

    static struct pci_dev_request pci_dev_req;
    static struct pci_dev* device;

    switch(ioctl_num) {
        case WR_PID:
            struct task_struct* tsk;
            if (copy_from_user(&pid, (int*) arg, sizeof(int))) {
                pr_err("Error while copying pid from user");
                return -EFAULT;
            }
            tsk = pid_task(find_vpid(pid), PIDTYPE_PID);
            vma = tsk->mm->mmap;
            vma_next = NULL;
            is_new = 1;
            pr_info("pid = %d", pid);
            break;
        case RD_NEXT_VM_AREA:
            struct vm_message msge = (struct vm_message){0};

            if (is_new == 1) {
                vma_next = vma;
                is_new = 0;
            } else {
                if (vma_next != NULL) {
                    vma_next = vma_next->vm_next;
                }
            }

            if (vma_next == NULL) {
                msge.vai = (struct vm_area_info){0};
                msge.area_num = -1;
                to_return = -1;
            } else {
                struct vm_area_info vail = (struct vm_area_info){0};
                vail.vm_start = vma_next->vm_start;
                vail.vm_end = vma_next->vm_end;
                vail.flags = vma_next->vm_flags;
                msge.vai = vail;
                vma_next = vma_next->vm_next;
                msge.area_num = 0;
            }
            if (copy_to_user((struct vm_message*) arg, &msge, sizeof(struct vm_message)) ) {
                pr_err("Error while copying vm_area_info to user");
                to_return = -EFAULT;
            }
            break;
        case WR_PCI_DEV_PARAMS:
            if (copy_from_user(&pci_dev_req, (struct pci_dev_request*) arg, sizeof(struct pci_dev_request))) {
                pr_err("Error while copying pci_dev_request from user");
                return -EFAULT;
            }
            pr_info("vendor_id = %d, device_id = %d", pci_dev_req.vendor_id, pci_dev_req.device_id);
            device = pci_get_device(pci_dev_req.vendor_id, pci_dev_req.device_id, NULL);
            pr_info("device = %p", device);
            break;
        case RD_PCI_DEV:
            struct pci_dev_message pci_dev_msg = {0};
            if (device == NULL) {
                pci_dev_msg.status = -1;
                to_return = -1;
            } else {
                struct pci_dev_info pci_device_info = {0};
                pci_device_info.device_id = device->device;
                pci_device_info.vendor_id = device->vendor;
                pci_device_info.class = device->class;
                pci_dev_msg.pci_dev = pci_device_info;
            }
            pr_info("Reading pci_struct");
            if (copy_to_user((struct pci_dev_message*) arg, &pci_dev_msg, sizeof(struct pci_dev_message)) ) {
                pr_err("Error while copying pci_dev_info to user");
                to_return = -EFAULT;
            }
            break;
        case WR_STRING_ID:
            if (copy_from_user(&string_id, (int*) arg, sizeof(int))) {
                pr_err("Error while copying string_id from user");
                return -EFAULT;
            }
            break;
        case RD_STRING_LENGTH:
            unsigned int length;
            switch (string_id) {
                case DRIVER_NAME:
                    pr_info("device: %d", device == NULL);
                    length = strlen(pci_name(device));
                    break;
                case PATH_NAME:
                    if (vma_next == NULL ||  vma_next->vm_file == NULL || vma_next->vm_file->f_path.dentry == NULL
                    || vma_next->vm_file->f_path.dentry->d_iname == NULL) {
                        length = 0;
                    } else {
                        length = strlen(vma_next->vm_file->f_path.dentry->d_iname);
                    }
                    break;
                default:
                    length = 0;
                    break;
                }
            if (copy_to_user((unsigned int*) arg, &length, sizeof(unsigned int)) ) {
                pr_err("Error while copying string length to user");
                to_return = -EFAULT;
            }
            break;
        case RD_STRING:
            const char* string;
            switch (string_id) {
                case DRIVER_NAME:
                    string = pci_name(device);
                    break;
                case PATH_NAME:
                    if (vma_next == NULL ||  vma_next->vm_file == NULL || vma_next->vm_file->f_path.dentry == NULL
                    || vma_next->vm_file->f_path.dentry->d_iname == NULL) {
                        string = "";
                    } else {
                        string = vma_next->vm_file->f_path.dentry->d_iname;
                    }
                    break;
                default:
                    string = "";
                    break;
            }
            pr_info("String: %s\n", string);
            if (copy_to_user((char*) arg, string, strlen(string))) {
                pr_err("Error while copying string to user");
                to_return = -EFAULT;
            }
            break;
        default:
            pr_info("Default\n");
            break;
    }
    return to_return;
} 

static struct file_operations fops = { 
    .owner = THIS_MODULE,
    .unlocked_ioctl = device_ioctl, 
    .open = device_open, 
    .release = device_release,
}; 

 
static int __init ioctl_mod_init(void) {
    int res;
    printk(KERN_INFO "ioctl_mod: module loaded.\n");
    res = alloc_chrdev_region(&dev, 0, 1, "nice_dev");
    if (res < 0) {
        pr_err("Cannot allocate major number\n");
        return -1;
    }
    pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

    cdev_init(&my_cdev, &fops);

    res = cdev_add(&my_cdev, dev, 1);
    if(res < 0){
        pr_err("Cannot add the device to the system\n");
        unregister_chrdev_region(dev,1);
        return -1;
    }

    dev_class = class_create(THIS_MODULE, "/dev/nice_dev"); 
    if (dev_class == NULL) {
        pr_err("Cannot create the struct class\n");
        unregister_chrdev_region(dev,1);
        return -1;
    }

    device_create(dev_class, NULL, dev, NULL, "nice_dev"); 

    pr_info("Device Driver Insert...Done!!!\n");
    return 0;
}
 
/*
** Module exit function
*/
static void __exit ioctl_mod_exit(void) {
    device_destroy(dev_class, dev); 
    class_destroy(dev_class); 
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("Device Driver Remove...Done!!!\n");
}
 
module_init(ioctl_mod_init);
module_exit(ioctl_mod_exit);
