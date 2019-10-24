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

#define LOG_TAG "LCM"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"


#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
/*#include <mach/mt_pm_ldo.h>*/
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>
#endif
#endif
#ifdef CONFIG_MTK_LEGACY
#include <cust_gpio_usage.h>
#endif
#ifndef CONFIG_FPGA_EARLY_PORTING
#if defined(CONFIG_MTK_LEGACY)
#include <cust_i2c.h>
#endif
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_notice("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

#define I2C_I2C_LCD_BIAS_CHANNEL 0
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)			(lcm_util.set_reset_pin((v)))
#define MDELAY(n)					(lcm_util.mdelay(n))

#ifndef BUILD_LK
	extern int tps65132_set_vpos_volt(int);
	extern int tps65132_set_vneg_volt(int);
	extern int tps65132_vpos_enable(bool);
	extern int tps65132_vneg_enable(bool);
	//#define SET_GPIO_LCD_ENP_LDO18_PIN(v)			(lcm_util.set_gpio_lcd_enp_ldo18((v)))
	//#define SET_GPIO_LCD_ENP_LDO28_PIN(v)			(lcm_util.set_gpio_lcd_enp_ldo28((v)))
	//#define SET_GPIO_LCD_ENP_LDO28_PIN(v)
#else	//BUILD_LK
	#if defined(GPIO_LCD_BIAS_ENP_PIN)	//define in dws
		#define GPIO_LCD_BIAS_ENP(v)	\
			do{	\
				if (v){	\
					mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00); 	\
					mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);		\
					mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);		\
				}else{	\
					mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00); 	\
					mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);		\
					mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);	\
				}	\
			}while(0);
	#else
		#define GPIO_LCD_BIAS_ENP(v)
	#endif
	#if defined(GPIO_LCD_BIAS_ENN_PIN)	//define in dws
		#define GPIO_LCD_BIAS_ENN(v)	\
			do{	\
				if (v){	\
					mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00); 	\
					mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);		\
					mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);		\
				}else{	\
					mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00); 	\
					mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);		\
					mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);	\
				}	\
			}while(0);
	#else
		#define GPIO_LCD_BIAS_ENN(v)
	#endif
	#if defined(GPIO_LCD_LDO18_PIN)	//define in dws
		#define SET_GPIO_LCD_ENP_LDO18_PIN(v)	\
			do{	\
				if (v){	\
					mt_set_gpio_mode(GPIO_LCD_LDO18_PIN, GPIO_MODE_00); 	\
					mt_set_gpio_dir(GPIO_LCD_LDO18_PIN, GPIO_DIR_OUT);		\
					mt_set_gpio_out(GPIO_LCD_LDO18_PIN, GPIO_OUT_ONE);		\
				}else{	\
					mt_set_gpio_mode(GPIO_LCD_LDO18_PIN, GPIO_MODE_00); 	\
					mt_set_gpio_dir(GPIO_LCD_LDO18_PIN, GPIO_DIR_OUT);		\
					mt_set_gpio_out(GPIO_LCD_LDO18_PIN, GPIO_OUT_ZERO);	\
				}	\
			}while(0);
	#else
		#define SET_GPIO_LCD_ENP_LDO18_PIN(v)
	#endif
	#if defined(GPIO_LCD_LDO28_PIN)	//define in dws
		#define SET_GPIO_LCD_ENP_LDO28_PIN(v)	\
			do{	\
				if (v){	\
					mt_set_gpio_mode(GPIO_LCD_LDO28_PIN, GPIO_MODE_00); 	\
					mt_set_gpio_dir(GPIO_LCD_LDO28_PIN, GPIO_DIR_OUT);		\
					mt_set_gpio_out(GPIO_LCD_LDO28_PIN, GPIO_OUT_ONE);		\
				}else{	\
					mt_set_gpio_mode(GPIO_LCD_LDO28_PIN, GPIO_MODE_00); 	\
					mt_set_gpio_dir(GPIO_LCD_LDO28_PIN, GPIO_DIR_OUT);		\
					mt_set_gpio_out(GPIO_LCD_LDO28_PIN, GPIO_OUT_ZERO);	\
				}	\
			}while(0);
	#else
		#define SET_GPIO_LCD_ENP_LDO28_PIN(v) 
	#endif
#endif

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) \
	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) \
	lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) \
	lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) \
	lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) \
	lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) \
	lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)


static const unsigned char LCD_MODULE_ID = 0x01;
/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */
#define LCM_DSI_CMD_MODE	0
#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1440)

