#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

static struct usb_device_id usbmouse_as_key_id_table[] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
	USB_INTERFACE_PROTOCOL_MOUSE)},
	{},
};

static int usbmouse_as_key_probe(struct usb_interface *intf,
		      const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);

	printk("Found USBMouse.\n");
	printk("bcdUSB version = %x.", dev->descriptor.bcdUSB);
	printk("VID = 0x%x.", dev->descriptor.idVendor);
	printk("PID = 0x%x.", dev->descriptor.idProduct);
	return 0;
}
static void usbmouse_as_key_disconnect(struct usb_interface *intf)
{
	printk("USBMouse disconnect.\n");
}
//1�����䡢����usb_driver
static struct usb_driver usbmouse_as_key_driver = {
	.name = "KeyTypeUSBMouse",
	.probe = usbmouse_as_key_probe,
	.disconnect = usbmouse_as_key_disconnect,
	.id_table = usbmouse_as_key_id_table,
};

static int usbmouse_as_key_init(void)
{
	
	//2��ע��
	usb_register(&usbmouse_as_key_driver);
	return 0;
}

static void usbmouse_as_key_exit(void)
{
	//3��ж��
	usb_deregister(&usbmouse_as_key_driver);
}

module_init(usbmouse_as_key_init);
module_exit(usbmouse_as_key_exit);

MODULE_LICENSE("GPL");
