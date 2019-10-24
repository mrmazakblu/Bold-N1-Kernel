/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": %s: " fmt, __func__

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/pinctrl/consumer.h>

#include "flashlight-core.h"
#include "flashlight-dt.h"

/* define device tree */
/* TODO: modify temp device tree name */
#ifndef AW_GPIO_DTNAME
#define AW_GPIO_DTNAME "mediatek,flashlights_aw_gpio"
#endif

/* TODO: define driver name */
#define AW_NAME "flashlights-aw-gpio"

/* define registers */
/* TODO: define register */

/* define mutex and work queue */
static DEFINE_MUTEX(aw_mutex);
static struct work_struct aw_work;

typedef enum {
#if !defined(CONFIG_MTK_FLASHLIGHT_WNM4153)
	HWEN_PIN = 0,
#endif
	FLASH_PIN,
}DEV_PIN;
/* define pinctrl */
/* TODO: define pinctrl */
#define AW_PINCTRL_PIN_FLASH 0
#define AW_PINCTRL_PINSTATE_LOW 0
#define AW_PINCTRL_PINSTATE_HIGH 1
#define AW_PINCTRL_STATE_FLASH_HIGH "flash_high"
#define AW_PINCTRL_STATE_FLASH_LOW  "flash_low"
static struct pinctrl *aw_pinctrl;
#if !defined(CONFIG_MTK_FLASHLIGHT_WNM4153)
static struct pinctrl_state *aw_hwen_high;
static struct pinctrl_state *aw_hwen_low;
#endif
static struct pinctrl_state *aw_flash_high;
static struct pinctrl_state *aw_flash_low;

/* define usage count */
static int use_count;

/* platform data */
struct aw_platform_data {
	int channel_num;
	struct flashlight_device_id *dev_id;
};


/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int aw_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	aw_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(aw_pinctrl)) {
		pr_err("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(aw_pinctrl);
		return ret;
	}

	/* TODO: Flashlight FLASH pin initialization */
	aw_flash_high = pinctrl_lookup_state(
			aw_pinctrl, AW_PINCTRL_STATE_FLASH_HIGH);
	if (IS_ERR(aw_flash_high)) {
		pr_err("Failed to init (%s)\n", AW_PINCTRL_STATE_FLASH_HIGH);
		ret = PTR_ERR(aw_flash_high);
	}
	aw_flash_low = pinctrl_lookup_state(
			aw_pinctrl, AW_PINCTRL_STATE_FLASH_LOW);
	if (IS_ERR(aw_flash_low)) {
		pr_err("Failed to init (%s)\n", AW_PINCTRL_STATE_FLASH_LOW);
		ret = PTR_ERR(aw_flash_low);
	}

	return ret;
}

static int aw_pinctrl_set(int pin, int state)
{
	int ret = 0;

	if (IS_ERR(aw_pinctrl)) {
		pr_err("pinctrl is not available\n");
		return -1;
	}

	switch (pin) {
	case HWEN_PIN:
		if (state == AW_PINCTRL_PINSTATE_LOW && !IS_ERR(aw_hwen_low))
			pinctrl_select_state(aw_pinctrl, aw_hwen_low);
		else if (state == AW_PINCTRL_PINSTATE_HIGH && !IS_ERR(aw_hwen_high))
			pinctrl_select_state(aw_pinctrl, aw_hwen_high);
		else
			pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	case FLASH_PIN:
		if (state == AW_PINCTRL_PINSTATE_LOW && !IS_ERR(aw_flash_low))
			pinctrl_select_state(aw_pinctrl, aw_flash_low);
		else if (state == AW_PINCTRL_PINSTATE_HIGH && !IS_ERR(aw_flash_high))
			pinctrl_select_state(aw_pinctrl, aw_flash_high);
		else
			pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	#if 0
	case AW_PINCTRL_PIN_FLASH:
		if (state == AW_PINCTRL_PINSTATE_LOW &&
				!IS_ERR(aw_flash_low))
			pinctrl_select_state(aw_pinctrl, aw_flash_low);
		else if (state == AW_PINCTRL_PINSTATE_HIGH &&
				!IS_ERR(aw_flash_high))
			pinctrl_select_state(aw_pinctrl, aw_flash_high);
		else
			pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	#endif
	default:
		pr_err("set err, pin(%d) state(%d)\n", pin, state);
		break;
	}
	pr_debug("pin(%d) state(%d)\n", pin, state);

	return ret;
}


