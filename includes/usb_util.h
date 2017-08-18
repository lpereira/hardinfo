#ifndef __USB_UTIL_H__
#define __USB_UTIL_H__

/* this is a linked list */
typedef struct usbd {
    int bus, dev;
    int vendor_id, product_id;
    char *vendor;
    char *product;
    int dev_class;
    int dev_subclass;
    char *usb_version;
    int max_curr_ma;

    int speed_mbs; /* TODO: */

    int user_scan; /* not scanned as root */

    struct usbd *next;
} usbd;

usbd *usb_get_device_list();
int usbd_list_count(usbd *);
void usbd_list_free(usbd *);

usbd *usb_get_device(int bus, int dev);
void usbd_free(usbd *);

#endif
