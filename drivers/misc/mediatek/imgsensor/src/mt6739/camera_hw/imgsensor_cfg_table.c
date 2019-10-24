/*
 * Copyright (C) 2017 MediaTek Inc.
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

#include "kd_imgsensor.h"

#include "regulator/regulator.h"
#include "gpio/gpio.h"
#include "mclk/mclk.h"



#include "imgsensor_cfg_table.h"

enum IMGSENSOR_RETURN (*hw_open[IMGSENSOR_HW_ID_MAX_NUM])(struct IMGSENSOR_HW_DEVICE **) = {
	imgsensor_hw_regulator_open,
	imgsensor_hw_gpio_open,
	imgsensor_hw_mclk_open
};

struct IMGSENSOR_HW_CFG imgsensor_custom_config[] = {
	{
		IMGSENSOR_SENSOR_IDX_MAIN,
		IMGSENSOR_I2C_DEV_0,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AFVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_SUB,
		IMGSENSOR_I2C_DEV_1,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
#ifdef MIPI_SWITCH
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_MIPI_SWITCH_EN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL},
#endif
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},
	{
		IMGSENSOR_SENSOR_IDX_MAIN2,
		IMGSENSOR_I2C_DEV_2,
		{
			{IMGSENSOR_HW_ID_MCLK, IMGSENSOR_HW_PIN_MCLK},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_AVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DOVDD},
			{IMGSENSOR_HW_ID_REGULATOR, IMGSENSOR_HW_PIN_DVDD},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_PDN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_RST},
#ifdef MIPI_SWITCH
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_MIPI_SWITCH_EN},
			{IMGSENSOR_HW_ID_GPIO, IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL},
#endif
			{IMGSENSOR_HW_ID_NONE, IMGSENSOR_HW_PIN_NONE},
		},
	},

	{IMGSENSOR_SENSOR_IDX_NONE}
};

struct IMGSENSOR_HW_POWER_SEQ platform_power_sequence[] = {
#ifdef MIPI_SWITCH
	{
		IMGSENSOR_SENSOR_IDX_NAME_SUB,
		{
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_EN,
				IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0
			},
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
				0
			},
		}
	},
	{
		IMGSENSOR_SENSOR_IDX_NAME_MAIN2,
		{
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_EN,
				IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0
			},
			{
				IMGSENSOR_HW_PIN_MIPI_SWITCH_SEL,
				IMGSENSOR_HW_PIN_STATE_LEVEL_HIGH,
				0,
				IMGSENSOR_HW_PIN_STATE_LEVEL_0,
				0
			},
		}
	},
#endif

	{NULL}
};

/* Legacy design */
struct IMGSENSOR_HW_POWER_SEQ sensor_power_sequence[] = {
/*zhengjiang.zhu@Koobee.Camera.Driver  2018/08/27  add for hi846 & bf2206*/
#if defined(HI846_MIPI_RAW)
	{
		SENSOR_DRVNAME_HI846_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(BF2206_MIPI_RAW)
		{
			SENSOR_DRVNAME_BF2206_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{PDN, Vol_High, 1},
				{RST, Vol_Low, 10},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AVDD, Vol_2800, 5},
				//{AFVDD, Vol_2800, 1},
				{PDN, Vol_Low, 5},
				{RST, Vol_High, 5}
			},
		},
#endif
#if defined(S5K5E8YX_MIPI_RAW_BYD)
	{
		SENSOR_DRVNAME_S5K5E8YX_BYD_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(GC8034_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC8034_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
/*zhengjiang.zhu@Koobee.Camera.Driver  2018/08/27  endif for hi846 & bf2206*/
#if defined(S5K5E8YX_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K5E8YX_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(OV13870_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV13870_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(IMX398_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX398_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1},
		},
	},
#endif
#if defined(OV23850_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV23850_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 2},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(IMX386_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX386_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1},
		},
	},
#endif

