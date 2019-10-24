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
//#include <mach/gpio_const.h>

#ifdef BUILD_LK
#define LCM_LOGI(fmt, args...)  printk(KERN_INFO  " LCM file=%s: %s: line=%d: "fmt"\n", __FILE__,__func__,  __LINE__,##args)
#define LCM_LOGD(fmt, args...)  printk(KERN_DEBUG " LCM file=%s: %s: line=%d: "fmt"\n", __FILE__,__func__,  __LINE__,##args)
#define LCM_ENTER() printk(KERN_DEBUG " LCM file=%s: %s: line=%d: Enter------->\n", __FILE__,__func__, __LINE__)
#define LCM_EXIT()  printk(KERN_DEBUG " LCM file=%s: %s: line=%d: Exit<-------\n",  __FILE__,__func__, __LINE__)

#else
#define LCM_LOGI(fmt, args...)  printk(KERN_INFO " LCM :"fmt"\n", ##args)
#define LCM_LOGD(fmt, args...)  printk(KERN_DEBUG " LCM :"fmt"\n", ##args)
#define LCM_ENTER() 
#define LCM_EXIT()  

#endif


#define I2C_I2C_LCD_BIAS_CHANNEL 0
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)			(lcm_util.set_reset_pin((v)))
#define MDELAY(n)					(lcm_util.mdelay(n))

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
#define FRAME_WIDTH  										1080
#define FRAME_HEIGHT 										2246
#define LCM_PHYSICAL_WIDTH                  				(64800)
#define LCM_PHYSICAL_HEIGHT                  				(129600)

#define REGFLAG_DELAY             							 0xFFFA
#define REGFLAG_UDELAY             							 0xFFFB
#define REGFLAG_PORT_SWAP									 0xFFFC
#define REGFLAG_END_OF_TABLE      							 0xFFFD   // END OF REGISTERS MARKER

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
{0x00,1,{0x00}},                                                                      
{0xff,3,{0x87,0x16,0x01}},                                                            
                                                                                        
{0x00,1,{0x80}},                                                                      
{0xff,2,{0x87,0x16}},                                                                
                                                                                        
//========================================                                              
//Resolution 1080x2246	                                                                
{0x00,1,{0x00}},                                                                      
{0x2A,4,{0x00,0x00,0x04,0x37}}, // 1080x2246                                          
                                                                                        
{0x00,1,{0x00}},                                                                      
{0x2B,4,{0x00,0x00,0x08,0xC5}}, // 1080x2246                                          
                                                                                        
                                                                                        
//========================================                                              
//TCON Initial Code	                                                                    
{0x00,1,{0x80}}, //		                                                                
{0xC0,15,{0x00,0x74,0x00,0x10,0x10,0x00,0x74,0x10,0x10,0x00,0x74,0x00,0x10,0x10,0x00}},

{0x00,1,{0xA0}}, //	CKH cmd mode					            
{0xC0,7,{0x00,0x02,0x02,0x09,0x01,0x16,0x07}}, 	    
		                       
{0x00,1,{0xD0}}, //	CKH vdo mode					            
{0xC0,7,{0x00,0x02,0x02,0x09,0x01,0x16,0x07}},      
                       
{0x00,1,{0x82}}, //	CKH Vblank                    
{0xA5,3,{0x33,0x02,0x0C}},                     
                       
{0x00,1,{0x87}}, //	CKH tp term                    
{0xA5,4,{0x00,0x07,0x77,0x77}}, 

//========================================				
//LTPS Initial Code	
{0x00,1,{0x80}}, // VST1	
{0xC2,4,{0x84,0x02,0x35,0xBF}}, 	
{0x00,1,{0xE0}}, // VEND1						
{0xC2,4,{0x80,0x01,0x08,0x08}},
{0x00,1,{0xF4}}, // RST 
{0xC2,4,{0x88,0x05,0xCB,0x0A,0xEA}}, // = ~AUO RST  (5+2246+3=2254) 8CE	
{0x00,1,{0xB0}}, // CKV 123
{0xC2,15,{0x84,0x01,0x00,0x06,0x84, 0x83,0x02,0x00,0x06,0x84, 0x82,0x03,0x00,0x06,0x84}}, 	
{0x00,1,{0xC0}}, // CKV 4
{0xC2,5,{0x81,0x04,0x00,0x06,0x84}}, 
{0x00,1,{0xDA}}, // CKV Period
{0xC2,2,{0x33,0x33	}}, 
{0x00,1,{0xAA}}, //	CKV TP term					
{0xC3,2,{0x39,0x9C	}},		
{0x00,1,{0xD0}}, //	GOFF 1  // AUO GOFF O E					
{0xC3,13,{0x00,0x0A,0x0A,0x00,0x00,0x00,0x00, 0x01,0x04,0x00,0x5A,0x00,0x00}},
{0x00,1,{0xE0}}, //	GOFF 2	// AUO TPSW				
{0xC3,13,{0x00,0x0A,0x0A,0x00,0x00,0x00,0x00, 0x01,0x04,0x00,0x5A,0x00,0x00}},

