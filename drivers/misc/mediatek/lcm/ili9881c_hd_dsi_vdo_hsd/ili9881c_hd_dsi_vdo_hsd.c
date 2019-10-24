#ifdef BUILD_LK
#include <debug.h>
#else
#include <linux/kernel.h>
#include <linux/string.h>
#if defined(BUILD_UBOOT)
#include <asm/arch/mt6577_gpio.h>
#else
//#include <mach/mt_gpio.h>
#endif
#endif
#include "lcm_drv.h"
#include "tps65132.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(720)
#define FRAME_HEIGHT 										(1440)
//prize-add wyq 20181126 fix display inch size incorrent shown in CPU-Z apk-start
#define LCM_PHYSICAL_WIDTH									(61880)
#define LCM_PHYSICAL_HEIGHT									(123770)
//prize-add wyq 20181126 fix display inch size incorrent shown in CPU-Z apk-end

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER

#define LCM_ID_ILI9881C	0x98811C

#define LCM_ID_ILI9881C_W3 0x07
#define LCM_ID_ILI9881C_W4 0x05

#define LCM_DSI_CMD_MODE									0

#ifndef TRUE
    #define   TRUE     1
#endif
 
#ifndef FALSE
    #define   FALSE    0
#endif

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
#define read_reg_v2(cmd, buffer, buffer_size)				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

 struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting[] = {
	{0xFF, 0x03, {0x98, 0x81, 0x03} },//GIP_1
	
	{0x01, 0x01, {0x00} },
	{0x02, 0x01, {0x00} },
	{0x03, 0x01, {0x73} },
	{0x04, 0x01, {0x00} },
	{0x05, 0x01, {0x00} },
	{0x06, 0x01, {0x08} },	  //0A
	{0x07, 0x01, {0x00} },
	{0x08, 0x01, {0x00} },
	{0x09, 0x01, {0x00} },
	{0x0a, 0x01, {0x00} },
	{0x0b, 0x01, {0x00} },
	{0x0c, 0x01, {0x0B} },
	{0x0d, 0x01, {0x00} },
	{0x0e, 0x01, {0x00} },
	{0x0f, 0x01, {0x26} },	 //45%
	{0x10, 0x01, {0x26} },	 //45% 
	{0x11, 0x01, {0x00} },
	{0x12, 0x01, {0x00} },
	{0x13, 0x01, {0x02} },
	{0x14, 0x01, {0x00} },
	{0x15, 0x01, {0x00} },
	{0x16, 0x01, {0x00} }, 
	{0x17, 0x01, {0x00} }, 
	{0x18, 0x01, {0x00} },
	{0x19, 0x01, {0x00} },
	{0x1a, 0x01, {0x00} },
	{0x1b, 0x01, {0x00} },
	{0x1c, 0x01, {0x00} },
	{0x1d, 0x01, {0x00} },
	{0x1e, 0x01, {0x40} },
	{0x1f, 0x01, {0xC0} },	
	{0x20, 0x01, {0x06} },	  //08
	{0x21, 0x01, {0x01} },	  //03
	{0x22, 0x01, {0x08} },	 
	{0x23, 0x01, {0x03} },
	{0x24, 0x01, {0x8C} },	
	{0x25, 0x01, {0x8C} }, 
	{0x26, 0x01, {0x00} },
	{0x27, 0x01, {0x00} },
	{0x28, 0x01, {0x33} }, 
	{0x29, 0x01, {0x03} },
	{0x2a, 0x01, {0x00} },
	{0x2b, 0x01, {0x00} },
	{0x2c, 0x01, {0x01} },
	{0x2d, 0x01, {0x01} },
	{0x2e, 0x01, {0x00} },
	{0x2f, 0x01, {0x00} },
	{0x30, 0x01, {0x00} },
	{0x31, 0x01, {0x00} },
	{0x32, 0x01, {0x31} },	  //03
	{0x33, 0x01, {0x00} },
	{0x34, 0x01, {0x23} },	  //43
	{0x35, 0x01, {0x00} },
	{0x36, 0x01, {0x03} },
	{0x37, 0x01, {0x00} },
	{0x38, 0x01, {0x00} },
	{0x39, 0x01, {0x00} },
	{0x3a, 0x01, {0x00} },
	{0x3b, 0x01, {0x00} },
	{0x3c, 0x01, {0x00} },
	{0x3d, 0x01, {0x00} },
	{0x3e, 0x01, {0x00} },
	{0x3f, 0x01, {0x00} },
	{0x40, 0x01, {0x00} },
	{0x41, 0x01, {0x00} },
	{0x42, 0x01, {0x00} },
	{0x43, 0x01, {0x08} },	 //00
	{0x44, 0x01, {0x00} },
	
	
	//GIP_2
	{0x50, 0x01, {0x01} },
	{0x51, 0x01, {0x23} },
	{0x52, 0x01, {0x45} },
	{0x53, 0x01, {0x67} },
	{0x54, 0x01, {0x89} },
	{0x55, 0x01, {0xab} },
	{0x56, 0x01, {0x01} },
	{0x57, 0x01, {0x23} },
	{0x58, 0x01, {0x45} },
	{0x59, 0x01, {0x67} },
	{0x5a, 0x01, {0x89} },
	{0x5b, 0x01, {0xab} },
	{0x5c, 0x01, {0xcd} },
	{0x5d, 0x01, {0xef} },
	
	//GIP_3
	{0x5e, 0x01, {0x11} },
	{0x5f, 0x01, {0x17} },	//GCL
	{0x60, 0x01, {0x00} },	//VSD
	{0x61, 0x01, {0x07} },	//STV3
	{0x62, 0x01, {0x06} },	//STV1
	{0x63, 0x01, {0x0e} },	//CKL1
	{0x64, 0x01, {0x0f} },	//CKL3
	{0x65, 0x01, {0x0c} },	//CKL5
	{0x66, 0x01, {0x0d} },	//CKL7
	{0x67, 0x01, {0x02} },
	{0x68, 0x01, {0x02} },
	{0x69, 0x01, {0x02} },
	{0x6a, 0x01, {0x02} },
	{0x6b, 0x01, {0x02} },
	{0x6c, 0x01, {0x02} },
	{0x6d, 0x01, {0x02} },
	{0x6e, 0x01, {0x02} },
	{0x6f, 0x01, {0x02} },
	{0x70, 0x01, {0x02} },	//VGL
	{0x71, 0x01, {0x02} },	//VGL	
	{0x72, 0x01, {0x02} },	//VGL
	{0x73, 0x01, {0x16} },	//GCH
	{0x74, 0x01, {0x01} },	//VDS
	
	{0x75, 0x01, {0x17} },
	{0x76, 0x01, {0x00} },
	{0x77, 0x01, {0x07} },
	{0x78, 0x01, {0x06} },
	{0x79, 0x01, {0x0e} },
	{0x7a, 0x01, {0x0f} },
	{0x7b, 0x01, {0x0c} },
	{0x7c, 0x01, {0x0d} },
	{0x7d, 0x01, {0x02} },
	{0x7e, 0x01, {0x02} },
	{0x7f, 0x01, {0x02} },
	{0x80, 0x01, {0x02} },
	{0x81, 0x01, {0x02} },
	{0x82, 0x01, {0x02} },
	{0x83, 0x01, {0x02} },
	{0x84, 0x01, {0x02} },
	{0x85, 0x01, {0x02} },
	{0x86, 0x01, {0x02} },
	{0x87, 0x01, {0x02} },
	{0x88, 0x01, {0x02} },
	{0x89, 0x01, {0x16} },
	{0x8A, 0x01, {0x01} },
	
	
	//CMD_Page 4
	{0xFF, 0x03, {0x98, 0x81, 0x04} },
		
	{0x6C, 0x01, {0x15} },
	{0x6E, 0x01, {0x1A} },				 //di_pwr_reg=0 VGH clamp 1A=>12.13V
	{0x6F, 0x01, {0xA5} },				 // reg vcl + pumping ratio VGH=2.5x VGL=-2.5x
	{0x8D, 0x01, {0x2A} },				 //VGL clamp -14V
	{0x87, 0x01, {0xBA} },
	{0x26, 0x01, {0x76} },			  
	{0xB2, 0x01, {0xD1} }, 
	{0x3A, 0x01, {0x24} },				 //power saving
	{0x35, 0x01, {0x1F} },
	{0xB5, 0x01, {0x07} }, 
	{0x33, 0x01, {0x14} },
	{0x38, 0x01, {0x01} },		
	{0x39, 0x01, {0x00} },	   
	
	//CMD_Page 1
	{0xFF, 0x03, {0x98, 0x81, 0x01} },
	
	{0x22, 0x01, {0x09} },		//BGR, 0x01, { SS
	{0x2E, 0x01, {0xF0} },				 //1440 GATE NL SEL 
	{0x31, 0x01, {0x00} },		//column inversion
	//{0x56, 0x01, {0x00} },		//enable VCoM 
	//{0x53, 0x01, {0x8D} },	//VCOM1
	//{0x55, 0x01, {0x93} },	//VCOM2
	{0x50, 0x01, {0xC9} },			//VREG1OUT=5.104V
	{0x51, 0x01, {0xC5} },			//VREG2OUT=-5.106V
	{0x60, 0x01, {0x14} },				 //SDT
	{0x61, 0x01, {0x00} }, 
	{0x62, 0x01, {0x20} }, 
	{0x63, 0x01, {0x10} }, 
	
	{0xA0, 0x01, {0x08} },		//VP255 Gamma P
	{0xA1, 0x01, {0x15} },		//VP251
	{0xA2, 0x01, {0x20} },	//VP247
	{0xA3, 0x01, {0x10} },		//VP243
	{0xA4, 0x01, {0x12} },				//VP239 
	{0xA5, 0x01, {0x24} },				 //VP231
	{0xA6, 0x01, {0x19} },				 //VP219
	{0xA7, 0x01, {0x1C} },				 //VP203
	{0xA8, 0x01, {0x74} },				 //VP175
	{0xA9, 0x01, {0x1B} },				 //VP144
	{0xAA, 0x01, {0x27} },				 //VP111
	{0xAB, 0x01, {0x70} },				 //VP80
	{0xAC, 0x01, {0x1E} },				 //VP52
	{0xAD, 0x01, {0x1E} },				 //VP36
	{0xAE, 0x01, {0x53} },				 //VP24
	{0xAF, 0x01, {0x27} },				 //VP16
	{0xB0, 0x01, {0x2B} },				//VP12
	{0xB1, 0x01, {0x53} },				 //VP8
	{0xB2, 0x01, {0x62} },				 //VP4
	{0xB3, 0x01, {0x39} },				 //VP0
	
	{0xC0, 0x01, {0x08} },		//VN255 GAMMA N
	{0xC1, 0x01, {0x15} },				 //VN251	 
	{0xC2, 0x01, {0x20} },				//VN247 	
	{0xC3, 0x01, {0x10} },				 //VN243	 
	{0xC4, 0x01, {0x12} },				//VN239 	
	{0xC5, 0x01, {0x24} },				 //VN231	 
	{0xC6, 0x01, {0x19} },			   //VN219	   
	{0xC7, 0x01, {0x1C} },			   //VN203	   
	{0xC8, 0x01, {0x74} },				 //VN175	 
	{0xC9, 0x01, {0x1B} },				 //VN144	 
	{0xCA, 0x01, {0x27} },				//VN111 	
	{0xCB, 0x01, {0x70} },				 //VN80 	 
	{0xCC, 0x01, {0x1E} },				 //VN52 	 
	{0xCD, 0x01, {0x1E} },				 //VN36 	 
	{0xCE, 0x01, {0x53} },				//VN24		
	{0xCF, 0x01, {0x27} },				//VN16		
	{0xD0, 0x01, {0x2B} },				 //VN12 	 
	{0xD1, 0x01, {0x53} },				//VN8		
	{0xD2, 0x01, {0x62} },				 //VN4		 
	{0xD3, 0x01, {0x39} },				 //VN0	
	
	{0xFF, 0x03, {0x98, 0x81, 0x00} },

	{0x11, 0x01, {0x00} },
	{REGFLAG_DELAY, 120, {} },
	{0x29, 0x01, {0x00} },
	{REGFLAG_DELAY, 20, {} },
	{0x35, 0x01, {0x00} } //TE on
};

