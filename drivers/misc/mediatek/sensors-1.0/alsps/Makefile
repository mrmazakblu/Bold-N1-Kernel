# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include
obj-y   +=  alsps.o
obj-y   +=  alsps_factory.o
obj-y   +=  aal_control.o

obj-$(CONFIG_MTK_ALSPSHUB)      +=  alspshub/
obj-$(CONFIG_MTK_CM36558)	+=  cm36558/
#prize-add stk3x1x-new-pengzhipeng-20180827-start
obj-$(CONFIG_MTK_STK3X1X_NEW)	+=  stk3x1x-new/
#prize-add stk3x1x-new-pengzhipeng-20180827-end


###prize
obj-$(CONFIG_MTK_LTR559)	+=  ltr559/