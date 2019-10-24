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
	{REGFLAG_DELAY, 10, {} },
	{0x10, 1,{0x00}},
	{REGFLAG_DELAY, 120, {} }
};
#endif
 
void init_lcm_registers(void)
{
unsigned int data_array[16];


data_array[0] = 0x00043902;
data_array[1] = 0x9483FFB9;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00073902;
data_array[1] = 0x680363BA;//data_array[1] = 0x680363BA;  4lane
data_array[2] = 0x00C0B26B;
dsi_set_cmdq(data_array, 3, 1);
MDELAY(1);

data_array[0] = 0x000B3902;
data_array[1] = 0x711148B1;
data_array[2] = 0x71443209;
data_array[3] = 0x00305551;
dsi_set_cmdq(data_array, 4, 1);
MDELAY(1);

data_array[0] = 0x00063902;
data_array[1] = 0x788000B2;
data_array[2] = 0x0000070C;
dsi_set_cmdq(data_array, 3, 1);
MDELAY(1);

data_array[0] = 0x00163902;
data_array[1] = 0x016301B4;
data_array[2] = 0x01630163;
data_array[3] = 0x00557C0C;
data_array[4] = 0x0163013F;
data_array[5] = 0x01630163;
data_array[6] = 0x00007C0C;
dsi_set_cmdq(data_array, 7, 1);
MDELAY(1);


data_array[0] = 0x003B3902;
data_array[1] = 0x060200E0;
data_array[2] = 0x110F0C0B;
data_array[3] = 0x4130210F;
data_array[4] = 0x6B634041;
data_array[5] = 0x84868271;
data_array[6] = 0x4F52A593;
data_array[7] = 0x7A716753;
data_array[8] = 0x02007F7F;
data_array[9] = 0x0E0B0A06;
data_array[10] = 0x30210F11;
data_array[11] = 0x63424141;
data_array[12] = 0x8782716B;
data_array[13] = 0x52A59484;
data_array[14] = 0x71675450;
data_array[15] = 0x007F7F7B;
dsi_set_cmdq(data_array, 16, 1);
MDELAY(1);






data_array[0] = 0x00223902;
data_array[1] = 0x000000D3;
data_array[2] = 0x001C3C00;
data_array[3] = 0x09103200;
data_array[4] = 0x15320900;
data_array[5] = 0x32AD05AD;
data_array[6] = 0x00000000;
data_array[7] = 0x0B0B0337;
data_array[8] = 0x00000037;
data_array[9] = 0x0000400C;
dsi_set_cmdq(data_array, 10, 1);
MDELAY(1);







data_array[0] = 0x002D3902;
data_array[1] = 0x181919D5;
data_array[2] = 0x1A1B1B18;
data_array[3] = 0x0201001A;
data_array[4] = 0x06050403;
data_array[5] = 0x18212007;
data_array[6] = 0x18181818;
data_array[7] = 0x18181818;
data_array[8] = 0x24181818;
data_array[9] = 0x18181825;
data_array[10] = 0x18181818;
data_array[11] = 0x18181818;
data_array[12] = 0x00000018;
dsi_set_cmdq(data_array, 13, 1);
MDELAY(1);

data_array[0] = 0x002D3902;
data_array[1] = 0x191818D6;
data_array[2] = 0x1A1B1B19;
data_array[3] = 0x0506071A;
data_array[4] = 0x01020304;
data_array[5] = 0x18242500;
data_array[6] = 0x18181818;
data_array[7] = 0x18181818;
data_array[8] = 0x21181818;
data_array[9] = 0x18181820;
data_array[10] = 0x18181818;
data_array[11] = 0x18181818;
data_array[12] = 0x00000018;
dsi_set_cmdq(data_array, 13, 1);
MDELAY(1);

data_array[0] = 0x00023902;
data_array[1] = 0x00000BCC;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00033902;
data_array[1] = 0x00311FC0;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00033902;
data_array[1] = 0x00A2A2B6;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);


data_array[0] = 0x00023902;
data_array[1] = 0x000002D4;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);


data_array[0] = 0x00023902;
data_array[1] = 0x000001BD;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00023902;
data_array[1] = 0x000000B1;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);


data_array[0] = 0x00023902;
data_array[1] = 0x000000BD;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);


data_array[0] = 0x00023902;
data_array[1] = 0x0000EDC6;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00023902;
data_array[1] = 0x000000DD;
dsi_set_cmdq(data_array, 2, 1);
MDELAY(1);

data_array[0] = 0x00110500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(150);

data_array[0] = 0x00290500;
dsi_set_cmdq(data_array, 1, 1);
MDELAY(20);

}
//#endif
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

    params->dsi.vertical_sync_active			=2;// 3;
    params->dsi.vertical_backporch				=16;// 14;
    params->dsi.vertical_frontporch				= 9;//16;
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active			= 50;//11;
    params->dsi.horizontal_backporch				= 50;//64;20
    params->dsi.horizontal_frontporch				= 50;//64;20
    params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

//    params->dsi.HS_TRAIL= 10;

    params->dsi.PLL_CLOCK=230;//234
/*
	params->dsi.cont_clock = 1;
	params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.ssc_disable = 1;	
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 1;
		params->dsi.lcm_esd_check_table[0].cmd  				= 0x0a;
		params->dsi.lcm_esd_check_table[0].count  			= 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 0;
	// enable tearing-free
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;*/
}

#define LCM_ID                      								(0x94)
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

unsigned int id1 = 0;
	unsigned int id2 = 0;
	unsigned char buffer[2];
	unsigned int array[16];  


 
	SET_RESET_PIN(1);
	 display_ldo18_enable(1);
	//SET_GPIO_LCD_ENP_LDO28(1);
	tps65132_set_vpos_volt(5800);
	tps65132_set_vneg_volt(5800);
	tps65132_vpos_enable(1);
	tps65132_vneg_enable(1);
	
	MDELAY(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(120);//Must over 6 ms

	array[0] = 0x00043902;
    	array[1] = 0x9483ffb9;
    	dsi_set_cmdq(array, 2, 1);
	MDELAY(10); 

	array[0] = 0x00023700;// return byte number
	dsi_set_cmdq(array, 1, 1);
	MDELAY(10);

	read_reg_v2(0xF4, buffer, 2);
	id1 = buffer[0]; //we only need ID
	id2 = buffer[1]; 

#ifdef BUILD_LK
	printf("[HX8394D]%s,  id = 0x%x\n", __func__, id1);
#else
	printk("[HX8394D]%s,  id = 0x%x\n", __func__, id1);
#endif

    return (LCM_ID == id1)?1:0;
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
	MDELAY(120);//250
	
//	lcm_compare_id();
    init_lcm_registers();

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
   MDELAY(10);
	SET_RESET_PIN(0);
    MDELAY(10);
	
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



LCM_DRIVER hx8394f_dsi_vdo_hd1440_hlt_55_hc_lcm_drv = 
{
    .name           = "hx8394f_dsi_vdo_hd1440_hlt_55_hc",
	#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "hx8394f",
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