#define REGFLAG_DELAY             							 0xFFA
#define REGFLAG_UDELAY             							 0xFFB
#define REGFLAG_PORT_SWAP									 0xFFC
#define REGFLAG_END_OF_TABLE      							 0xFFD   // END OF REGISTERS MARKER

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

#define  SET_GPIO_LCD_ENP_LDO18(v)							(lcm_util.set_gpio_lcd_enp_ldo18((v)))
#define  SET_GPIO_LCD_ENP_LDO28(v)							(lcm_util.set_gpio_lcd_enp_ldo28((v)))

extern int tps65132_vpos_enable(bool);
extern int tps65132_vneg_enable(bool);
extern int tps65132_set_vpos_volt(int);
extern int tps65132_set_vneg_volt(int);
struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};
#if 1
static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 1,{0x00}},
	{REGFLAG_DELAY, 100, {} },
	{0x10, 1,{0x00}},
	{REGFLAG_DELAY, 50, {} }
};
#endif
static struct LCM_setting_table lcm_initialization_setting[] = {
{0xB9,3,{0xF1,0x12,0x83}},  
 
{0xBA,27,{0x33,0x81,0x05,0xF9,0x0E,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x44,
	        0x25,0x00,0x91,0x0A,0x00,0x00,0x02,0x4F,0x11,0x03,0x02,0x37}}, //27 // 1 0x33:4Lane, 0x32:3Lane
 
{0xB8,1,{0x78}}, //0x75 for 3 Power Mode,0x25 for Power IC Mode

{0xBF,4,{0x02,0x10,0x00}},

{0xB3,10,{0x07,0x0B,0x1E,0x1E,0x03,0xFF,0x00,0x00,0x00,0x00}},  

{0xC0,9,{0x73,0x73,0x50,0x50,0xC0,0x00,0x08,0x70,0x00}},  

{0xBC,1,{0x46}},  
{0xCC,1,{0x0b}},   // 0x0B:Forward , 0x07:Backward

{0xB4,1,{0x80}},   //Set Panel inversion

{0xB2,3,{0xF0,0x12,0xf0}},   

{0xE3,14,{0x07,0x07,0x0B,0x0B,0x07,0x0B,0x00,0x00,0x00,0x00,0xFF,0x80,0xC0,0x10}},  

{0xC1,12,{0x74,0x00,0x1e,0x1e,0x77,0xf1,0xcc,0xdd,0x67,0x67,0x33,0x33}}, 

{0xB5,2,{0x07,0x07}},

{0xCB,1,{0x02}},

//{0xC7,1,{0x10}},

{0xB6,2,{0xb5,0xb5}},//Set VCOMs

{0xE9,63,{0x82,0x10,0x05,0x05,0xA1,0x08,0xa0,0x12,0x31,0x23,0x37,0x82,0x08,0xa0,0x37,0x00,
	        0x00,0x81,0x00,0x00,0x00,0x00,0x00,0x81,0x00,0x00,0x00,0x00,0xF8,0xAB,0x02,0x46,
	        0x08,0x88,0x88,0x84,0x88,0x88,0x88,0xF8,0xAB,0x13,0x57,0x18,0x88,0x88,0x85,0x88,
	        0x88,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}}, 

{0xEA,61,{0x0B,0x12,0x01,0x01,0x00,0x3D,0x00,0x00,0x00,0x00,0x00,0x00,0x8F,0xAB,0x75,0x31,0x58,
	        0x88,0x88,0x81,0x88,0x88,0x88,0x8F,0xAB,0x64,0x20,0x48,0x88,0x88,0x80,0x88,0x88,0x88,
	        0x23,0x00,0x00,0x01,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	        0x00,0x00,0x00,0x30,0x08,0xE0,0x00,0x00,0x00,0x00}},

{0xE0,34,{0x00,0x06,0x0B,0x24,0x2B,0x3F,0x39,0x33,0x05,0x0A,0x0C,0x10,0x11,0x0F,0x12,0x16,0x1C,
	        0x00,0x06,0x0B,0x24,0x2B,0x3F,0x39,0x33,0x05,0x0A,0x0C,0x10,0x11,0x0F,0x12,0x16,0x1C}},

{0x11,1,{}}, ////Sleep Out
{REGFLAG_DELAY,120,{}},

{0x29,1,{}}, ///Display On
{REGFLAG_DELAY,10,{}},
};	

