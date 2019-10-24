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

#ifndef BUILD_LK
	extern int tps65132_set_vpos_volt(int);
	extern int tps65132_set_vneg_volt(int);
	extern int tps65132_vpos_enable(bool);
	extern int tps65132_vneg_enable(bool);
	#define SET_GPIO_LCD_ENP_LDO18_PIN(v)        (lcm_util.set_gpio_lcd_enp_ldo18((v)))
	#define SET_GPIO_LCD_ENP_LDO28_PIN(v)
#else	//BUILD_LK
	#if defined(GPIO_LCD_BIAS_ENP_PIN)	//define in dws
		#define GPIO_LCD_BIAS_ENP(v)	\
			do{	\
				if (v){	\
					mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00); 	\
					mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);   \
					mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);   \
				}else{	\
					mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00); 	\
					mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);   \
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
					mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);   \
					mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);   \
				}else{	\
					mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00); 	\
					mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);   \
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
					mt_set_gpio_out(GPIO_LCD_LDO18_PIN, GPIO_OUT_ZERO);     \
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
					mt_set_gpio_out(GPIO_LCD_LDO28_PIN, GPIO_OUT_ZERO);	    \
				}	\
			}while(0);
	#else
		#define SET_GPIO_LCD_ENP_LDO28_PIN(v) 
	#endif
#endif

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
#define LCM_ID_ILI9881C 0x0098810d

/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */
#ifndef BUILD_LK
#define SET_RESET_PIN(v)                                     (lcm_util.set_reset_pin((v)))

#define UDELAY(n)                                            (lcm_util.udelay(n))
#define MDELAY(n)                                            (lcm_util.mdelay(n))

#define  SET_GPIO_LCD_ENP_LDO18(v)                           (lcm_util.set_gpio_lcd_enp_ldo18((v)))
#define  SET_GPIO_LCD_ENP_LDO28(v)                           (lcm_util.set_gpio_lcd_enp_ldo28((v)))

extern int tps65132_vpos_enable(bool);
extern int tps65132_vneg_enable(bool);
extern int tps65132_set_vpos_volt(int);
extern int tps65132_set_vneg_volt(int);
#endif

struct LCM_setting_table
{
    unsigned int cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_suspend_setting[] =
{
    {0x28, 1, {0x00}},
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {} }
};