/*

static struct LCM_setting_table lcm_set_window[] = {
	{0x2A,	4,	{0x00, 0x00, (FRAME_WIDTH>>8), (FRAME_WIDTH&0xFF)}},
	{0x2B,	4,	{0x00, 0x00, (FRAME_HEIGHT>>8), (FRAME_HEIGHT&0xFF)}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

*/

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
	{0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	// Display off sequence
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},

    // Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

/*

static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;
		//prize-add wyq 20181126 fix display inch size incorrent shown in CPU-Z apk-start		
		params->physical_width = LCM_PHYSICAL_WIDTH/1000;
		params->physical_height = LCM_PHYSICAL_HEIGHT/1000;
		params->physical_width_um = LCM_PHYSICAL_WIDTH;
		params->physical_height_um = LCM_PHYSICAL_HEIGHT;
		//prize-add wyq 20181126 fix display inch size incorrent shown in CPU-Z apk-end
		
		// enable tearing-free
		params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

#if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
#else
		params->dsi.mode   = BURST_VDO_MODE;
#endif
	
		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		// Not support in MT6573
		params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.intermediat_buffer_num = 2;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
                /*d.j modify 2015.02.13*/
		params->dsi.vertical_sync_active				= 4;
		params->dsi.vertical_backporch					= 20;
		params->dsi.vertical_frontporch					= 16;
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 50;
		params->dsi.horizontal_backporch				= 90;
		params->dsi.horizontal_frontporch				= 70;
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

        params->dsi.HS_TRAIL							= 25;//add pzp
                params->dsi.PLL_CLOCK = 220; //this value must be in MTK suggested table
		// Bit rate calculation
