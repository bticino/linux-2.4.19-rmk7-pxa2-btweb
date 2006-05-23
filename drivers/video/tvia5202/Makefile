# Makefile for TVIA CyberPro 5202 framebuffer driver
# 
# 

O_TARGET := tviafb.o

export-objs    :=  tviafb_core.o

obj-$(CONFIG_FB_CP5202)  += tviafb_core.o saa7114.o tvia5202-alpha.o \
	tvia5202-capture.o tvia5202-dvideo.o tvia5202-i2c.o tvia5202-misc.o \
	tvia5202-overlay.o tvia5202-synclock.o igsmode_5202.o igstv_5202.o

include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s