//CE80=0x25 touch function enable
//TP control setting(term1/2/3/4)
{0x00,1,{0x80}},
{0xCE,9,{0x25,0x01,0x08,0x01,0x0C,0xFF,0x00,0x20,0x05}}, //8¿Ó
// TP term1/2/3/4 widths control
{0x00,1,{0x90}},
{0xCE,8,{0x00,0x5C,0x0B,0x75,0x00,0x5C,0x00,0x12}},

//===============================================
//DummyTerm_20171120
{0x00,1,{0xB0}},                
{0xCE,3,{0x04,0x00,0x72}},       
{0x00,1,{0x9B}},                   
{0xCE,3,{0x01,0x16,0x07}},         

//========================================
//PanelIF Initial Code	
{0x00,1,{0x80}},	// U 2 D	CC80	
{0xCC,12,{0x02,	0x24,	0x10,	0x01,	0x06,	0x07,	0x08,	0x09,	0x20,	0x21,	0x24,	0x24}},
{0x00,1,{0x90}},	// D 2 U	CC90	
{0xCC,12,{0x10,	0x24,	0x02,	0x01,	0x09,	0x08,	0x07,	0x06,	0x20,	0x21,	0x24,	0x24}},
{0x00,1,{0xA0}},	// no dir 1	CCA0	
{0xCC,15,{0x1A,	0x1B,	0x1C,	0x1D,	0x1E,	0x1F,	0x18,	0x19,	0x14,	0x15,	0x24,	0x24,	0x24,	0x24,	0x24}},
{0x00,1,{0xB0}},	// no dir 2	CCB0	
{0xCC,5,{0x24,	0x24,	0x24,	0x24,	0x24}},
{0x00,1,{0x80}},	// slp in	CB80	
{0xCB,8,{0x00,	0x00,	0x00,	0xC0,	0x30,	0x0C,	0x03,	0x00}},
{0x00,1,{0x90}},	// power on 1	CB90	
{0xCB,15,{0x00,	0x00,	0x00,	0xC0,	0x00,	0x00,	0x00,	0x00,	0x00,	0xFF,	0x00,	0x00,	0x00,	0x00,	0x00}},
{0x00,1,{0xA0}},	// power on 2	CBA0	
{0xCB,15,{0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00}},
{0x00,1,{0xB0}},	// power on 3	CBB0	
{0xCB,2,{0x00,	0x00}},
{0x00,1,{0xC0}},	// power off 1	CBC0	
{0xCB,15,{0x15,	0x00,	0x15,	0x2A,	0x15,	0x15,	0x15,	0x15,	0x2A,	0xD5,	0x00,	0x00,	0x15,	0x15,	0x15}},
{0x00,1,{0xD0}},	// power off 2	CBD0	
{0xCB,15,{0x15,	0x15,	0x15,	0x55,	0x55,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00}},
{0x00,1,{0xE0}},	// power off 3	CBE0	
{0xCB,2,{0x00,	0x00}},
{0x00,1,{0xF0}},	// L V D	CBF0	
{0xCB,8,{0xF3,  0xFF,	0x0F,	0x3F,	0xF0,	0x0C,	0x00,	0x00}},
{0x00,1,{0x80}},	// CGOUT R 1	CD80	
{0xCD,15,{0x01,0x03,	0x13,	0x14,	0x04,	0x16,	0x16,	0x06,	0x08,	0x09,	0x0A,	0x22,	0x0D,	0x0D,	0x0E}},
{0x00,1,{0x90}},	// CGOUT R 2	CD90	
{0xCD,3,{0x0E,0x0F,0x0F}},