//		params->dsi.pll_div1=29;		// fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
//		params->dsi.pll_div2=1; 		// div2=0~15: fout=fvo/(2*div2)

		/* ESD or noise interference recovery For video mode LCM only. */ // Send TE packet to LCM in a period of n frames and check the response. 
		params->dsi.lcm_int_te_monitor = FALSE; 
		params->dsi.lcm_int_te_period = 1; // Unit : frames 
 
		// Need longer FP for more opportunity to do int. TE monitor applicably. 
		if(params->dsi.lcm_int_te_monitor) 
			params->dsi.vertical_frontporch *= 2; 
 
		// Monitor external TE (or named VSYNC) from LCM once per 2 sec. (LCM VSYNC must be wired to baseband TE pin.) 
		params->dsi.lcm_ext_te_monitor = FALSE; 
		// Non-continuous clock 
		params->dsi.noncont_clock = TRUE; 
		params->dsi.noncont_clock_period = 2; // Unit : frames
		
		params->dsi.esd_check_enable = 1;
		params->dsi.customization_esd_check_enable = 1;
		
		params->dsi.lcm_esd_check_table[0].cmd			= 0x0A;
		params->dsi.lcm_esd_check_table[0].count		= 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
		/*prize-add-for round corner-houjian-20180918-start*/
		#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
			params->corner_pattern_width = 32;
			params->corner_pattern_height = 32;
		#endif
		/*prize-add-for round corner-houjian-20180918-end*/
}