static struct LCM_setting_table lcm_initialization_setting[] =
{
{0xFF,3,{0x98,0x81,0x03}},
{0x01,1,{0x00}},
{0x02,1,{0x00}},
{0x03,1,{0x73}},
{0x04,1,{0x00}},
{0x05,1,{0x00}},
{0x06,1,{0x08}},            //0A
{0x07,1,{0x00}},
{0x08,1,{0x00}},
{0x09,1,{0x00}},
{0x0a,1,{0x00}},
{0x0b,1,{0x00}},
{0x0c,1,{0x0B}},
{0x0d,1,{0x00}},
{0x0e,1,{0x00}},
{0x0f,1,{0x26}},   //45%
{0x10,1,{0x26}},   //45% 
{0x11,1,{0x00}},
{0x12,1,{0x00}},
{0x13,1,{0x02}},
{0x14,1,{0x00}},
{0x15,1,{0x00}},           //08
{0x16,1,{0x00}},           //08
{0x17,1,{0x00}},  
{0x18,1,{0x00}},           //08
{0x19,1,{0x00}},
{0x1a,1,{0x00}},
{0x1b,1,{0x00}},
{0x1c,1,{0x00}},
{0x1d,1,{0x00}},
{0x1e,1,{0x40}},
{0x1f,1,{0xC0}},  
{0x20,1,{0x06}},           //08
{0x21,1,{0x01}},           //03
{0x22,1,{0x08}},   
{0x23,1,{0x03}},
{0x24,1,{0x8C}},  
{0x25,1,{0x8C}}, 
{0x26,1,{0x00}},
{0x27,1,{0x00}},
{0x28,1,{0x33}}, 
{0x29,1,{0x03}},
{0x2a,1,{0x00}},
{0x2b,1,{0x00}},
{0x2c,1,{0x01}},
{0x2d,1,{0x01}},
{0x2e,1,{0x00}},
{0x2f,1,{0x00}},
{0x30,1,{0x00}},
{0x31,1,{0x00}},
{0x32,1,{0x31}},           //03
{0x33,1,{0x00}},
{0x34,1,{0x23}},           //43
{0x35,1,{0x00}},
{0x36,1,{0x03}},
{0x37,1,{0x00}},
{0x38,1,{0x00}},
{0x39,1,{0x35}},
{0x3A,1,{0x01}},
{0x3B,1,{0x40}},
{0x3C,1,{0x00}},
{0x3D,1,{0x01}},
{0x3E,1,{0x00}},
{0x3F,1,{0x00}},
{0x40,1,{0x35}},
{0x41,1,{0x88}},
{0x42,1,{0x00}},
{0x43,1,{0x40}},           //00
{0x44,1,{0x3F}},     //1F TO 3F_ RESET KEEP LOW ALL GATE ON
{0x45,1,{0x20}},     //LVD觸發後ALL GATE ON 至VGH
{0x46,1,{0x00}},


//GIP_2
{0x50,1,{0x01}},
{0x51,1,{0x23}},
{0x52,1,{0x45}},
{0x53,1,{0x67}},
{0x54,1,{0x89}},
{0x55,1,{0xab}},
{0x56,1,{0x01}},
{0x57,1,{0x23}},
{0x58,1,{0x45}},
{0x59,1,{0x67}},
{0x5a,1,{0x89}},
{0x5b,1,{0xab}},
{0x5c,1,{0xcd}},
{0x5d,1,{0xef}},

//GIP_3
{0x5e,1,{0x11}},
{0x5f,1,{0x17}},  //GCL
{0x60,1,{0x00}},  //VSD
{0x61,1,{0x07}},  //STV3
{0x62,1,{0x06}},  //STV1
{0x63,1,{0x0E}},  //CKL1           //0C
{0x64,1,{0x0F}},  //CKL3           //0D
{0x65,1,{0x0C}},  //CKL5           //0E
{0x66,1,{0x0D}},  //CKL7           //0F
{0x67,1,{0x02}},
{0x68,1,{0x02}},
{0x69,1,{0x02}},
{0x6a,1,{0x02}},
{0x6b,1,{0x02}},
{0x6c,1,{0x02}},
{0x6d,1,{0x02}},
{0x6e,1,{0x02}},
{0x6f,1,{0x02}},
{0x70,1,{0x02}},  //VGL
{0x71,1,{0x02}},  //VGL   
{0x72,1,{0x02}},  //VGL
{0x73,1,{0x16}},  //GCH
{0x74,1,{0x01}},  //VDS

{0x75,1,{0x17}},
{0x76,1,{0x00}},
{0x77,1,{0x07}},
{0x78,1,{0x06}},
{0x79,1,{0x0E}},           //0C
{0x7a,1,{0x0F}},           //0D
{0x7b,1,{0x0C}},           //0E
{0x7c,1,{0x0D}},           //0F
{0x7d,1,{0x02}},
{0x7e,1,{0x02}},
{0x7f,1,{0x02}},
{0x80,1,{0x02}},
{0x81,1,{0x02}},
{0x82,1,{0x02}},
{0x83,1,{0x02}},
{0x84,1,{0x02}},
{0x85,1,{0x02}},
{0x86,1,{0x02}},
{0x87,1,{0x02}},
{0x88,1,{0x02}},
{0x89,1,{0x16}},
{0x8A,1,{0x01}},


{0xFF,3,{0x98,0x81,0x04}},  
//{0x00,1,{0x00}},    //************** 氝樓峈 00 3lane ㄛ 祥迡涴跺敵湔ん麼氪迡80峈 4lane**************************
{0x68,1,{0xDB}},     //nonoverlap 18ns (VGH and VGL)
{0x6D,1,{0x08}},     //gvdd_isc[2:0]=0 (0.2uA) 可減少VREG1擾動
{0x70,1,{0x00}},     //VGH_MOD and VGH_DC CLKDIV disable
{0x71,1,{0x00}},     //VGL CLKDIV disable
{0x66,1,{0x1E}},     //VGH 4X
{0x3A,1,{0x24}},     //PS_EN OFF
{0x82,1,{0x0A}},     //VREF_VGH_MOD_CLPSEL 12V
{0x84,1,{0x0A}},     //VREF_VGH_CLPSEL 12V
{0x85,1,{0x15}},     //VREF_VGL_CLPSEL 12V
{0x32,1,{0xAC}},     //開啟負channel的power saving
{0x8C,1,{0x80}},     //sleep out Vcom disable以避免Vcom source不同步enable導致玻璃微亮
{0x3C,1,{0xF5}},     //開啟Sample & Hold Function
{0x3A,1,{0x24}},     //PS_EN OFF       
{0xB5,1,{0x02}},     //GAMMA OP 
{0x31,1,{0x25}},     //SOURCE OP 
{0x88,1,{0x33}},     //VSP/VSN LVD Disable     
    

{0xFF,3,{0x98,0x81,0x01}},    
{0x22,1,{0x0A}},      
{0x31,1,{0x00}},     //column inversion     
{0x50,1,{0x5C}},     //VREG10UT 4.5  
{0x51,1,{0x5C}},     //VREG20UT -4.5
{0x53,1,{0x7f}},     //VC0M1  6b     
{0x55,1,{0x8f}},     //VC0M2   77     
{0x60,1,{0x2A}},     //SDT      
{0x61,1,{0x00}},     //CR    
{0x62,1,{0x19}},     //EQ
{0x63,1,{0x00}},     //PC
{0x2E,1,{0xF0}},     //1440 GATE NL SEL  
{0x2F,1,{0x00}},     //480 SOURCE

//Pos Register

{0xA0,1,{0x00}},                                                                                                        
{0xA1,1,{0x09}},                                                                                                        
{0xA2,1,{0x10}},                                                                                                        
{0xA3,1,{0x0F}},                                                                                                        
{0xA4,1,{0x0F}},                                                                                                        
{0xA5,1,{0x1F}},                                                                                                        
{0xA6,1,{0x12}},                                                                                                        
{0xA7,1,{0x17}},                                                                                                        
{0xA8,1,{0x42}},                                                                                                        
{0xA9,1,{0x1A}},                                                                                                        
{0xAA,1,{0x26}},                                                                                                        
{0xAB,1,{0x48}},                                                                                                        
{0xAC,1,{0x1E}},                                                                                                        
{0xAD,1,{0x1E}},                                                                                                        
{0xAE,1,{0x53}},                                                                                                        
{0xAF,1,{0x26}},                                                                                                        
{0xB0,1,{0x2A}},                                                                                                        
{0xB1,1,{0x4C}},                                                                                                        
{0xB2,1,{0x5D}},                                                                                                        
{0xB3,1,{0x39}},


//Neg Register
{0xC0,1,{0x00}},                                                                                                        
{0xC1,1,{0x09}},                                                                                                        
{0xC2,1,{0x10}},                                                                                                        
{0xC3,1,{0x0F}},                                                                                                        
{0xC4,1,{0x0F}},                                                                                                        
{0xC5,1,{0x1F}},                                                                                                        
{0xC6,1,{0x12}},                                                                                                        
{0xC7,1,{0x17}},                                                                                                        
{0xC8,1,{0x42}},                                                                                                        
{0xC9,1,{0x1A}},                                                                                                        
{0xCA,1,{0x26}},                                                                                                        
{0xCB,1,{0x48}},                                                                                                        
{0xCC,1,{0x1E}},                                                                                                        
{0xCD,1,{0x1E}},                                                                                                        
{0xCE,1,{0x53}},                                                                                                        
{0xCF,1,{0x26}},                                                                                                        
{0xD0,1,{0x2A}},                                                                                                        
{0xD1,1,{0x4C}},                                                                                                        
{0xD2,1,{0x5D}},                                                                                                        
{0xD3,1,{0x39}},


{0xFF,3,{0x98,0x81,0x00}},
{0x35,1,{0x00}},

{0x11,1,{0x00}},
{REGFLAG_DELAY, 100, {}},

{0x29,1,{0x00}},
{REGFLAG_DELAY, 20, {}},

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
    //5.2 18:9 59068 118136, 16:9 64754 115118
	params->physical_width = 62;//LCM_PHYSICAL_WIDTH/1000;
	params->physical_height = 125;//LCM_PHYSICAL_HEIGHT/1000;
	params->physical_width_um = 62476;//LCM_PHYSICAL_WIDTH;	= sqrt((size*25.4)^2/(18^2+9^2))*9*1000
	params->physical_height_um = 124951;//LCM_PHYSICAL_HEIGHT; = sqrt((size*25.4)^2/(18^2+9^2))*18*1000
	params->density = 320;//LCM_DENSITY;

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

