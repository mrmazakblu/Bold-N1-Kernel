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
#define FRAME_HEIGHT 										 (1440)

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

struct LCM_setting_table
{
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] =
{
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 10, {}},
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {} }
};

static struct LCM_setting_table lcm_initialization_setting[] =
{

{0xFF,3,{0x98,0x81,0x05}},

{0xB2,1,{0x70}},
{0x03,1,{0x00}},
{0x04,1,{0x24}},

{0x30,1,{0xF7}},
{0x29,1,{0x00}},
{0x2A,1,{0x12}},
{0x38,1,{0xA8}},
{0x1A,1,{0x50}},
{0x52,1,{0x5F}},
{0x54,1,{0x28}},
{0x55,1,{0x25}},

{0x26,1,{0x02}},
{0x3D,1,{0xA1}},
{0x1B,1,{0x01}},

{0xFF,3,{0x98,0x81,0x02}},
{0x42,1,{0x2F}},
{0x01,1,{0x50}},
{0x15,1,{0x10}},

{0x57,1,{0x00}},
{0x58,1,{0x17}},
{0x59,1,{0x26}},
{0x5A,1,{0x14}},
{0x5B,1,{0x17}},
{0x5C,1,{0x29}},
{0x5D,1,{0x1D}},
{0x5E,1,{0x1F}},
{0x5F,1,{0x8B}},
{0x60,1,{0x1E}},
{0x61,1,{0x2A}},
{0x62,1,{0x78}},
{0x63,1,{0x19}},
{0x64,1,{0x17}},
{0x65,1,{0x4B}},
{0x66,1,{0x20}},
{0x67,1,{0x27}},
{0x68,1,{0x4A}},
{0x69,1,{0x5A}},
{0x6A,1,{0x25}},
{0x6B,1,{0x00}},
{0x6C,1,{0x17}},
{0x6D,1,{0x26}},
{0x6E,1,{0x14}},
{0x6F,1,{0x17}},
{0x70,1,{0x29}},
{0x71,1,{0x1D}},
{0x72,1,{0x1F}},
{0x73,1,{0x8B}},
{0x74,1,{0x1E}},
{0x75,1,{0x2A}},
{0x76,1,{0x78}},
{0x77,1,{0x19}},
{0x78,1,{0x17}},
{0x79,1,{0x4B}},
{0x7A,1,{0x20}},
{0x7B,1,{0x27}},
{0x7C,1,{0x4A}},
{0x7D,1,{0x5A}},
{0x7E,1,{0x25}},

{0xFF,3,{0x98,0x81,0x01}},
{0x01,1,{0x00}},
{0x02,1,{0x00}},
{0x03,1,{0x56}},
{0x04,1,{0x13}},
{0x05,1,{0x13}},
{0x06,1,{0x0a}},
{0x07,1,{0x05}},
{0x08,1,{0x05}},
{0x09,1,{0x1D}},
{0x0a,1,{0x01}},
{0x0b,1,{0x00}},
{0x0c,1,{0x3F}},
{0x0d,1,{0x29}},
{0x0e,1,{0x29}},
{0x0f,1,{0x1D}},
{0x10,1,{0x1D}},
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x13,1,{0x08}},
{0x14,1,{0x08}},
{0x15,1,{0x00}},
{0x16,1,{0x00}},
{0x17,1,{0x00}},
{0x18,1,{0x00}},
{0x19,1,{0x00}},
{0x1a,1,{0x00}},
{0x1b,1,{0x00}},
{0x1c,1,{0x00}},
{0x1d,1,{0x00}},
{0x1e,1,{0x40}},
{0x1f,1,{0x88}},
{0x20,1,{0x08}},
{0x21,1,{0x01}},
{0x22,1,{0x00}},
{0x23,1,{0x00}},
{0x24,1,{0x00}},
{0x25,1,{0x00}},
{0x26,1,{0x00}},
{0x27,1,{0x00}},
{0x28,1,{0x33}},
{0x29,1,{0x03}},
{0x2a,1,{0x00}},
{0x2b,1,{0x00}},
{0x2c,1,{0x00}},
{0x2d,1,{0x00}},
{0x2e,1,{0x00}},
{0x2f,1,{0x00}},
{0x30,1,{0x00}},
{0x31,1,{0x00}},
{0x32,1,{0x00}},
{0x33,1,{0x00}},
{0x34,1,{0x00}},
{0x35,1,{0x00}},
{0x36,1,{0x00}},
{0x37,1,{0x00}},
{0x38,1,{0x00}},
{0x39,1,{0x0f}},
{0x3a,1,{0x2a}},
{0x3b,1,{0xc0}},
{0x3c,1,{0x00}},
{0x3d,1,{0x00}},
{0x3e,1,{0x00}},
{0x3f,1,{0x00}},
{0x40,1,{0x00}},
{0x41,1,{0xe0}},
{0x42,1,{0x40}},
{0x43,1,{0x0f}},
{0x44,1,{0x31}},
{0x45,1,{0xa8}},
{0x46,1,{0x00}},
{0x47,1,{0x08}},
{0x48,1,{0x00}},
{0x49,1,{0x00}},
{0x4a,1,{0x00}},
{0x4b,1,{0x00}},

{0x4c,1,{0xb2}},
{0x4d,1,{0x22}},
{0x4e,1,{0x01}},
{0x4f,1,{0xf7}},
{0x50,1,{0x29}},
{0x51,1,{0x72}},
{0x52,1,{0x25}},
{0x53,1,{0xb2}},
{0x54,1,{0x22}},
{0x55,1,{0x22}},
{0x56,1,{0x22}},

