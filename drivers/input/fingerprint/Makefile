#
# Makefile for all kinds of Fingerprint
#

obj-$(CONFIG_GOODIX_FINGERPRINT) += goodix/
obj-$(CONFIG_FPC_FINGERPRINT) += fpc/

###prize
subdir-ccflags-y += -I$(srctree)/drivers/spi/mediatek/$(subst ",,$(CONFIG_MTK_PLATFORM))/
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/hardware_info/

# begin, prize-lifenfen-20181212, modify for compile error
# obj-y += $(subst ",,$(CONFIG_MTK_PLATFORM))/

ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6739"))
obj-y += mt6739/
endif

# end, prize-lifenfen-20181212, modify for compile error

# begin, prize-lifenfen-20190104, add for ata sunwave fp
obj-$(CONFIG_SUNWAVE_FINGERPRINT) += sunwave_driver_all_in_one/
# end, prize-lifenfen-20190104, add for ata sunwave fp

# begin, prize-pengguangyi-20190119, add for goodix gf5118m fp
subdir-ccflags-y += -I$(srctree)/drivers/spi/mediatek/mt6763/
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/mt6763/include/
obj-$(CONFIG_GOODIX_GF5118M_FINGERPRINT) += goodix-gf5118m/
# end, prize-pengguangyi-20190119, add for goodix gf5118m fp
#prize-add finger-pengzhipeng-20190419-start
obj-$(CONFIG_FT9362_FINGERPRINT) += focaltech/
obj-$(CONFIG_FT9362_FINGERPRINT_TEE) += focaltech_tee/
#prize-add finger-pengzhipeng-20190419-end