	params->dsi.vertical_sync_active				= 4;//2;
		params->dsi.vertical_backporch					= 16;//8;
	params->dsi.vertical_frontporch					= 10;  // rom Q driver
		params->dsi.vertical_active_line				= FRAME_HEIGHT;

	params->dsi.horizontal_sync_active				= 20;//10;
		params->dsi.horizontal_backporch				= 60;//20;
		params->dsi.horizontal_frontporch				= 50;//40;
		params->dsi.horizontal_active_pixel			= FRAME_WIDTH;

                params->dsi.esd_check_enable = 1;
                params->dsi.customization_esd_check_enable = 1;
                params->dsi.lcm_esd_check_table[0].cmd          = 0x0a;
                params->dsi.lcm_esd_check_table[0].count        = 1;
                params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9c;
		params->dsi.PLL_CLOCK = 236;//240; //this value must be in MTK suggested table 224 235 241 245 231 251 238 243 237
		params->dsi.HS_TRAIL = 15;
}

#ifdef BUILD_LK
#define I2C_I2C_LCD_BIAS_CHANNEL   0
#define TPS65132_SLAVE_ADDR_WRITE  0x7C
static struct mt_i2c_t TPS65132_i2c;

static int TPS65132_write_byte(kal_uint8 addr, kal_uint8 value)
{
    kal_uint32 ret_code = I2C_OK;
    kal_uint8 write_data[2];
    kal_uint16 len;

    write_data[0] = addr;
    write_data[1] = value;

    TPS65132_i2c.id = I2C_I2C_LCD_BIAS_CHANNEL; /* I2C2; */
    /* Since i2c will left shift 1 bit, we need to set FAN5405 I2C address to >>1 */
    TPS65132_i2c.addr = (TPS65132_SLAVE_ADDR_WRITE >> 1);
    TPS65132_i2c.mode = ST_MODE;
    TPS65132_i2c.speed = 100;
    len = 2;

    ret_code = i2c_write(&TPS65132_i2c, write_data, len);
    /* printf("%s: i2c_write: ret_code: %d\n", __func__, ret_code); */

    return ret_code;
}