#if defined(IMX338_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX338_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2500, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1100, 0},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(S5K4E6_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K4E6_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 1},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2900, 0},
			{DVDD, Vol_1200, 2},
			{AFVDD, Vol_2800, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K3P8SP_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3P8SP_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
		},
	},
#endif
#if defined(S5K2T7SP_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2T7SP_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
		},
	},
#endif
#if defined(S5K3M2_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3M2_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K3P3SX_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3P3SX_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K5E2YA_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K5E2YA_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K4ECGX_MIPI_YUV)
	{
		SENSOR_DRVNAME_S5K4ECGX_MIPI_YUV,
		{
			{DVDD, Vol_1200, 1},
			{AVDD, Vol_2800, 1},
			{DOVDD, Vol_1800, 1},
			{AFVDD, Vol_2800, 0},
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(OV16880_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV16880_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(S5K2P7_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2P7_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0},
		},
	},
#endif
#if defined(S5K2P8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2P8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 4},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 1},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX258_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX258_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX258_MIPI_MONO)
	{
		SENSOR_DRVNAME_IMX258_MIPI_MONO,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX377_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX377_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(OV8858_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV8858_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 1},
			{AVDD, Vol_2800, 1},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 1},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(OV8856_MIPI_RAW)
	{SENSOR_DRVNAME_OV8856_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 2},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(S5K2X8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2X8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX214_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX214_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
            {CAMAFEN, Vol_High, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(IMX214_MIPI_MONO)
	{
		SENSOR_DRVNAME_IMX214_MIPI_MONO,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(IMX230_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX230_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K3L8_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3L8_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(IMX362_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX362_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K2L7_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K2L7_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 3},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(IMX318_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX318_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(OV8865_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV8865_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(IMX219_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX219_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 10},
			{DOVDD, Vol_1800, 10},
			{DVDD, Vol_1200, 10},
			{AFVDD, Vol_2800, 5},
			{CAMAFEN, Vol_High, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 0}
		},
	},
#endif
#if defined(S5K3M3_MIPI_RAW)
	{
		SENSOR_DRVNAME_S5K3M3_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1000, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 2}
		},
	},
#endif
#if defined(OV5670_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV5670_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(OV5670_MIPI_RAW_2)
	{
		SENSOR_DRVNAME_OV5670_MIPI_RAW_2,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 5},
			{RST, Vol_Low, 5},
			{DOVDD, Vol_1800, 5},
			{AVDD, Vol_2800, 5},
			{DVDD, Vol_1200, 5},
			{AFVDD, Vol_2800, 5},
			{PDN, Vol_High, 5},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(OV20880_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV20880_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{RST, Vol_Low, 1},
			{AVDD, Vol_2800, 1},
			{DOVDD, Vol_1800, 1},
			{DVDD, Vol_1100, 1},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(GC2365_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC2365_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_High, 1},
			{RST, Vol_Low, 10},
			{DOVDD, Vol_1800, 5},
			{DVDD, Vol_1200, 5},
			{AVDD, Vol_2800, 5},
			{PDN, Vol_Low, 5},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(GC2366_MIPI_RAW)
	{
		SENSOR_DRVNAME_GC2366_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_High, 1},
			{RST, Vol_Low, 10},
			{DOVDD, Vol_1800, 5},
			{DVDD, Vol_1200, 5},
			{AVDD, Vol_2800, 5},
			{PDN, Vol_Low, 5},
			{RST, Vol_High, 5}
		},
	},
#endif
/* prize added by chenjiaxi, add camera power on/off, 20190111-start */
#if defined(OV13850_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV13850_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
            {CAMAFEN, Vol_High, 5},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(OV13853_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV13853_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
            {CAMAFEN, Vol_High, 5},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
	/*Test*/
#if defined(OV13870_MIPI_RAW_5MP)
	{
		SENSOR_DRVNAME_OV13870_MIPI_RAW_5MP,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
#if defined(OV8856_MIPI_RAW_5MP)
	{SENSOR_DRVNAME_OV8856_MIPI_RAW_5MP,
		{
			{SensorMCLK, Vol_High, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 2},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 5},
		},
	},
#endif
#if defined(GC2755_MIPI_RAW)
		{
			SENSOR_DRVNAME_GC2755_MIPI_RAW,           //5024
			{
				{SensorMCLK,Vol_High, 0},
				//{AFVDD, Vol_2800, 0},
				{DOVDD, Vol_1800, 1},
				{DVDD,  Vol_1500, 1},
				{AVDD,  Vol_2800, 1},		
				{PDN,   Vol_High,  4},           //Vol_Low
				{PDN,   Vol_Low, 0},
				{RST,   Vol_Low,  10},
				{RST,   Vol_High, 1},
			},
		},
#endif
#if defined(GC2385_MIPI_RAW)
		{
			SENSOR_DRVNAME_GC2385_MIPI_RAW,           //5024
			{
				{SensorMCLK,Vol_High, 0},
				//{AFVDD, Vol_2800, 0},
				{DOVDD, Vol_1800, 1},
				{DVDD,  Vol_1500, 1},
				{AVDD,  Vol_2800, 1},		
				{PDN,   Vol_Low,  4},           //Vol_Low
				{PDN,   Vol_High, 0},
				{RST,   Vol_Low,  10},
				{RST,   Vol_High, 1},
			},
		},
#endif
#if defined(OV5648_MIPI_RAW)
		{
			SENSOR_DRVNAME_OV5648_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{PDN, Vol_Low, 5},
				{RST, Vol_Low, 5},
				{DOVDD, Vol_1800, 5},
				{AVDD, Vol_2800, 5},
				{DVDD, Vol_1500, 5},
				{AFVDD, Vol_2800, 5},
				{PDN, Vol_High, 5},
				{RST, Vol_High, 5}
				//{DOVDD, Vol_1800, 0},
				//{AVDD, Vol_2800, 0},
				//{DVDD, Vol_1200, 0},
				//{AFVDD, Vol_2800, 2},
				//{PDN, Vol_Low, 0},
				//{PDN, Vol_High, 0},
				//{RST, Vol_Low, 0},
				//{RST, Vol_High, 5},
			},
		},
#endif
#if defined(HM8040_MIPI_RAW)
		{
			SENSOR_DRVNAME_HM8040_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 0},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 0},
				{RST, Vol_High, 1}
			},
		},