static void lcm_power_sequence_on(void)
{
	
	tps65132_avdd_en(TRUE);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(5);
    SET_RESET_PIN(0);
	MDELAY(5);
    SET_RESET_PIN(1);
    MDELAY(120);

	
}

#if 0

static void lcm_power_sequence_off(void)
{
	
	 tps65132_avdd_en(FALSE);
	 SET_RESET_PIN(0);

}
#endif


static struct LCM_setting_table lcm_esd_toggle_p[] = {
 {0xD5,8, {0x19,0x19,0x18,0x18,0x18,0x18,0x19,0x19}},
 {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_esd_toggle_n[] = {
 {0xD5,8, {0x19,0x19,0x18,0x18,0x19,0x19,0x18,0x18}},
 {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static int toggle = 0;
void lcm_toggle(void)
{
	if (toggle)
	{
		toggle = 0;
		printk("steven1 kernel----------------%s------------enter\n",__func__);
		push_table(lcm_esd_toggle_n, sizeof(lcm_esd_toggle_n) / sizeof(struct LCM_setting_table), 1);
	}
	else
	{
		toggle = 1;
		printk("steven1 kernel----------------%s------------enter\n",__func__);
		push_table(lcm_esd_toggle_p, sizeof(lcm_esd_toggle_p) / sizeof(struct LCM_setting_table), 1);
	}
		
}
EXPORT_SYMBOL(lcm_toggle);

static void lcm_init(void)
{  
    printk("steven1 kernel----------------%s------------enter\n",__func__);
	lcm_power_sequence_on();
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	printk("steven1 kernel----------------%s------------exit\n",__func__);
}


static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);
	//lcm_power_sequence_off();

}

//extern int lcd_status;
//static int n_recovert=0;

static void lcm_resume(void)
{
    //lcm_power_sequence_on();
	push_table(lcm_sleep_out_setting, sizeof(lcm_sleep_out_setting) / sizeof(struct LCM_setting_table), 1);
}

/*
static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}


static void lcm_setbacklight(unsigned int level)
{
	unsigned int default_level = 145;
	unsigned int mapped_level = 0;

	//for LGE backlight IC mapping table
	if(level > 255) 
			level = 255;

	if(level >0) 
			mapped_level = default_level+(level)*(255-default_level)/(255);
	else
			mapped_level=0;

	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = mapped_level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}

*/


static unsigned int lcm_compare_id(void)
{
    int array[4];
    char buffer[4]={0,0,0,0};
    char id_high=0;
    char id_mid=0;
    char id_low=0;
    int id=0;
#if 0
    int sub_id=0;
#endif
    SET_RESET_PIN(1);
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(1);
    SET_RESET_PIN(1);
    MDELAY(120);
#if 0
	array[0] = 0x00083700;
	dsi_set_cmdq(array, 1, 1);	
	MDELAY(10);
	read_reg_v2(0xDA, buffer, 4);//    NC 0x00	0x98 0x16
	sub_id = buffer[0];
	#ifdef BUILD_LK
	dprintf(CRITICAL, "%s, LK debug: ili9881c sub_id = 0x%02x\n", __func__, sub_id);
	#else
	//printk("%s: ili9881c sub_id = 0x%02x\n", __func__, sub_id);
	#endif
#endif
    array[0]=0x00043902;
    array[1]=0x018198ff;
    dsi_set_cmdq(array, 2, 1);

    MDELAY(10);
    array[0] = 0x00083700;
    dsi_set_cmdq(array, 1, 1);

    MDELAY(10);
    read_reg_v2(0x00, &buffer[0], 1);//    NC 0x00  0x98 0x16
    MDELAY(10);
    read_reg_v2(0x01, &buffer[1], 1);//    NC 0x00  0x98 0x16
    MDELAY(10);
    read_reg_v2(0x02, &buffer[2], 1);//    NC 0x00  0x98 0x16

    id_high = buffer[0];
    id_mid = buffer[1];
    id_low = buffer[2];
    id = (id_high<<16) | (id_mid<<8) |id_low;
	
	#ifdef BUILD_LK
	//dprintf(CRITICAL, "%s, LK debug: ili9881c id = 0x%08x  %x\n", __func__, id, buffer[3]);
	dprintf(CRITICAL, "%s, LK debug: ili9881c id = 0x%08x\n", __func__, id);
	#else
	//printk("%s: ili9881c id = 0x%08x \n", __func__, id);
	#endif

	//return ((LCM_ID_ILI9881C == id) && (sub_id == LCM_ID_ILI9881C_W4))?1:0;
	return (LCM_ID_ILI9881C == id)?1:0;
	//return 1;
}
#ifndef BUILD_LK
extern atomic_t ESDCheck_byCPU;
#endif
static unsigned int lcm_ata_check(unsigned char *buffer)
{
	 #ifndef BUILD_LK
unsigned int ret = 0 ,ret1=2; 
//unsigned int x0 = FRAME_WIDTH/4; 
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
data_array[0]= 0x00043902; 
data_array[1]= 0x008198ff; 
dsi_set_cmdq(data_array, 2, 1); 

//write page frist lhr
data_array[0]= 0x0002390A;//HS packet 
data_array[1]= 0x00002453; 
dsi_set_cmdq(data_array, 2, 1); 
 data_array[0]= 0x0002390A;//HS packet 
data_array[1]= 0x00000F5e; 
dsi_set_cmdq(data_array, 2, 1); 
data_array[0]= 0x0002390A;//HS packet 
data_array[1]= 0x00000355; 
dsi_set_cmdq(data_array, 2, 1); 
data_array[0] = 0x00013700; 
dsi_set_cmdq(data_array, 1, 1); 

	atomic_set(&ESDCheck_byCPU, 1);
//read_reg_v2(0X52, read_buf, 1); 
read_reg_v2(0X56, read_buf1, 1); 
read_reg_v2(0X54, read_buf2, 1); 
read_reg_v2(0X5F, read_buf3, 1);
	atomic_set(&ESDCheck_byCPU, 0);

if((read_buf1[0] == 0x03)&& (read_buf2[0] == 0x24) && (read_buf3[0] == 0x0f)) 
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


LCM_DRIVER ili9881c_hd_dsi_vdo_hsd_lcm_drv = {
	.name		= "ili9881c_hd_dsi_vdo_hsd_drv",
    	//prize-lixuefeng-20150512-start
	#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "ili9881c",
		.vendor	= "unknow",
		.id		= "0x98811C",
		.more	= "1280*720",
	},
	#endif
	//prize-lixuefeng-20150512-end	
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
	.compare_id    = lcm_compare_id,	
	.ata_check		= lcm_ata_check,
#if (LCM_DSI_CMD_MODE)
	.set_backlight	= lcm_setbacklight,
    .update         = lcm_update,
#endif
};