static void lcm_set_bias(int enable)
{
    int ret = 0;

    if (1 == enable)
    {
        GPIO_LCD_BIAS_ENN(1);
        MDELAY(30);
        GPIO_LCD_BIAS_ENP(1);
        MDELAY(50);

        ret = TPS65132_write_byte(0x00, 0x12);
        if (ret < 0)
            LCM_LOGI("nt35695----tps6132----cmd=%0x--i2c write error----\n", 0x00);
        else
            LCM_LOGI("nt35695----tps6132----cmd=%0x--i2c write success----\n", 0x00);
        ret = TPS65132_write_byte(0x01, 0x12);
        if (ret < 0)
            LCM_LOGI("nt35695----tps6132----cmd=%0x--i2c write error----\n", 0x01);
        else
            LCM_LOGI("nt35695----tps6132----cmd=%0x--i2c write success----\n", 0x01);
    }
    else
    {
        GPIO_LCD_BIAS_ENN(0);
        MDELAY(10);
        GPIO_LCD_BIAS_ENP(1);
        MDELAY(10);
    }
}
#endif

#define AUXADC_COMPARE_ID          0
#if defined(ILI9881D_HD1440_DSI_VDO_BOE) && defined(ILI9881D_HD1440_DSI_VDO_LT)
#define AUXADC_LCM_VOLTAGE_CHANNEL (12)
#define MIN_VOLTAGE (1000)
#define MAX_VOLTAGE (1400)
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
#endif

