#ifndef CHARDEV_H 
#define CHARDEV_H 
 
#include <linux/ioctl.h> 

struct vm_area_info {
    unsigned long vm_start;
    unsigned long vm_end;
    unsigned long flags;
    char* name;
    struct vm_area_info* next;
};

struct string_dto {
    char* string;
    unsigned int length;
};

struct pci_dev_info {
    unsigned int device_id;
    unsigned int vendor_id;
    unsigned int class;
    char* device_name;
};

struct vm_message {
    struct vm_area_info vai;
    int area_num;
};

struct pci_dev_request {
    unsigned int device_id;
    unsigned int vendor_id;
};

struct pci_dev_message {
    struct pci_dev_info pci_dev;
    int status;
};

#define DRIVER_NAME 1
#define PATH_NAME 2

#define WR_PID _IOW('a', 1, int)

#define RD_NEXT_VM_AREA _IOR('a', 9, struct vm_message*)

#define WR_PCI_DEV_PARAMS _IOW('a', 4, struct pci_dev_request*)
#define RD_PCI_DEV _IOR('a', 5, struct pci_dev_message*)

#define WR_STRING_ID _IOW('a', 6, int)
#define RD_STRING_LENGTH _IOR('a', 7, unsigned int*)
#define RD_STRING _IOR('a', 8, char*)


#endif
