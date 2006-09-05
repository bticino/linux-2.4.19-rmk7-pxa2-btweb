#
# Makefile for USB peripheral controller and gadget drivers
# for kbuild 2.4
#

# for static linking
O_TARGET	:= built-in.o

list-multi			:= g_zero.o g_ether.o gadgetfs.o g_file_storage.o g_serial.o

obj-$(CONFIG_USB_NET2280)	+= net2280.o
obj-$(CONFIG_USB_PXA2XX)	+= pxa2xx_udc.o
obj-$(CONFIG_USB_GOKU)		+= goku_udc.o
obj-$(CONFIG_USB_SUPERH)	+= superh_udc.o

# only one of these may be statically linked ...
controller-$(CONFIG_USB_NET2280)	+= net2280.o
controller-$(CONFIG_USB_PXA2XX)		+= pxa2xx_udc.o
controller-$(CONFIG_USB_GOKU)		+= goku_udc.o
controller-$(CONFIG_USB_SUPERH)		+= superh_udc.o

# ... and only one of these, too; kbuild/kconfig don't help though.
g_zero-objs			:= zero.o usbstring.o config.o epautoconf.o
obj-$(CONFIG_USB_ZERO)		+= g_zero.o

g_ether-objs			:= ether.o usbstring.o
#obj-$(CONFIG_USB_ETH)		+= g_ether.o

gadgetfs-objs			:= inode.o usbstring.o
#obj-$(CONFIG_USB_GADGETFS)	+= gadgetfs.o

g_file_storage-objs		:= file_storage.o usbstring.o
#obj-$(CONFIG_USB_FILE_STORAGE)	+= g_file_storage.o

g_serial-objs			:= gserial.o usbstring.o config.o epautoconf.o
obj-$(CONFIG_USB_G_SERIAL)	+= g_serial.o

export-objs :=			$(controller-y) $(controller-m)

include $(TOPDIR)/Rules.make

g_zero.o: $(g_zero-objs)
	$(LD) -r -o $@ $(g_zero-objs)
g_ether.o: $(g_ether-objs)
	$(LD) -r -o $@ $(g_ether-objs)
gadgetfs.o: $(gadgetfs-objs)
	$(LD) -r -o $@ $(gadgetfs-objs)
g_file_storage.o: $(g_file_storage-objs)
	$(LD) -r -o $@ $(g_file_storage-objs)
g_serial.o: $(g_serial-objs)
	$(LD) -r -o $@ $(g_serial-objs)