{0x57,1,{0xa2}},
{0x58,1,{0x22}},
{0x59,1,{0x01}},
{0x5a,1,{0xe6}},
{0x5b,1,{0x28}},
{0x5c,1,{0x62}},
{0x5d,1,{0x24}},
{0x5e,1,{0xa2}},
{0x5f,1,{0x22}},
{0x60,1,{0x22}},
{0x61,1,{0x22}},

{0x62,1,{0xee}},

{0x63,1,{0x02}},
{0x64,1,{0x0b}},
{0x65,1,{0x02}},
{0x66,1,{0x02}},
{0x67,1,{0x01}},
{0x68,1,{0x00}},
{0x69,1,{0x0f}},
{0x6a,1,{0x07}},
{0x6b,1,{0x55}},
{0x6c,1,{0x02}},
{0x6d,1,{0x02}},
{0x6e,1,{0x5b}},
{0x6f,1,{0x59}},
{0x70,1,{0x02}},
{0x71,1,{0x02}},
{0x72,1,{0x57}},
{0x73,1,{0x02}},
{0x74,1,{0x02}},
{0x75,1,{0x02}},
{0x76,1,{0x02}},
{0x77,1,{0x02}},
{0x78,1,{0x02}},

{0x79,1,{0x02}},
{0x7a,1,{0x0a}},
{0x7b,1,{0x02}},
{0x7c,1,{0x02}},
{0x7d,1,{0x01}},
{0x7e,1,{0x00}},
{0x7f,1,{0x0e}},
{0x80,1,{0x06}},
{0x81,1,{0x54}},
{0x82,1,{0x02}},
{0x83,1,{0x02}},
{0x84,1,{0x5a}},
{0x85,1,{0x58}},
{0x86,1,{0x02}},
{0x87,1,{0x02}},
{0x88,1,{0x56}},
{0x89,1,{0x02}},
{0x8a,1,{0x02}},
{0x8b,1,{0x02}},
{0x8c,1,{0x02}},
{0x8d,1,{0x02}},
{0x8e,1,{0x02}},

{0x8f,1,{0x44}},
{0x90,1,{0x44}},

{0xFF,3,{0x98,0x81,0x06}},
{0x01,1,{0x03}},
{0x04,1,{0x70}},
{0x2B,1,{0x0A}},

{0xC0,1,{0xCF}},
{0xC1,1,{0x2A}},

{0xFF,3,{0x98,0x81,0x00}},

{0x11, 1, {0x00}},
{REGFLAG_DELAY, 120, {}},
{0x29, 1, {0x00}},
{REGFLAG_DELAY, 20, {}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;
    printk("nt35695----tps6132-lcm_init   push_table++++++++++++++===============================devin----\n");
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

    params->dsi.vertical_sync_active     = 8;
    params->dsi.vertical_backporch       = 16;
    params->dsi.vertical_frontporch      = 16;
    params->dsi.vertical_active_line     = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active   = 20;
    params->dsi.horizontal_backporch     = 40;//36
    params->dsi.horizontal_frontporch    = 40;//78
    params->dsi.horizontal_active_pixel  = FRAME_WIDTH;
    /* params->dsi.ssc_disable = 1; */

    params->dsi.PLL_CLOCK                = 230;
}

static unsigned int lcm_compare_id(void)
{
#define LCM_ID 0x988110

    int array[4];
    char buffer[5];
    int id = 0;

    display_ldo18_enable(1);
	display_bias_vpos_set(5800);
	display_bias_vneg_set(5800);
	display_bias_vpos_enable(1);
	display_bias_vneg_enable(1);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(120);

    array[0]=0x00043902;
    array[1]=0x068198ff;
    dsi_set_cmdq(array, 2, 1);
    
    MDELAY(10);
    array[0] = 0x00083700;
    dsi_set_cmdq(array, 1, 1);
    
    MDELAY(10);
    read_reg_v2(0xf0, &buffer[0], 1);
    read_reg_v2(0xf1, &buffer[1], 1);
    read_reg_v2(0xf2, &buffer[2], 1);
    
    id = (buffer[0]<<16) | (buffer[1]<<8) | buffer[2];

#ifdef BUILD_LK
    printf("cjx: ili9881a %s %d, id = 0x%x\n", __func__,__LINE__, id);
#else
    printk("cjx: ili9881a %s %d, id = 0x%x\n", __func__,__LINE__, id);
#endif

	return (LCM_ID == id)?1:0;
}

static void lcm_init(void)
{
    LCM_LOGI("cjx : lcm_init +++++++++++++++++++++++++++++\n");
    
    display_ldo18_enable(1);
	display_bias_vpos_set(5800);
	display_bias_vneg_set(5800);
	display_bias_vpos_enable(1);
	display_bias_vneg_enable(1);
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
	display_bias_vpos_enable(0);
	display_bias_vneg_enable(0);
}

static void lcm_resume(void)
{
    lcm_init();
}

LCM_DRIVER ili9881p_hd1440_dsi_vdo_panda_hlt_lcm_drv =
{
    .name			= "ili9881p_hd1440_dsi_vdo_panda_hlt",
#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "ili9881p_panda_hlt",
		.vendor	= "unknow",
		.id		= "0x988110",
		.more	= "1440*720",
	},
#endif
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
};