{0x00,1,{0xA0}},	// CGOUT L 1	CDA0	
{0xCD,15,{0x01,	0x03,	0x13,	0x14,	0x04,	0x16,	0x16,	0x05,	0x07,	0x09,	0x16,	0x0A,	0x0D,	0x0D,	0x0E}},

{0x00,1,{0xB0}},	// CGOUT L 2	CDB0	
{0xCD,3,{0x0E,	0x0F,	0x0F}},

{0x00,1,{0x80}},	// 	
{0xCF,6,{ 0x00,	0x05,	0x02, 0x00,	0x05,	0x02}},

{0x00,1,{0x81}},	// All gate on off	F381	
{0xF3,12,{	0xFF,	0xF3,	0xC0,	0xFF,	0xF7,	0xC0,	0x0,	0x2,	0x0,	0x0,	0x2,	0x0}},


// DC Power setting
{0x00,1,{0x80}},     
{0xc5,10,{0x00,0xc1,0xdd,0xc4,0x14,0x1e,0x00,0x55,0x50,0x03}}, //Í¬0x06 = Sx = 4.5V / 0x03 = 3.0

{0x00,1,{0x90}},     
{0xc5,10,{0x77,0x19,0x1E,0x00,0x88,0x01,0x32,0x52,0x55,0x50}}, //VGHO = +8.5V / VGLO = -8V / VGH = +11V / VGL = -13.2V 


{0x00,1,{0x00}},
{0xD9,5,{0x00,0xBC,0xBC,0xBC,0xBC}},	//VCOM = TBD

{0x00,1,{0x00}},
{0xD8,2,{0x29,0x29}},		//GVDD/NGVDD = TBD
	
//========================================
{0x00,1,{0x86}},// mipi tx clk div
{0xb0,1,{0x0b}},//

{0x00,1,{0x8C}},// vcom_en pwr on off 
{0xf5,2,{0x15,0x22}},//

{0x00,1,{0x88}},// CKH EQ gnd 
{0xc3,2,{0x11,0x11}},	

{0x00,1,{0x98}},// CKH EQ vsp vsn  
{0xc3,2,{0x11,0x11}},	


//========================================
{0x00,1,{0xa0}}, // panel size = 1080x2246
{0xb3,7,{0x03,0x04,0x38,0x08,0xC6, 0x00,0x50}},  

