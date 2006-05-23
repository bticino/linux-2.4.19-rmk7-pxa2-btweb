#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).

USE_STANDARD_AS_RULE := true

O_TARGET		:= pxa.o

obj-y :=
obj-m :=
obj-n :=
obj-  :=

export-objs := generic.o irq.o dma.o sa1111.o \
               usb_ctl.o usb_recv.o usb_send.o btweb.o

# Common support (must be linked before board specific support)
obj-y += generic.o irq.o dma.o
obj-$(CONFIG_SA1111) += sa1111.o

# Specific board support
obj-$(CONFIG_ARCH_CSB226) += csb226.o
obj-$(CONFIG_ARCH_INNOKOM) += innokom.o
obj-$(CONFIG_ARCH_LUBBOCK) += lubbock.o
obj-$(CONFIG_ARCH_PXA_CERF) += cerf.o
obj-$(CONFIG_ARCH_PXA_IDP) += idp.o
obj-$(CONFIG_ARCH_TRIZEPS2) += trizeps2.o
obj-$(CONFIG_MACH_BTWEB) += btweb.o btweb-sysctl.o

# Support for blinky lights
leds-y := leds.o
leds-$(CONFIG_ARCH_CSB226) += leds-csb226.o
leds-$(CONFIG_ARCH_INNOKOM) += leds-innokom.o
leds-$(CONFIG_ARCH_LUBBOCK) += leds-lubbock.o
leds-$(CONFIG_ARCH_PXA_IDP) += leds-idp.o
leds-$(CONFIG_ARCH_PXA_CERF) += leds-cerf.o

obj-$(CONFIG_LEDS) += $(leds-y)

# PXA USB client support
list-multi += pxausb_core.o
pxausb_core-objs := usb_ctl.o usb_ep0.o usb_recv.o usb_send.o
obj-$(CONFIG_PXA_USB) += pxausb_core.o
obj-$(CONFIG_PXA_USB_NETLINK) += usb-eth.o
obj-$(CONFIG_PXA_USB_CHAR) += usb-char.o

# Misc features
obj-$(CONFIG_PM) += pm.o sleep.o
obj-$(CONFIG_CPU_FREQ) += cpu-pxa.o

include $(TOPDIR)/Rules.make

pxausb_core.o: $(pxausb_core-objs)
	$(LD) -r -o $@ $(pxausb_core-objs)
