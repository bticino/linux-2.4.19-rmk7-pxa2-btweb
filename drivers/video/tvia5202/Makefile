# Makefile for TVIA CyberPro 5202 framebuffer driver
# 
# 

#O_TARGET := tviafb.o
#export-objs    :=  tviafb.o

tviafb-objs	:= tviafb_core.o saa7114.o tvia5202-alpha.o \
	tvia5202-capture.o tvia5202-dvideo.o tvia5202-i2c.o tvia5202-misc.o \
	tvia5202-overlay.o tvia5202-synclock.o igsmode_5202.o igstv_5202.o
	

obj-$(CONFIG_FB_CP5202_BT)  += tviafb.o

tviafb.o: $(tviafb-objs)
			$(LD) -r -o $@ $(tviafb-objs)

include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s



#sr_mod-objs	:= sr.o sr_ioctl.o sr_vendor.o
#obj-$(CONFIG_BLK_DEV_SR)	+= sr_mod.o
#sr_mod.o: $(sr_mod-objs)
#			$(LD) -r -o $@ $(sr_mod-objs)