// Gamma
{0x00,1,{0x00}}, 
{0xe1,24,{0x0d,0x1b,0x34,0x48,0x52, 0x5f,0x6f,0x7c,0x80,0x87,0x8c, 0x91,0x6b,0x68,0x67,0x60,0x53, 0x48,0x39,0x30,0x28,0x1a,0x09, 0x08}},  
{0x00,1,{0xa0}}, 
{0xe2,24,{0x0d,0x1b,0x34,0x48,0x52, 0x5f,0x6f,0x7c,0x80,0x87,0x8c, 0x91,0x6b,0x68,0x67,0x60,0x53, 0x48,0x39,0x30,0x28,0x1a,0x11, 0x10}},  

	
// TP start&&count
{0x00,1,{0x94}},
{0xCF,4,{0x00,0x00,0x10,0x20}},
{0x00,1,{0xA4}},
{0xCF,4,{0x00,0x07,0x01,0x80}},
{0x00,1,{0xd0}},
{0xCF,1,{0x08}},

	{0x29,1,{0x00}},
	{REGFLAG_DELAY,15,{}},
	{0x11,1,{0x00}},


	{REGFLAG_DELAY, 120, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}    
              
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28, 1,{0x00} },
	{REGFLAG_DELAY, 120, {} },
	{0x10, 1,{0x00} },
	{REGFLAG_DELAY, 10, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

//#if 0
static void push_table(struct LCM_setting_table *table, unsigned int count,
		       unsigned char force_update)
{
	unsigned int i;
    LCM_ENTER();
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
	LCM_EXIT();
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
	
	params->physical_width = 70;//LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = 140;//LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um = 70200;//LCM_PHYSICAL_WIDTH;	= sqrt((size*25.4)^2/(18^2+9^2))*9*1000
	params->physical_height_um = 140400;//LCM_PHYSICAL_HEIGHT; = sqrt((size*25.4)^2/(18^2+9^2))*18*1000
	params->density = 480;//LCM_DENSITY;

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
	params->dsi.intermediat_buffer_num = 0;	//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
#endif

	// Video mode setting
	params->dsi.packet_size=256;

	params->dsi.vertical_sync_active				=  2;
	params->dsi.vertical_backporch					= 16;//16 25 30 35 12 8
	params->dsi.vertical_frontporch					= 32;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active = 14;
	params->dsi.horizontal_backporch = 32;//32
	params->dsi.horizontal_frontporch = 32;//78
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	/* params->dsi.ssc_disable                                                       = 1; */

	params->dsi.PLL_CLOCK = 580;//244;
	params->dsi.ssc_disable = 0;
	params->dsi.ssc_range = 1;
	params->dsi.cont_clock = 1;
	params->dsi.clk_lp_per_line_enable = 1;
	
	#if 1
	//params->dsi.ssc_disable = 1;
	params->dsi.lcm_ext_te_monitor = FALSE;
	
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
		params->dsi.lcm_esd_check_table[0].cmd  				= 0x0a;
		params->dsi.lcm_esd_check_table[0].count  			= 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
		
		params->dsi.lcm_esd_check_table[1].cmd  				= 0xac;
		params->dsi.lcm_esd_check_table[1].count  			= 1;
		params->dsi.lcm_esd_check_table[1].para_list[0] = 0x00;
		
		params->dsi.lcm_esd_check_table[2].cmd  				= 0x0d;
		params->dsi.lcm_esd_check_table[2].count  			= 1;
		params->dsi.lcm_esd_check_table[2].para_list[0] = 0x00;
	
		#endif

}


static unsigned int lcm_compare_id(void)
{
	
	unsigned char buffer[2];
	unsigned int array[16];  
	
	mt_dsi_pinctrl_set(LCM_RESET_PIN_NO, 1);
	
	MDELAY(20);//100

	mt_dsi_pinctrl_set(LCM_RESET_PIN_NO, 0);
	MDELAY(20);//100
	display_bias_enable();

	MDELAY(10);  

	mt_dsi_pinctrl_set(LCM_RESET_PIN_NO, 1);
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
	LCM_LOGI("%s, LK TDDI id = 0x%08x\n", __func__, buffer[0]);
   #else
	LCM_LOGI("%s, Kernel TDDI id = 0x%08x\n", __func__, buffer[0]);
   #endif

   return (0x77 == buffer[0])?1:0; 
	//return 1;//(LCM_ID_NT35532 == id)?1:0;


}
#ifndef BUILD_LK
extern atomic_t ESDCheck_byCPU;
#endif
static unsigned int lcm_ata_check(unsigned char *bufferr)
{
#ifndef BUILD_LK 
unsigned int ret = 0 ,ret1=2; 

unsigned char x0_MSB = 0x5;//((x0>>8)&0xFF); 
unsigned char x0_LSB = 0x2;//(x0&0xFF); 
unsigned char x1_MSB = 0x1;//((x1>>8)&0xFF); 
unsigned char x1_LSB = 0x4;//(x1&0xFF); 

unsigned int data_array[6]; 
unsigned char read_buf[4]; 
unsigned char read_buf1[4]; 
unsigned char read_buf2[4]; 
unsigned char read_buf3[4]; 

#ifdef BUILD_LK 
printf("ATA check size = 0x%x,0x%x,0x%x,0x%x\n",x0_MSB,x0_LSB,x1_MSB,x1_LSB); 
#else 
printk("ATA check size = 0x%x,0x%x,0x%x,0x%x\n",x0_MSB,x0_LSB,x1_MSB,x1_LSB); 
#endif 

//write page frist lhr

//data_array[0]= 0x0002150A;//HS packet 
//data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x51; 
//data_array[2]= (x1_LSB); 
//dsi_set_cmdq(data_array, 3, 1); 
   
data_array[0]= 0x0002390A;//HS packet 
data_array[1]= 0x00002453; 
//data_array[2]= (x1_LSB); 
dsi_set_cmdq(data_array, 2, 1); 
    
 data_array[0]= 0x0002390A;//HS packet 
data_array[1]= 0x0000F05e; 
dsi_set_cmdq(data_array, 2, 1); 

//data_array[0]= 0x0002390A;//HS packet 
//data_array[1]= 0x00000151; 
//dsi_set_cmdq(data_array, 2, 1); 

data_array[0]= 0x0002390A;//HS packet 
data_array[1]= 0x00000355; 
//data_array[2]= (x1_LSB); 
dsi_set_cmdq(data_array, 2, 1); 
 
//data_array[0]= 0x0002150A;//HS packet 
//data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x51; 
//data_array[2]= (x1_LSB); 
//dsi_set_cmdq(data_array, 3, 1); 
 
data_array[0] = 0x00013700; 
dsi_set_cmdq(data_array, 1, 1); 

//atomic_set(&PMaster_enable, 1);
//read_reg_v2(0X52, read_buf, 1); 
read_reg_v2(0X56, read_buf1, 1); 
read_reg_v2(0X54, read_buf2, 1); 
read_reg_v2(0X5F, read_buf3, 1);
//atomic_set(&PMaster_enable, 0);

if((read_buf1[0] == 0x03)&& (read_buf2[0] == 0x24) && (read_buf3[0] == 0xf0)) 
   ret = 1; 
else 
    ret = 0; 

#ifdef BUILD_LK 
printf("ATA read buf size = 0x%x,0x%x,0x%x,0x%x,ret= %d\n",read_buf[0],read_buf[1],read_buf[2],read_buf[3],ret); 
#else 
printk("ATA read buf  size = 0x%x,0x%x,0x%x,0x%x,ret= %d ret1= %d\n",read_buf[0],read_buf1[0],read_buf2[0],read_buf3[0],ret,ret1); 
printk("ATA read buf new  size = 0x%x,0x%x,0x%x,0x%x,ret= %d ret1= %d\n",read_buf1[0],read_buf1[1],read_buf1[2],read_buf1[3],ret,ret1); 
#endif 

return ret; 
#endif 
}

static void lcm_init(void)
{
	//SET_RESET_PIN(1);
	//MDELAY(100);
	display_ldo18_enable(1);
	mt_dsi_pinctrl_set(LCM_RESET_PIN_NO, 1);

	MDELAY(10);
	//lcm_set_bias();
	//MDELAY(10);
	mt_dsi_pinctrl_set(LCM_RESET_PIN_NO, 0);
	MDELAY(20);//100
	
	
    //mt_dsi_pinctrl_set(LCM_POWER_DP_NO, 1);
	display_bias_enable();
	mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 1);
	MDELAY(12);

	mt_dsi_pinctrl_set(LCM_RESET_PIN_NO, 1);
	MDELAY(120);//250
	

	push_table(lcm_initialization_setting,
		   sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	LCM_EXIT();
}

static void lcm_suspend(void)
{

   // unsigned int data_array[16];
    
	//data_array[0]=0x00280500; // Display Off
	//dsi_set_cmdq(data_array, 1, 1);
   // MDELAY(20);

	//data_array[0] = 0x00100500; // Sleep In
	//dsi_set_cmdq(data_array, 1, 1);
   // MDELAY(120);
	LCM_ENTER(); 
	push_table(lcm_suspend_setting,
		   sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
	//tps65132_avdd_en(FALSE);
	MDELAY(10);

	mt_dsi_pinctrl_set(LCM_RESET_PIN_NO, 0);
	MDELAY(1);
	mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 0);
	display_bias_disable();
	MDELAY(10);
	display_ldo18_enable(0);
	//printk("########################pzp lcm_suspend");
	//SET_RESET_PIN(0);
	//MDELAY(10);
	LCM_EXIT();
}

static void lcm_resume(void)
{
	lcm_init();
	
}

static void lcm_init_power(void)
{
	display_bias_enable();
	mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 1);
}

static void lcm_suspend_power(void)
{
	display_bias_disable();
	mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 0);
}

static void lcm_resume_power(void)
{
	mt_dsi_pinctrl_set(LCM_RESET_PIN_NO, 0);
	display_bias_enable();
	mt_dsi_pinctrl_set(LCM_POWER_DM_NO, 1);
}




LCM_DRIVER ft8716f_fhdp_dsi_vdo_auo618_lcm_drv= 
{
    .name			= "ft8716f_fhdp_dsi_vdo_auo618",
	#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "ft8716f",
		.vendor	= "Focaltech",
		.id		= "0x80",
		.more	= "1080*2246",
	},
	#endif
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.init_power = lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.resume         = lcm_resume,
	.compare_id     = lcm_compare_id,
	.ata_check 		= lcm_ata_check,
   
};
