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
    {REGFLAG_DELAY, 20, {}},
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {} }
};

static struct LCM_setting_table lcm_initialization_setting[] = {
{0xB9,3,{0xF1,0x12,0x83}},        //此代码表示Write_CMD 0xB9，Write_parameter 0xF1,0x12,0x83，以下类同
{0xBA,27,{0x33,0x81,0x05,0xF9,0x0E,0x0E,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x44,0x25,0x00,0x91,0x0A,0x00,0x00,0x02,0x4F,0x01,0x00,0x00,0x37}},
{0xB8,4,{0x75,0x22,0x20,0x03}},
{0xB3,10,{0x0C,0x10,0x05,0x05,0x03,0xFF,0x00,0x00,0x00,0x00}},
{0xC0,9,{0x73,0x73,0x50,0x50,0x00,0x00,0x08,0x70,0x00}},
{0xBC,1,{0x4F}},
{0xCC,1,{0x0B}},
{0xB4,1,{0x80}},
{0xB2,3,{0xF0,0x12,0xF0}},
{0xE3,14,{0x07,0x07,0x0B,0x0B,0x03,0x0B,0x00,0x00,0x00,0x00,0xFF,0x00,0xC0,0x10}},
{0xC1,12,{0x53,0x00,0x1E,0x1E,0x77,0xF1,0xFF,0xFF,0xCC,0xCC,0x77,0x77}},  //0X53 VGH=15,VGL=10,0x1E=4.7
{0xB5,2,{0x14,0x14}},
{0xB6,2,{0x46,0x46}},
{0xBF,3,{0x02,0x11,0x00}},
{0xE9,63,{0x08,0x00,0x06,0x05,0xAD,0xB0,0x80,0x12,0x31,0x23,0x4F,0x0A,0xb0,0x80,0x47,0x08,0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x66,0x44,0x22,0x00,0x08,0x88,0x8F,0x48,0x88,0x88,0x88,0x77,0x55,0x33,0x11,0x18,0x88,0x8F,0x58,0x88,0x88,0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{0xEA,61,{0x00,0x1A,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x11,0x33,0x55,0x77,0x58,0x88,0x88,0x1F,0x88,0x88,0x88,0x00,0x22,0x44,0x66,0x48,0x88,0x88,0x0F,0x88,0x88,0x88,0x23,0x20,0x00,0x00,0x54,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0xB0,0xB1,0x00,0x00,0x00,0x00}},
{0xE0,34,{0x00,0x3F,0x3F,0x30,0x3D,0x3F,0x5F,0x48,0x06,0x0B,0x0D,0x10,0x12,0x10,0x13,0x11,0x17,0x00,0x3F,0x3F,0x30,0x3D,0x3F,0x5F,0x48,0x06,0x0B,0x0D,0x10,0x12,0x10,0x13,0x11,0x17}},
{0xC7,1,{0xB0}},
{0xC8,2,{0x00,0x04}},

{0x11,1,{0x00}},
{REGFLAG_DELAY, 150, {}},
{0x29,1,{0x00}},
{REGFLAG_DELAY, 50, {}},
{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;
    LCM_LOGI("nt35695----tps6132-lcm_init   push_table++++++++++++++===============================devin----\n");
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

    params->dsi.vertical_sync_active     = 5;
    params->dsi.vertical_backporch       = 9;
    params->dsi.vertical_frontporch      = 9;
    params->dsi.vertical_active_line     = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active   = 10;
    params->dsi.horizontal_backporch     = 40;
    params->dsi.horizontal_frontporch    = 40;
    params->dsi.horizontal_active_pixel  = FRAME_WIDTH;
    /* params->dsi.ssc_disable = 1; */

    params->dsi.PLL_CLOCK=225;//234
}

static unsigned int lcm_compare_id(void)
{
    return 1;
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

LCM_DRIVER st7703_hd1440_dsi_vdo_xl_lcm_drv = 
{
    .name           = "st7703_hd1440_dsi_vdo_xl",
#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "ST7703_XL",
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
    .compare_id     = lcm_compare_id,
};