//#if 0
static void push_table(struct LCM_setting_table *table, unsigned int count,
		       unsigned char force_update)
{
	unsigned int i;
    LCM_LOGI("nt35695----tps6132-lcm_init   push_table++++++++++++++===============================devin----\n");
	for (i = 0; i < count; i++) {
		unsigned cmd;

		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY:
			if (table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE:
			break;

		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}




/* --------------------------------------------------------------------------- */
/* LCM Driver Implementations */
/* --------------------------------------------------------------------------- */

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
	
	params->density = 320;//LCM_DENSITY;

		// enable tearing-free
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
    params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
        #else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;//SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;////
        #endif
	
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;
	
	
	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	
#if (LCM_DSI_CMD_MODE)
	params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
	params->dsi.word_count=FRAME_WIDTH*3;	//DSI CMD mode need set these two bellow params, different to 6577
#else
	params->dsi.intermediat_buffer_num = 2;	//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
#endif

	// Video mode setting
	params->dsi.packet_size=256;

    params->dsi.vertical_sync_active			=8;// 3;
    params->dsi.vertical_backporch				=20;// 14;
    params->dsi.vertical_frontporch				= 25;//16;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active			= 10;//11;
    params->dsi.horizontal_backporch				= 40;//64;20
    params->dsi.horizontal_frontporch				= 50;//64;20
    params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

//    params->dsi.HS_TRAIL= 10;

    params->dsi.PLL_CLOCK=205;//234

	params->dsi.cont_clock = 1;
	params->dsi.clk_lp_per_line_enable = 0;
	params->dsi.ssc_disable = 1;	
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
		params->dsi.lcm_esd_check_table[0].cmd  				= 0x68;
		params->dsi.lcm_esd_check_table[0].count  			= 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0xc0;
		params->dsi.lcm_esd_check_table[1].cmd  				= 0xAF;
		params->dsi.lcm_esd_check_table[1].count  			= 1;
		params->dsi.lcm_esd_check_table[1].para_list[0] = 0xFD;
		params->dsi.lcm_esd_check_table[2].cmd  				= 0x09;
		params->dsi.lcm_esd_check_table[2].count  			= 1;
		params->dsi.lcm_esd_check_table[2].para_list[0] = 0x80;
		/*params->dsi.lcm_esd_check_table[3].cmd  				= 0x68;
		params->dsi.lcm_esd_check_table[3].count  			= 1;
		params->dsi.lcm_esd_check_table[3].para_list[0] = 0xC0;*/
	
	// enable tearing-free
	/*params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;*/
}



#define LCM_ID (0x38)
static unsigned int lcm_compare_id(void)
{
	
 /*   unsigned int id0,id1,id=0;
	unsigned char buffer[2];
	unsigned int array[16];  
    mt_set_gpio_mode(VLCM_LDO_EN3_V18, GPIO_MODE_00);
	mt_set_gpio_dir(VLCM_LDO_EN3_V18, GPIO_DIR_OUT);
	mt_set_gpio_out(VLCM_LDO_EN3_V18, GPIO_OUT_ONE);
	MDELAY(5);
	mt_set_gpio_mode(VLCM_LDO_EN2_V28, GPIO_MODE_00);
	mt_set_gpio_dir(VLCM_LDO_EN2_V28, GPIO_DIR_OUT);
	mt_set_gpio_out(VLCM_LDO_EN2_V28, GPIO_OUT_ONE);
	
	SET_RESET_PIN(1);
	
	MDELAY(20);//100

	SET_RESET_PIN(0);
	MDELAY(20);//100
	lcm_set_bias();

	MDELAY(10);  

	SET_RESET_PIN(1);
	MDELAY(250);//250
	
   

	array[0]=0x00DE0500;
	dsi_set_cmdq(array, 1, 1);

	array[0]=0x32B41500; 
	dsi_set_cmdq(array, 1, 1);

	array[0]=0x00DF0500;
	dsi_set_cmdq(array, 1, 1);

	array[0] = 0x00013700;// read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);

	read_reg_v2(0x0C, buffer, 1);
	
    #ifdef BUILD_LK
	printf("%s, LK TDDI id = 0x%08x\n", __func__, buffer[0]);
   #else
	printk("%s, Kernel TDDI id = 0x%08x\n", __func__, buffer[0]);
   #endif

   return (0x77 == buffer[0])?1:0; 
	//return 1;//(LCM_ID_NT35532 == id)?1:0;*/

    unsigned int  data_array[16];
    unsigned char buffer[3];
    unsigned int  id = 0;

    SET_RESET_PIN(1);
	//lcm_set_bias();
	display_ldo18_enable(1);
	//SET_GPIO_LCD_ENP_LDO28(1);
	tps65132_set_vpos_volt(5800);
	tps65132_set_vneg_volt(5800);
	tps65132_vpos_enable(1);
	tps65132_vneg_enable(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(20);
    SET_RESET_PIN(1);
    MDELAY(20);

    data_array[0]=0x00043902;
    data_array[1]=0x018198FF;
    dsi_set_cmdq(data_array,2,1);

    data_array[0] = 0x00023700;
    dsi_set_cmdq(data_array, 1, 1);

    read_reg_v2(0x04, buffer, 1);
    id = buffer[0];
    printk("%s, st7703 id = 0x%x\n", __func__, id);

    return (LCM_ID == id)?1:0; 
}

static void lcm_init(void)
{
	//SET_RESET_PIN(1);
	//MDELAY(100);
	LCM_LOGI("nt35695----tps6132-lcm_init3333333333333333333333333333===============================devin----\n");
	
    display_ldo18_enable(1);
	//SET_GPIO_LCD_ENP_LDO28(1);
	tps65132_set_vpos_volt(5800);
	tps65132_set_vneg_volt(5800);
	tps65132_vpos_enable(1);
	tps65132_vneg_enable(1);
	
	MDELAY(10);
	SET_RESET_PIN(1);
	
	MDELAY(10);
	//lcm_set_bias();
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(20);//100

	SET_RESET_PIN(1);
	MDELAY(20);//250
	
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);

}

static void lcm_suspend(void)
{
	
 /* //  unsigned int data_array[16];
    
	//data_array[0]=0x00280500; // Display Off
	//dsi_set_cmdq(data_array, 1, 1);
   // MDELAY(20);

	//data_array[0] = 0x00100500; // Sleep In
	//dsi_set_cmdq(data_array, 1, 1);
   // MDELAY(120);
	     
//	NT35532_DCS_write_1A_1P(0xFF,0x05);
   // mt_set_gpio_mode(INC18_ENP_PIN, GPIO_MODE_00);
	//mt_set_gpio_dir(INC18_ENP_PIN, GPIO_DIR_OUT);
	//mt_set_gpio_out(INC18_ENP_PIN, GPIO_OUT_ZERO);
	
	
	mt_set_gpio_mode(VLCM_LDO_EN3_V18, GPIO_MODE_00);
	mt_set_gpio_dir(VLCM_LDO_EN3_V18, GPIO_DIR_OUT);
    mt_set_gpio_out(VLCM_LDO_EN3_V18, GPIO_OUT_ONE);
	//mt_set_gpio_mode(INC33_ENP_PIN, GPIO_MODE_00);
	//mt_set_gpio_dir(INC33_ENP_PIN, GPIO_DIR_OUT);
	//mt_set_gpio_out(INC33_ENP_PIN, GPIO_OUT_ZERO);
	SET_RESET_PIN(1);
	MDELAY(10);
	
	
	//lcm_set_bias();
	
	push_table(lcm_suspend_setting,
		   sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	//MDELAY(10);
	//SET_RESET_PIN(0);
//MDELAY(10);*/
  push_table(lcm_suspend_setting,
		   sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
       
	SET_RESET_PIN(0);
         MDELAY(20);
	
	display_ldo18_enable(0);
	//SET_GPIO_LCD_ENP_LDO28(0);
	tps65132_vpos_enable(0);
	tps65132_vneg_enable(0);
}

static void lcm_resume(void)
{
	lcm_init();
}

#if (LCM_DSI_CMD_MODE)
static void lcm_update(unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0 >> 8) & 0xFF);
	unsigned char x0_LSB = (x0 & 0xFF);
	unsigned char x1_MSB = ((x1 >> 8) & 0xFF);
	unsigned char x1_LSB = (x1 & 0xFF);
	unsigned char y0_MSB = ((y0 >> 8) & 0xFF);
	unsigned char y0_LSB = (y0 & 0xFF);
	unsigned char y1_MSB = ((y1 >> 8) & 0xFF);
	unsigned char y1_LSB = (y1 & 0xFF);

	unsigned int data_array[16];

	data_array[0] = 0x00053902;
	data_array[1] = (x1_MSB << 24) | (x0_LSB << 16) | (x0_MSB << 8) | 0x2a;
	data_array[2] = (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x00053902;
	data_array[1] = (y1_MSB << 24) | (y0_LSB << 16) | (y0_MSB << 8) | 0x2b;
	data_array[2] = (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0] = 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}
#endif

//#define LCM_ID_NT35532 (0x32)



LCM_DRIVER st7703_dsi_vdo_hd1440_dzx_55_hsd_lcm_drv = 
{
    .name           = "st7703_dsi_vdo_hd1440_dzx_55_hsd",
	#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "ST7703",
		.vendor	= "unknow",
		.id		= "0x7",
		.more	= "1440*720",
	},
	#endif
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id    = lcm_compare_id,
	//.esd_check = lcm_esd_check,
    #if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
    #endif
    };