#endif
#if defined(OV8865_MIPI_RAW)
		{
			SENSOR_DRVNAME_OV8865_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif
#if defined(T4KB3_MIPI_RAW)
		{
			SENSOR_DRVNAME_T4KB3_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif
#if defined(S5K4H8_MIPI_RAW)
		{
			SENSOR_DRVNAME_S5K4H8_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif
#if defined(S5K3H2YX_MIPI_RAW)
		{
			SENSOR_DRVNAME_S5K3H2YX_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif
#if defined(S5K3L2_MIPI_RAW)
		{
			SENSOR_DRVNAME_S5K3L2_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif

#if defined(GC5025_MIPI_RAW)
		{
			SENSOR_DRVNAME_GC5025_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif

#if defined(AR1335_MIPI_RAW)
		{SENSOR_DRVNAME_AR1335_MIPI_RAW,
			{
				/*
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
				*/
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 0},
				{PDN, Vol_High, 0},
				{RST, Vol_Low, 0},
				{RST, Vol_High, 0}
			},
		},
#endif

#if defined(GC5025MAIN_MIPI_RAW)
		{
			SENSOR_DRVNAME_GC5025MAIN_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif
#if defined(IMX134_SENSOR_ID)
		{
			SENSOR_DRVNAME_IMX134_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif

#if defined(IMX135_SENSOR_ID)
		{
			SENSOR_DRVNAME_IMX135_MIPI_RAW,
			{
				
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				// {CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif

#if defined(S5K4H5YC_SENSOR_ID)
		{
			SENSOR_DRVNAME_S5K4H5YC_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif
/* prize added by wangmengdong,imgsensor, add for S5K4H5YXKERR, 20190119-start */
#if defined(S5K4H5YXKERR_MIPI_RAW)
		{
			SENSOR_DRVNAME_S5K4H5YXKERR_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif
/* prize added by wangmengdong,imgsensor, add for S5K4H5YXKERR, 20190119-end */
#if defined(HI841_MIPI_RAW)
		{
			SENSOR_DRVNAME_HI841_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif

#if defined(HI551_MIPI_RAW)
		{
			SENSOR_DRVNAME_HI551_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				//{AFVDD, Vol_2800, 5},
				//{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif

#if defined(HI846_MIPI_RAW)
		{
			SENSOR_DRVNAME_HI846_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{PDN, Vol_Low, 5},
				{RST, Vol_Low, 5},
				{DOVDD, Vol_1800, 5},
				{AVDD, Vol_2800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				{CAMAFEN, Vol_High, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_High, 1}
			},
		},
#endif
#if defined(GC8024_MIPI_RAW)
		{
			SENSOR_DRVNAME_GC8024_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				//{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif
/* prize added by yaozhipeng, gc8034, 20190123-start */
#if defined(GC8034_MIPI_RAW)
		{
			SENSOR_DRVNAME_GC8034_MIPI_RAW,
			{
				{PDN, Vol_Low, 0},
				{RST, Vol_Low, 0},
				{DVDD, Vol_1200, 5},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{AFVDD, Vol_2800, 5},
				{SensorMCLK, Vol_High, 0},
				{CAMAFEN, Vol_High, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_High, 1}
			},
		},
#endif
/* prize added by yaozhipeng, gc8034, 20190123-end */
#if defined(S5K4H7YX_MIPI_RAW)
		{
			SENSOR_DRVNAME_S5K4H7YX_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif
#if defined(SP5509_MIPI_RAW)
		{
			SENSOR_DRVNAME_SP5509_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif
#if defined(S5K3H2YX_MIPI_RAW)
		{
			SENSOR_DRVNAME_S5K3H2YX_MIPI_RAW,
			{
				{SensorMCLK, Vol_High, 0},
				{AVDD, Vol_2800, 5},
				{DOVDD, Vol_1800, 5},
				{DVDD, Vol_1200, 5},
				{AFVDD, Vol_2800, 5},
				{CAMAFEN, Vol_High, 5},
				{PDN, Vol_Low, 5},
				{PDN, Vol_High, 4},
				{RST, Vol_Low, 5},
				{RST, Vol_High, 1}
			},
		},
#endif
#if defined(IMX190_MIPI_RAW)
	{
		SENSOR_DRVNAME_IMX190_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{AVDD, Vol_2800, 0},
			{DOVDD, Vol_1800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
            {CAMAFEN, Vol_High, 5},
			{PDN, Vol_Low, 0},
			{PDN, Vol_High, 0},
			{RST, Vol_Low, 0},
			{RST, Vol_High, 1}
		},
	},
#endif
#if defined(OV12830_MIPI_RAW)
	{
		SENSOR_DRVNAME_OV12830_MIPI_RAW,
		{
			{SensorMCLK, Vol_High, 0},
			{PDN, Vol_Low, 0},
			{RST, Vol_Low, 0},
			{DOVDD, Vol_1800, 0},
			{AVDD, Vol_2800, 0},
			{DVDD, Vol_1200, 0},
			{AFVDD, Vol_2800, 1},
			{PDN, Vol_High, 0},
			{RST, Vol_High, 5}
		},
	},
#endif
/* prize added by chenjiaxi, add camera power on/off, 20190111-end */
	/* add new sensor before this line */
	{NULL,},
};

