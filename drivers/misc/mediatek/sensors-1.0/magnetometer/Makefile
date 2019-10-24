# In case the platform does NOT support this type of sensors
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/hwmon/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/accelerometer/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/magnetometer/inc
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/include
obj-y   +=  mag.o
obj-y   +=  mag_factory.o

obj-$(CONFIG_MTK_MAGHUB)        +=  maghub/
#prize added by chenjiaxi, afx133 drivers, 20190111-start
obj-$(CONFIG_MTK_AFX133)	+=  afx133/
#prize added by chenjiaxi, afx133 drivers, 20190111-end
obj-$(CONFIG_MTK_AKM09911)         +=  akm09911/
obj-$(CONFIG_MTK_ST480)         +=  st480/
obj-$(CONFIG_MTK_AKM09918)      +=  akm09918/