static unsigned int lcm_compare_id(void)
{
    int array[4];
    char buffer[4]={0,0,0,0};
    char id_high=0;
    char id_mid=0;
    char id_low=0;
    int id=0;

    SET_RESET_PIN(1);
    MDELAY(1);
    SET_RESET_PIN(0);
    MDELAY(1);
    SET_RESET_PIN(1);
    MDELAY(120);

    array[0]=0x00043902;
    array[1]=0x068198ff;
    dsi_set_cmdq(array, 2, 1);

    MDELAY(10);
    array[0] = 0x00083700;
    dsi_set_cmdq(array, 1, 1);

    MDELAY(10);
    read_reg_v2(0xf0, &buffer[0], 1);//    NC 0x00  0x98 0x16
    MDELAY(10);
    read_reg_v2(0xf1, &buffer[1], 1);//    NC 0x00  0x98 0x16
    MDELAY(10);
    read_reg_v2(0xf2, &buffer[2], 1);//    NC 0x00  0x98 0x16

    id_high = buffer[0];
    id_mid = buffer[1];
    id_low = buffer[2];
    id = (id_high<<16) | (id_mid<<8) |id_low;


        #ifdef BUILD_LK
        printf(CRITICAL, "%s, LK debug: ili9881d id = 0x%08x\n", __func__, buffer);
        #else
        printk("%s: ili9881c id = 0x%08x \n", __func__, id);
        #endif

        return (LCM_ID_ILI9881C == id)?1:0;
}

#if defined(ILI9881D_HD1440_DSI_VDO_BOE) && defined(ILI9881D_HD1440_DSI_VDO_LT)
static unsigned int ili_lcm_compare_id(void)
{

 int data[4] = {0,0,0,0};
 int res = 0;
 int rawdata = 0;
 int lcm_vol = 0;

 #ifdef AUXADC_LCM_VOLTAGE_CHANNEL
 res = IMM_GetOneChannelValue(AUXADC_LCM_VOLTAGE_CHANNEL,data,&rawdata);
 if(res < 0)
 {
	 #ifdef BUILD_LK
	 dprintf(0,"lixf lcd [adc_uboot]: get data error\n");
	 #endif
	 return 0;

 }
 #endif
 lcm_vol = data[0]*1000+data[1]*10;

 #ifdef BUILD_LK
 dprintf(0,"lk: lcm_vol= %d , file : %s, line : %d\n",lcm_vol, __FILE__, __LINE__);
 #else
 printk("kernel: lcm_vol= %d , file : %s, line : %d\n",lcm_vol, __FILE__, __LINE__);
 #endif

 if (lcm_vol>=MIN_VOLTAGE &&lcm_vol <= MAX_VOLTAGE &&lcm_compare_id())
 {
 		return 1;
 }

 return 0;
}
#endif
 
static void lcm_init(void)
{
    LCM_LOGI("nt35695----tps6132-lcm_init3333333333333333333333333333===============================devin----\n");

    // SET_GPIO_LCD_ENP_LDO18_PIN(1); //incell must add

#ifndef BUILD_LK
	tps65132_set_vpos_volt(5500);
	tps65132_set_vneg_volt(5500);
	tps65132_vpos_enable(1);
	tps65132_vneg_enable(1);
#else
	lcm_set_bias(1);
#endif
	
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
    MDELAY(100);

    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
    push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);

    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);
	
	// SET_GPIO_LCD_ENP_LDO18_PIN(0); //incell must add
#ifndef BUILD_LK
	tps65132_vpos_enable(0);
	tps65132_vneg_enable(0);
#endif
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

LCM_DRIVER ili9881d_hd1440_dsi_vdo_boe_lcm_drv =
{
    .name			= "ili9881d_hd1440_dsi_vdo_boe",
#if defined(CONFIG_PRIZE_HARDWARE_INFO) && !defined (BUILD_LK)
	.lcm_info = {
		.chip	= "ili9881d",
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
	//prize-chenxiaowen-20161013-compatibility-(ili9881c_hd720_dsi_vdo_yin vs ili9881c_hd720_dsi_vdo_cmi)-start
  #if defined(ILI9881D_HD1440_DSI_VDO_BOE) && defined(ILI9881D_HD1440_DSI_VDO_LT)
  .compare_id = ili_lcm_compare_id,
  #else
  .compare_id = lcm_compare_id,
  #endif
	//prize-chenxiaowen-20161013-compatibility-(ili9881c_hd720_dsi_vdo_yin vs ili9881c_hd720_dsi_vdo_cmi)-end
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update,
#endif
};
