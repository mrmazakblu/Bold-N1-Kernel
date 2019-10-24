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
#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#endif

#ifdef BUILD_LK
#define LCM_LOGI(string, args...)  dprintf(0, "[LK/"LOG_TAG"]"string, ##args)
#define LCM_LOGD(string, args...)  dprintf(1, "[LK/"LOG_TAG"]"string, ##args)
#else
#define LCM_LOGI(fmt, args...)  pr_notice("[KERNEL/"LOG_TAG"]"fmt, ##args)
#define LCM_LOGD(fmt, args...)  pr_debug("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif

static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v)            (lcm_util.set_reset_pin((v)))
#define MDELAY(n)                   (lcm_util.mdelay(n))

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)     lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)        lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                       lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)                   lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)                                        lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)                lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */
#define LCM_DSI_CMD_MODE	0
#define FRAME_WIDTH  										 (720)
#define FRAME_HEIGHT 										(1280)


#define REGFLAG_DELAY             							0x1FC
#define REGFLAG_END_OF_TABLE      							0xFFF   // END OF REGISTERS MARKER

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */

struct LCM_setting_table
{
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] =
{
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_initialization_setting[] = {
//**************************************************/		
//LCDD (Peripheral) Setting
		
//LD052HF1L02_JD9366D/

	{0xE0,01,{0x00}},
	//--- PASSWORD  ----//
	{0xE1,01,{0x93}},
	{0xE2,01,{0x66}},
	{0xE3,01,{0xF9}},
	{0x80,01,{0x03}},
	//{0x74,01,{0x6C}},

	//--- Page1  ----//
	{0xE0,01,{0x01}},
	{0x00,01,{0x01}},
	{0x01,01,{0x21}},//2c
	{0x03,01,{0x01}},
	{0x04,01,{0x2E}},
	{0x0A,01,{0x08}},
	{0x0C,01,{0x74}},
	{0x13,01,{0x00}},
	{0x14,01,{0x99}},	
	{0x15,01,{0x51}},
	{0x16,01,{0x51}},	
	{0x17,01,{0x00}},
	{0x18,01,{0xA4}},	
	{0x19,01,{0x00}},
	{0x1A,01,{0x00}},
	{0x1B,01,{0xA4}},  
	{0x1C,01,{0x00}},
	{0x1F,01,{0x2F}},	
	{0x20,01,{0x2D}},	
	{0x21,01,{0x19}},	
	{0x22,01,{0x0D}},	
	{0x23,01,{0x02}},	
	{0x24,01,{0xFE}},
	{0x26,01,{0xDD}},
	{0x35,01,{0x11}},	
	{0x37,01,{0x09}},
	{0x38,01,{0x04}},	
	{0x39,01,{0x0C}},	
	{0x3A,01,{0x18}},	
	{0x3C,01,{0x7C}},	
	//{0x3D,01,{0x18}},	
	//{0x3E,01,{0x80}},	
	//{0x3F,01,{0x4E}},		
	{0x3D,01,{0xFF}},	
	{0x3E,01,{0xFF}},	
	{0x3F,01,{0xFF}},               
	{0x4B,01,{0x04}},     

	{0x40,01,{0x04}},
	{0x41,01,{0xA0}},	
	{0x42,01,{0x7F}},	
	{0x43,01,{0x14}},
	{0x44,01,{0x0B}},	
	{0x45,01,{0x64}},	
	//{0x4A,01,{0x3A}},
	{0x55,01,{0x01}},	
	{0x56,01,{0x01}},
	{0x57,01,{0x6D}},	
	{0x58,01,{0x0A}},	
	{0x59,01,{0x8A}},	
	{0x5A,01,{0x28}},	
	{0x5B,01,{0x23}},
	{0x5C,01,{0x15}},	
	//31#gamma2.2
	{0x5D,01,{0x75}},
	{0x5E,01,{0x60}},
	{0x5F,01,{0x52}},
	{0x60,01,{0x46}},
	{0x61,01,{0x44}},
	{0x62,01,{0x35}},
	{0x63,01,{0x3A}},
	{0x64,01,{0x24}},
	{0x65,01,{0x3D}},
	{0x66,01,{0x3C}},
	{0x67,01,{0x3D}},
	{0x68,01,{0x5D}},
	{0x69,01,{0x4D}},
	{0x6A,01,{0x57}},
	{0x6B,01,{0x4B}},
	{0x6C,01,{0x4B}},
	{0x6D,01,{0x3E}},
	{0x6E,01,{0x2E}},
	{0x6F,01,{0x1A}},

	{0x70,01,{0x75}},
	{0x71,01,{0x60}},
	{0x72,01,{0x52}},
	{0x73,01,{0x46}},
	{0x74,01,{0x44}},
	{0x75,01,{0x35}},
	{0x76,01,{0x3A}},
	{0x77,01,{0x24}},
	{0x78,01,{0x3D}},
	{0x79,01,{0x3C}},
	{0x7A,01,{0x3D}},
	{0x7B,01,{0x5D}},
	{0x7C,01,{0x4D}},
	{0x7D,01,{0x57}},
	{0x7E,01,{0x4B}},
	{0x7F,01,{0x4B}},
	{0x80,01,{0x3E}},
	{0x81,01,{0x2E}},
	{0x82,01,{0x1A}},

	//Page2
	{0xE0,01,{0x02}},
	{0x00,01,{0x5D}},
	{0x01,01,{0x51}},
	{0x02,01,{0x5D}},
	{0x03,01,{0x5D}},
	{0x04,01,{0x5D}},
	{0x05,01,{0x5D}},
	{0x06,01,{0x5D}},
	{0x07,01,{0x41}},
	{0x08,01,{0x45}},
	{0x09,01,{0x5D}},
	{0x0A,01,{0x5D}},
	{0x0B,01,{0x4B}},
	{0x0C,01,{0x49}},
	{0x0D,01,{0x5D}},
	{0x0E,01,{0x5D}},
	{0x0F,01,{0x47}},
	{0x10,01,{0x5F}},
	{0x11,01,{0x5D}},
	{0x12,01,{0x5D}},
	{0x13,01,{0x5D}},
	{0x14,01,{0x5D}},
	{0x15,01,{0x5D}},
	                      
	{0x16,01,{0x5D}},
	{0x17,01,{0x50}},
	{0x18,01,{0x5D}},
	{0x19,01,{0x5D}},
	{0x1A,01,{0x5D}},
	{0x1B,01,{0x5D}},
	{0x1C,01,{0x5D}},
	{0x1D,01,{0x40}},
	{0x1E,01,{0x44}},
	{0x1F,01,{0x5D}},
	{0x20,01,{0x5D}},
	{0x21,01,{0x4A}},
	{0x22,01,{0x48}},
	{0x23,01,{0x5D}},
	{0x24,01,{0x5D}},
	{0x25,01,{0x46}},
	{0x26,01,{0xDD}},
	{0x27,01,{0x5D}},
	{0x28,01,{0x5D}},
	{0x29,01,{0x5D}},
	{0x2A,01,{0x5D}},
	{0x2B,01,{0x5D}},

	{0x2C,01,{0x1D}},
	{0x2D,01,{0x00}},
	{0x2E,01,{0x1D}},
	{0x2F,01,{0x1D}},
	{0x30,01,{0x1D}},
	{0x31,01,{0x1D}},
	{0x32,01,{0x1D}},
	{0x33,01,{0x10}},
	{0x34,01,{0x06}},
	{0x35,01,{0x1D}},
	{0x36,01,{0x1D}},
	{0x37,01,{0x08}},
	{0x38,01,{0x0A}},
	{0x39,01,{0x1D}},
	{0x3A,01,{0x1D}},
	{0x3B,01,{0x04}},
	{0x3C,01,{0x1F}},
	{0x3D,01,{0x1D}},
	{0x3E,01,{0x1D}},
	{0x3F,01,{0x1D}},
	{0x40,01,{0x1D}},
	{0x41,01,{0x1D}},

	{0x42,01,{0x1D}},
	{0x43,01,{0x01}},
	{0x44,01,{0x1D}},
	{0x45,01,{0x1D}},
	{0x46,01,{0x1D}},
	{0x47,01,{0x1D}},
	{0x48,01,{0x1D}},
	{0x49,01,{0x11}},
	{0x4A,01,{0x07}},
	{0x4B,01,{0x1D}},
	{0x4C,01,{0x1D}},
	{0x4D,01,{0x09}},
	{0x4E,01,{0x0B}},
	{0x4F,01,{0x1D}},
	{0x50,01,{0x1D}},
	{0x51,01,{0x05}},
	{0x52,01,{0x1F}},
	{0x53,01,{0x1D}},
	{0x54,01,{0x1D}},
	{0x55,01,{0x1D}},
	{0x56,01,{0x1D}},
	{0x57,01,{0x1D}},
	
	{0x58,01,{0x41}},
	{0x59,01,{0x00}},
	{0x5A,01,{0x00}},
	{0x5B,01,{0x10}},
	{0x5C,01,{0x02}},
	{0x5D,01,{0x60}},
	{0x5E,01,{0x01}},
	{0x5F,01,{0x02}},
	{0x60,01,{0x40}},
	{0x61,01,{0x03}},
	{0x62,01,{0x04}},
	{0x63,01,{0x59}},
	{0x64,01,{0x59}},
	{0x65,01,{0x75}},
	{0x66,01,{0x12}},
	{0x67,01,{0x74}},
	{0x68,01,{0x04}},
	{0x69,01,{0x61}},//59 59
	{0x6A,01,{0x60}},//63 5A
	{0x6B,01,{0x0A}},
	{0x6C,01,{0x00}},
	{0x6D,01,{0x0C}},
	{0x6E,01,{0x00}},
	{0x6F,01,{0x88}},
	{0x70,01,{0x00}},
	{0x71,01,{0x00}},
	{0x72,01,{0x06}},
	{0x73,01,{0x7B}},
	{0x74,01,{0x00}},
	{0x75,01,{0xBC}},
	{0x76,01,{0x00}},
	{0x77,01,{0x05}},
	{0x78,01,{0x34}},
	{0x79,01,{0x00}},
	{0x7A,01,{0x00}},
	{0x7B,01,{0x00}},
	{0x7C,01,{0x00}},
	{0x7D,01,{0x03}},
	{0x7E,01,{0x7B}},
	{0x95,01,{0x01}},

	//Page4
	{0xE0,01,{0x04}},
	{0x00,01,{0x02}},
	{0x02,01,{0x23}},
	{0x03,01,{0x8F}},
	//{0x2D,01,{0x03}},
	{0x09,01,{0x11}},
	{0x0E,01,{0x2A}},
	{0x9A,01,{0x01}},
	{0x9D,01,{0x15}},
	{0xA9,01,{0x01}},
	{0xAA,01,{0x68}},
	{0xAC,01,{0x11}},
	{0xAD,01,{0x0E}},
	{0xAE,01,{0x13}},

	//Page0
	{0xE0,01,{0x00}},    
	{0xE6,01,{0x02}},
	{0xE7,01,{0x06}},
	//{0x74,01,{0x60}},

	//Page0
	{0xE0,01,{0x00}},
	
	{0x11,1,{0x00}},	//sleep out
	{REGFLAG_DELAY,120,{}},
	{0x29,1,{0x00}},	//display on
	{REGFLAG_DELAY,10,{}},
	//{0x35,1,{0x00}},	//TE on
	{REGFLAG_DELAY,20,{}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}

};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;
    for (i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd)
        {
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

    params->type                         = LCM_TYPE_DSI;
    params->width                        = FRAME_WIDTH;
    params->height                       = FRAME_HEIGHT;

#ifndef BUILD_LK
    //5.2 18:9 59068 118136, 16:9 64754 115118
	params->physical_width               = 62;     //LCM_PHYSICAL_WIDTH/1000;
	params->physical_height              = 125;    //LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um            = 62476;  //LCM_PHYSICAL_WIDTH; = sqrt((size*25.4)^2/(18^2+9^2))*9*1000
	params->physical_height_um           = 124951; //LCM_PHYSICAL_HEIGHT; = sqrt((size*25.4)^2/(18^2+9^2))*18*1000
	params->density                      = 320;    //LCM_DENSITY;
#endif

    // enable tearing-free
    params->dbi.te_mode                  = LCM_DBI_TE_MODE_VSYNC_ONLY;
    params->dbi.te_edge_polarity         = LCM_POLARITY_RISING;

#if (LCM_DSI_CMD_MODE)
    params->dsi.mode                     = CMD_MODE;
    params->dsi.switch_mode              = SYNC_PULSE_VDO_MODE;
#else
    params->dsi.mode                     = SYNC_PULSE_VDO_MODE;//SYNC_EVENT_VDO_MODE;//BURST_VDO_MODE;////
#endif

    // DSI
    /* Command mode setting */
    //1 Three lane or Four lane
    params->dsi.LANE_NUM                 = LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order  = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq    = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding      = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format       = LCM_DSI_FORMAT_RGB888;

    params->dsi.PS                       = LCM_PACKED_PS_24BIT_RGB888;

#if (LCM_DSI_CMD_MODE)
    params->dsi.intermediat_buffer_num   = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
    params->dsi.word_count               = FRAME_WIDTH * 3; //DSI CMD mode need set these two bellow params, different to 6577
#else
    params->dsi.intermediat_buffer_num   = 2;	//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage
#endif

    // Video mode setting
    params->dsi.packet_size              = 256;

	params->dsi.vertical_sync_active				= 8;//4;//calm lcd noise
	params->dsi.vertical_backporch					= 16;//10;//calm lcd noise
	params->dsi.vertical_frontporch					= 16;//10;//calm lcd noise
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 50;//calm lcd noise
	params->dsi.horizontal_backporch				= 80;//30;//calm lcd noise
	params->dsi.horizontal_frontporch				= 80;//44;//calm lcd noise
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
    /* params->dsi.ssc_disable = 1; */

    params->dsi.PLL_CLOCK=215;//234
}

static unsigned int lcm_compare_id(void)
{
	 #define LCM_ID 0x9366
    
    int array[4];
    char buffer[5];
    int id = 0;

    display_ldo18_enable(1);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

    array[0] = 0x00023700; // read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);
    read_reg_v2(0x04, buffer, 2);

    id = (buffer[0] << 8) | buffer[1]; 

#ifdef BUILD_LK
    printf("cjx: jd9366d %s %d, id = 0x%08x\n", __func__,__LINE__, id);
#else
    printk("cjx: jd9366d %s %d, id = 0x%08x\n", __func__,__LINE__, id);
#endif

    return (id == LCM_ID) ? 1 : 0;

}

static void lcm_init(void)
{
    display_ldo18_enable(1);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);
    
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
    push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);

    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);
    display_ldo18_enable(0);

}

static void lcm_resume(void)
{
    lcm_init();
}

LCM_DRIVER jd9366_hd720_dsi_vdo_xm_lcm_drv = 
{
    .name           = "jd9366_hd720_dsi_vdo_xm",
#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "jd9366",
		.vendor	= "unknow",
		.id		= "0x9366",
		.more	= "1280*720",
	},
#endif
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
};
