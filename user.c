#include "kmod_header.h"

#include <sys/ioctl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
 
struct vm_area_info* get_area_struct(int fd, int pid) {
	struct vm_area_info* area = NULL;
	struct vm_area_info* current = area;
	int c = 0;
    struct vm_message buffer = {0};
    ioctl(fd, WR_PID, &pid);
    int type = PATH_NAME;
    ioctl(fd, WR_STRING_ID, &type);
	do {
        c = ioctl(fd, RD_NEXT_VM_AREA, (struct vm_message*) &buffer);
		if (c == 0) {
			if (current == NULL) {
				current = malloc(sizeof(struct vm_area_info));
				area = current;
			} else {
				current->next = malloc(sizeof(struct vm_area_info));
				current = current->next;
			}
            *current = buffer.vai;
            unsigned int length;
            ioctl(fd, RD_STRING_LENGTH, &length);
            current->name = malloc(length + 1);
            ioctl(fd, RD_STRING, current->name);
            current->name[length] = '\0';
		}
	} while (c == 0);
	return area;
}

void free_area_struct(struct vm_area_info* area) {
    struct vm_area_info* current = area;
    struct vm_area_info* next = NULL;
    while (current != NULL) {
        next = current->next;
        free(current->name);
        free(current);
        current = next;
    }
}


struct pci_dev_info* get_pci_dev(int fd, int vendor_id, int device_id) {
    struct pci_dev_request pci_dev_req = { .vendor_id=vendor_id, .device_id=device_id };
    ioctl(fd, WR_PCI_DEV_PARAMS, &pci_dev_req);

    int type = DRIVER_NAME;
    ioctl(fd, WR_STRING_ID, &type);

    struct pci_dev_message pci_dev_msg = {0};
    ioctl(fd, RD_PCI_DEV, &pci_dev_msg);
    if (pci_dev_msg.status == -1) {
        return NULL;
    }
    struct pci_dev_info* pci_dev_info = malloc(sizeof(struct pci_dev_info));
    *pci_dev_info = pci_dev_msg.pci_dev;

    unsigned int length;
    ioctl(fd, RD_STRING_LENGTH, &length);
    pci_dev_info->device_name = malloc(length + 1);
    ioctl(fd, RD_STRING, pci_dev_info->device_name);
    pci_dev_info->device_name[length] = '\0';

    return pci_dev_info;
}

void free_pci_dev(struct pci_dev_info* pci_dev_info) {
    free(pci_dev_info->device_name);
    free(pci_dev_info);
}
 
int main(int argc, char *argv[]) {

    if (argc < 4) {
        printf("Usage: <pid> <vendor> <device>\n");
        return 0;
    }

    int fd;
    int pid = atoi(argv[1]);
    printf("Opening Driver\n");
    fd = open("/dev/nice_dev", O_RDWR);
    if (fd < 0) { 
        printf("Can't open device file: %s, error:%d\n", "/dev/nice_dev", fd); 
        exit(EXIT_FAILURE); 
    } 
    
    struct vm_area_info* res = get_area_struct(fd, pid);
    struct vm_area_info* iter = res;

    printf("vm_area_struct:\n");
    printf("%-20s   %-20s    %-15s   %-50s\n", "vm_start", "vm_end", "flags", "path");
    while (res != NULL) {
        printf("%-20lx   %-20lx    %-15lx   %-50s\n", res->vm_start, res->vm_end, res->flags, res->name);
        res = res->next;
    }
    free_area_struct(iter);

    unsigned int vendor_id = strtol(argv[2], NULL, 16);
    unsigned int device_id = strtol(argv[3], NULL, 16);
    struct pci_dev_info* device_info = get_pci_dev(fd, vendor_id, device_id);

    if (device_info == NULL) {
        printf("There are no devices with vendor_id=%d and device_id=%d\n", vendor_id, device_id);
    } else {
        printf("\npci_dev:\n");
        printf("vendor: %x\n", device_info->vendor_id);
        printf("device: %x\n", device_info->device_id);
        printf("class: %x\n", device_info->class);
        printf("device_name: %s\n", device_info->device_name);
        free_pci_dev(device_info);
    }

    close(fd);
}