/******************************************************************************
 * aw operations
 *****************************************************************************/
/* flashlight enable function */
static int aw_enable(void)
{
	//int pin = 0, state = 0;

	/* TODO: wrap enable function */
        return aw_pinctrl_set(FLASH_PIN, 1);  // return 0;
	//return aw_pinctrl_set(pin, state);
}

/* flashlight disable function */
static int aw_disable(void)
{
	//int pin = 0, state = 0;

	/* TODO: wrap disable function */

	return aw_pinctrl_set(FLASH_PIN, 0);
	//return aw_pinctrl_set(pin, state);
}

/* set flashlight level */
static int aw_set_level(int level)
{
	int state = 0;

	/* TODO: wrap set level function */
	printk("flashlight duty = %d\n",level);
	if (level == 0){
		state = 0;
	}else{
		state = 1;
	}
	return aw_pinctrl_set(FLASH_PIN, state);
	//return aw_pinctrl_set(pin, state);
}

/* flashlight init */
static int aw_init(void)
{
	//int pin = 0, state = 0;

	/* TODO: wrap init function */

	return aw_pinctrl_set(FLASH_PIN, 0);
	//return aw_pinctrl_set(pin, state);
}

/* flashlight uninit */
static int aw_uninit(void)
{
	//int pin = 0, state = 0;

	/* TODO: wrap uninit function */

	return aw_pinctrl_set(FLASH_PIN, 0);
	//return aw_pinctrl_set(pin, state);
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer aw_timer;
static unsigned int aw_timeout_ms;

static void aw_work_disable(struct work_struct *data)
{
	pr_debug("work queue callback\n");
	aw_disable();
}

static enum hrtimer_restart aw_timer_func(struct hrtimer *timer)
{
	schedule_work(&aw_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int aw_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;
	unsigned int s;
	unsigned int ns;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		pr_debug("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		aw_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_debug("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		aw_set_level(fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		pr_debug("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (aw_timeout_ms) {
				s = aw_timeout_ms / 1000;
				ns = aw_timeout_ms % 1000 * 1000000;
				ktime = ktime_set(s, ns);
				hrtimer_start(&aw_timer, ktime,
						HRTIMER_MODE_REL);
			}
			aw_enable();
		} else {
			aw_disable();
			hrtimer_cancel(&aw_timer);
		}
		break;
	default:
		pr_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int aw_open(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int aw_release(void)
{
	/* Move to set driver for saving power */
	return 0;
}

static int aw_set_driver(int set)
{
	int ret = 0;

	/* set chip and usage count */
	mutex_lock(&aw_mutex);
	if (set) {
		if (!use_count)
			ret = aw_init();
		use_count++;
		pr_debug("Set driver: %d\n", use_count);
	} else {
		use_count--;
		if (!use_count)
			ret = aw_uninit();
		if (use_count < 0)
			use_count = 0;
		pr_debug("Unset driver: %d\n", use_count);
	}
	mutex_unlock(&aw_mutex);

	return ret;
}

static ssize_t aw_strobe_store(struct flashlight_arg arg)
{
	aw_set_driver(1);
	aw_set_level(arg.level);
	aw_timeout_ms = 0;
	aw_enable();
	msleep(arg.dur);
	aw_disable();
	aw_set_driver(0);

	return 0;
}

static struct flashlight_operations aw_ops = {
	aw_open,
	aw_release,
	aw_ioctl,
	aw_strobe_store,
	aw_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int aw_chip_init(void)
{
	/* NOTE: Chip initialication move to "set driver" for power saving.
	 * aw_init();
	 */

	return 0;
}

static int aw_parse_dt(struct device *dev,
		struct aw_platform_data *pdata)
{
	struct device_node *np, *cnp;
	u32 decouple = 0;
	int i = 0;

	if (!dev || !dev->of_node || !pdata)
		return -ENODEV;

	np = dev->of_node;

	pdata->channel_num = of_get_child_count(np);
	if (!pdata->channel_num) {
		pr_info("Parse no dt, node.\n");
		return 0;
	}
	pr_info("Channel number(%d).\n", pdata->channel_num);

	if (of_property_read_u32(np, "decouple", &decouple))
		pr_info("Parse no dt, decouple.\n");

	pdata->dev_id = devm_kzalloc(dev,
			pdata->channel_num *
			sizeof(struct flashlight_device_id),
			GFP_KERNEL);
	if (!pdata->dev_id)
		return -ENOMEM;

	for_each_child_of_node(np, cnp) {
		if (of_property_read_u32(cnp, "type", &pdata->dev_id[i].type))
			goto err_node_put;
		if (of_property_read_u32(cnp, "ct", &pdata->dev_id[i].ct))
			goto err_node_put;
		if (of_property_read_u32(cnp, "part", &pdata->dev_id[i].part))
			goto err_node_put;
		snprintf(pdata->dev_id[i].name, FLASHLIGHT_NAME_SIZE,
				AW_NAME);
		pdata->dev_id[i].channel = i;
		pdata->dev_id[i].decouple = decouple;

		pr_info("Parse dt (type,ct,part,name,channel,decouple)=(%d,%d,%d,%s,%d,%d).\n",
				pdata->dev_id[i].type, pdata->dev_id[i].ct,
				pdata->dev_id[i].part, pdata->dev_id[i].name,
				pdata->dev_id[i].channel,
				pdata->dev_id[i].decouple);
		i++;
	}

	return 0;

err_node_put:
	of_node_put(cnp);
	return -EINVAL;
}

static int aw_probe(struct platform_device *pdev)
{
	struct aw_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int err;
	int i;

	pr_debug("Probe start.\n");

	/* init pinctrl */
	if (aw_pinctrl_init(pdev)) {
		pr_debug("Failed to init pinctrl.\n");
		err = -EFAULT;
		goto err;
	}

	/* init platform data */
	if (!pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto err;
		}
		pdev->dev.platform_data = pdata;
		err = aw_parse_dt(&pdev->dev, pdata);
		if (err)
			goto err;
	}

	/* init work queue */
	INIT_WORK(&aw_work, aw_work_disable);

	/* init timer */
	hrtimer_init(&aw_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	aw_timer.function = aw_timer_func;
	aw_timeout_ms = 100;

	/* init chip hw */
	aw_chip_init();

	/* clear usage count */
	use_count = 0;

	/* register flashlight device */
	if (pdata->channel_num) {
		for (i = 0; i < pdata->channel_num; i++)
			if (flashlight_dev_register_by_device_id(
						&pdata->dev_id[i],
						&aw_ops)) {
				err = -EFAULT;
				goto err;
			}
	} else {
		if (flashlight_dev_register(AW_NAME, &aw_ops)) {
			err = -EFAULT;
			goto err;
		}
	}

	pr_debug("Probe done.\n");

	return 0;
err:
	return err;
}

static int aw_remove(struct platform_device *pdev)
{
	struct aw_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int i;

	pr_debug("Remove start.\n");

	pdev->dev.platform_data = NULL;

	/* unregister flashlight device */
	if (pdata && pdata->channel_num)
		for (i = 0; i < pdata->channel_num; i++)
			flashlight_dev_unregister_by_device_id(
					&pdata->dev_id[i]);
	else
		flashlight_dev_unregister(AW_NAME);

	/* flush work queue */
	flush_work(&aw_work);

	pr_debug("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id aw_gpio_of_match[] = {
	{.compatible = AW_GPIO_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, aw_gpio_of_match);
#else
static struct platform_device aw_gpio_platform_device[] = {
	{
		.name = AW_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, aw_gpio_platform_device);
#endif

static struct platform_driver aw_platform_driver = {
	.probe = aw_probe,
	.remove = aw_remove,
	.driver = {
		.name = AW_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = aw_gpio_of_match,
#endif
	},
};

static int __init flashlight_aw_init(void)
{
	int ret;

	pr_debug("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&aw_gpio_platform_device);
	if (ret) {
		pr_err("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&aw_platform_driver);
	if (ret) {
		pr_err("Failed to register platform driver\n");
		return ret;
	}

	pr_debug("Init done.\n");

	return 0;
}

static void __exit flashlight_aw_exit(void)
{
	pr_debug("Exit start.\n");

	platform_driver_unregister(&aw_platform_driver);

	pr_debug("Exit done.\n");
}

module_init(flashlight_aw_init);
module_exit(flashlight_aw_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight AW GPIO Driver");

