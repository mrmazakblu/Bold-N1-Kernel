/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include "kd_imgsensor.h"
#include "imgsensor_sensor_list.h"

/* Add Sensor Init function here
* Note:
* 1. Add by the resolution from ""large to small"", due to large sensor
*    will be possible to be main sensor.
*    This can avoid I2C error during searching sensor.
* 2. This file should be the same as mediatek\custom\common\hal\imgsensor\src\sensorlist.cpp
*/
struct IMGSENSOR_INIT_FUNC_LIST kdSensorList[MAX_NUM_OF_SUPPORT_SENSOR] = {
/* zhengjiang.zhu@Koobee..Camera.Driver  2018/04/13  add for Hi846 & BF2206 */
/* prize added by chenjiaxi, imx219, 20190112-start */
#if defined(IMX219_MIPI_RAW)
	{IMX219_SENSOR_ID, SENSOR_DRVNAME_IMX219_MIPI_RAW, IMX219_MIPI_RAW_SensorInit},
#endif
/* prize added by chenjiaxi, imx219, 20190112-end */
/* prize added by chenjiaxi, imx134, 20190112-start */
#if defined(IMX134_MIPI_RAW)
	{IMX134_SENSOR_ID, SENSOR_DRVNAME_IMX134_MIPI_RAW, IMX134_MIPI_RAW_SensorInit},
#endif
/* prize added by chenjiaxi, imx134, 20190112-end */
	/*OV (OmniVision)*/
#if defined(OV8865_MIPI_RAW)
	{OV8865_SENSOR_ID, SENSOR_DRVNAME_OV8865_MIPI_RAW, OV8865_MIPI_RAW_SensorInit},
#endif
/* prize added by chenjiaxi, add for ov5648, 20190111-start */
#if defined(OV5648_MIPI_RAW)
	{OV5648MIPI_SENSOR_ID, SENSOR_DRVNAME_OV5648_MIPI_RAW, OV5648MIPISensorInit},
#endif
/* prize added by chenjiaxi, add for ov5648, 20190111-end */
/* prize added by yaozhipeng, add for ov13850, 20190123-start */
#if defined(OV13850_MIPI_RAW)
	{OV13850_SENSOR_ID, SENSOR_DRVNAME_OV13850_MIPI_RAW, OV13850_MIPI_RAW_SensorInit},
#endif
/* prize added by yaozhipeng, add for ov13850, 20190123-end */
/* prize added by chenjiaxi, s5k4h7yx, 20190112-start */
#if defined(S5K4H7YX_MIPI_RAW)
	{S5K4H7YX_SENSOR_ID, SENSOR_DRVNAME_S5K4H7YX_MIPI_RAW, S5K4H7YX_MIPI_RAW_SensorInit},
#endif
/* prize added by chenjiaxi, s5k4h7yx, 20190112-end */
/* prize added by wangmengdong,imgsensor, add for S5K4H5YXKERR, 20190119-start */
#if defined(S5K4H5YXKERR_MIPI_RAW)
	{S5K4H5YXKERR_SENSOR_ID, SENSOR_DRVNAME_S5K4H5YXKERR_MIPI_RAW, S5K4H5YXKERR_MIPI_RAW_SensorInit},
#endif
/* prize added by wangmengdong,imgsensor, add for S5K4H5YXKERR, 20190119-end */
#if defined(HI846_MIPI_RAW)
	{HI846_SENSOR_ID, SENSOR_DRVNAME_HI846_MIPI_RAW,HI846_MIPI_RAW_SensorInit},
#endif
	/*GC*/
/* prize added by yaozhipeng, add for gc8034 gc8034, 20190123-start */
#if defined(GC8034_MIPI_RAW)
    {GC8034_SENSOR_ID, SENSOR_DRVNAME_GC8034_MIPI_RAW, GC8034_MIPI_RAW_SensorInit},
#endif
#if defined(GC8024_MIPI_RAW)
    {GC8024_SENSOR_ID, SENSOR_DRVNAME_GC8024_MIPI_RAW, GC8024_MIPI_RAW_SensorInit},
#endif
/* prize added by yaozhipeng, add for gc8034 gc8034, 20190123-end */
/* prize added by chenjiaxi, gc5025, 20190112-start */
#if defined(GC5025_MIPI_RAW)
    {GC5025_SENSOR_ID, SENSOR_DRVNAME_GC5025_MIPI_RAW,GC5025MIPI_RAW_SensorInit},
#endif
/* prize added by chenjiaxi, gc5025, 20190112-end */
#if defined(GC2755_MIPI_RAW)
    {GC2755_SENSOR_ID, SENSOR_DRVNAME_GC2755_MIPI_RAW,GC2755_MIPI_RAW_SensorInit},
#endif
#if defined(GC2385_MIPI_RAW)
    {GC2385_SENSOR_ID, SENSOR_DRVNAME_GC2385_MIPI_RAW,GC2385MIPI_RAW_SensorInit},
#endif
	/*HM*/
/* prize added by chenjiaxi, add for hm8040, 20190111-start */
#if defined(HM8040_MIPI_RAW)
    {HM8040_SENSOR_ID, SENSOR_DRVNAME_HM8040_MIPI_RAW, HM8040_MIPI_RAW_SensorInit},
#endif
/* prize added by chenjiaxi, add for hm8040, 20190111-end */
#if defined(BF2206_MIPI_RAW)
    {BF2206_SENSOR_ID, SENSOR_DRVNAME_BF2206_MIPI_RAW, BF2206MIPI_RAW_SensorInit},
#endif


	/*  ADD sensor driver before this line */
	{0, {0}, NULL}, /* end of list */
};
/* e_add new sensor driver here */

