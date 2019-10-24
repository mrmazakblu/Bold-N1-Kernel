#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>


#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>


#ifdef CFG_PLATFORM_MTK
//#include <mach/gpio_const.h>
#if defined(CONFIG_ARCH_MT6572)
#include <mach/mt_gpio.h>
#include <mach/mt_spi.h>
#else
//#include <mt-plat/mt_gpio.h>
//#include <mt_spi.h>
#include <mt-plat/mtk_gpio.h>
//#include <mtk_spi.h>
#include <linux/platform_data/spi-mt65xx.h>



#endif
#endif

#include "veb_errno.h"
#include "veb_common.h"
#include "veb_platform.h"




#if defined(CFG_PLATFORM_MTK)
#if defined(CONFIG_MACH_MT6771)
static const struct mtk_chip_config veb_mtk_spi_conf = {
	.rx_mlsb = 1,
	.tx_mlsb = 1,
	.cs_pol = 0,
	.sample_sel = 0,
};

#else

static struct mt_chip_conf veb_mtk_spi_conf =
{
    .setuptime  = 1, 
    .holdtime   = 1,
    .high_time  = 1,
    .low_time   = 1,
    .cs_idletime = 15, 

    .cpol = 1,
    .cpha = 1,

    .rx_mlsb = 1,  
    .tx_mlsb = 1,

    .tx_endian = 0, 
    .rx_endian = 0,

    .com_mod = DMA_TRANSFER,
    .pause = 0,
    .finish_intr = 1,
    .deassert = 0,
    .ulthigh = 0,
    .tckdly = 2,
};
#endif
#if defined(CONFIG_ARCH_MT6572)
#define    VEB_PIN_WAKE                     GPIO30  // DPI_R4
#define    VEB_PIN_STATUS                   GPIO31  // DPI_R5
#define    VEB_PIN_RESET                    GPIO18  // DPI_B4

// #define    VEB_PIN_WAKE                     GPIO111
// #define    VEB_PIN_STATUS                   GPIO110
// #define    VEB_PIN_RESET                    GPIO112

#define    VEB_PIN_SPI_CS                   97
#define    VEB_PIN_SPI_SCK                  98
#define    VEB_PIN_SPI_MOSI                 99
#define    VEB_PIN_SPI_MISO                 100

#define    MTK_PIN_MODE_SPI                 GPIO_MODE_01
#define    MTK_PIN_MODE_GPIO                GPIO_MODE_00
#elif  defined(CONFIG_ARCH_MT6755)
#define    VEB_PIN_WAKE                     (GPIO83 | 0x80000000)
#define    VEB_PIN_STATUS                   (GPIO84 | 0x80000000)

#define    VEB_PIN_SPI_CS                   (GPIO26 | 0x80000000)
#define    VEB_PIN_SPI_SCK                  (GPIO28 | 0x80000000)
#define    VEB_PIN_SPI_MOSI                 (GPIO27 | 0x80000000)
#define    VEB_PIN_SPI_MISO                 (GPIO25 | 0x80000000)

#define    MTK_PIN_MODE_SPI                 GPIO_MODE_02
#define    MTK_PIN_MODE_GPIO                GPIO_MODE_GPIO

#elif  defined(CONFIG_ARCH_MT6735)
#define    VEB_PIN_RESET                    (GPIO70 | 0x80000000)
#define    VEB_PIN_WAKE                     (GPIO59 | 0x80000000)
#define    VEB_PIN_STATUS                   (GPIO60 | 0x80000000)
#define    VEB_PIN_LDO_EN                   (GPIO57 | 0x80000000)

#define    VEB_PIN_SPI_CS                   (GPIO5 | 0x80000000)
#define    VEB_PIN_SPI_SCK                  (GPIO6 | 0x80000000)
#define    VEB_PIN_SPI_MOSI                 (GPIO4 | 0x80000000)
#define    VEB_PIN_SPI_MISO                 (GPIO3 | 0x80000000)

#define    MTK_PIN_MODE_SPI                 GPIO_MODE_03
#define    MTK_PIN_MODE_GPIO                GPIO_MODE_GPIO
#endif

struct pinctrl *pinctrl_gpios;
//struct pinctrl_state *veb_pins_default;
struct pinctrl_state *pins_reset_high, *pins_reset_low, *pins_reset_in;
struct pinctrl_state *pins_power_high, *pins_power_low;
struct pinctrl_state *pins_irq_inpull;
struct pinctrl_state *pins_spi_csb_low;
struct pinctrl_state *pins_spi_csb_high;
struct pinctrl_state *pins_spi_clk;
struct pinctrl_state *pins_spi_miso;
struct pinctrl_state *pins_spi_mosi;
struct pinctrl_state *pins_gpio_csb_low;
struct pinctrl_state *pins_gpio_clk;
struct pinctrl_state *pins_gpio_miso;
struct pinctrl_state *pins_gpio_mosi;
struct pinctrl_state *pins_wake_high, *pins_wake_low;

extern unsigned int irq_gpio;



static int veb_a5_get_gpio_dts_info(void){
	int ret = 0;
	struct device_node *node = NULL;
	struct platform_device *pdev = NULL;
	//unsigned int irq_gpio;
	
	node = of_find_compatible_node(NULL, NULL, "veb,veba5-spi");
	
	if(node){
		pdev = of_find_device_by_node(node);
		if(pdev){
			pinctrl_gpios = devm_pinctrl_get(&pdev->dev);
			if (IS_ERR(pinctrl_gpios)) {
				ret = PTR_ERR(pinctrl_gpios);
				VEB_ERR("%s can't find veba5 pinctrl\n", __func__);
				return ret;
			}
		}else {
			VEB_ERR("%s platform device is null\n", __func__);
		}
		
		} else {
		VEB_ERR("%s device node is null\n", __func__);
	}
	
	/*veb_pins_default = pinctrl_lookup_state(pinctrl_gpios, "default");
	if(IS_ERR(veb_pins_default)){
		ret = PTR_ERR(veb_pins_default);
		VEB_A3_SPI_ERR("%s can't find veba3 pinctrl default\n", __func__);
		return ret;
	}*/
	
	pins_reset_high = pinctrl_lookup_state(pinctrl_gpios, "a5_reset_high");
	if(IS_ERR(pins_reset_high)){
		ret = PTR_ERR(pins_reset_high);
		VEB_ERR("%s can't find veba5 pinctrl pins_reset_high\n", __func__);
		return ret;
	}
		
	pins_reset_low = pinctrl_lookup_state(pinctrl_gpios, "a5_reset_low");
	if(IS_ERR(pins_reset_low)){
		ret = PTR_ERR(pins_reset_low);
		VEB_ERR("%s can't find veba5 pinctrl pins_reset_low\n", __func__);
		return ret;
	}
	pins_reset_in = pinctrl_lookup_state(pinctrl_gpios, "a5_reset_in");
	if(IS_ERR(pins_reset_in)){
		ret = PTR_ERR(pins_reset_in);
		VEB_ERR("%s can't find veba5 pinctrl pins_reset_low\n", __func__);
		return ret;
	}	
	pins_power_high = pinctrl_lookup_state(pinctrl_gpios, "a5_power_high");
	if(IS_ERR(pins_power_high)){
		ret = PTR_ERR(pins_power_high);
		VEB_ERR("%s can't find veba5 pinctrl pins_power_high\n", __func__);
		return ret;
	}
		
	pins_power_low = pinctrl_lookup_state(pinctrl_gpios, "a5_power_low");
	if(IS_ERR(pins_power_low)){
		ret = PTR_ERR(pins_power_low);
		VEB_ERR("%s can't find veba5 pinctrl pins_power_low\n", __func__);
		return ret;
	}
		
	pins_irq_inpull = pinctrl_lookup_state(pinctrl_gpios, "a5_irq_inpull");
	if(IS_ERR(pins_irq_inpull)){
		ret = PTR_ERR(pins_irq_inpull);
		VEB_ERR("%s can't find veba5 pinctrl pins_irq_inpull\n", __func__);
		return ret;
	}	
		
	pins_spi_csb_low = pinctrl_lookup_state(pinctrl_gpios, "a5_spi_csb_low");
	if(IS_ERR(pins_spi_csb_low)){
		ret = PTR_ERR(pins_spi_csb_low);
		VEB_ERR("%s can't find veba5 pinctrl pins_spi_csb_low\n", __func__);
		return ret;
	}
    pins_spi_csb_high = pinctrl_lookup_state(pinctrl_gpios, "a5_spi_csb_high");
	if(IS_ERR(pins_spi_csb_high)){
		ret = PTR_ERR(pins_spi_csb_high);
		VEB_ERR("%s can't find veba5 pinctrl pins_spi_csb_high\n", __func__);
		return ret;
	}
		
	pins_spi_clk = pinctrl_lookup_state(pinctrl_gpios, "a5_spi_clk");
	if(IS_ERR(pins_spi_clk)){
		ret = PTR_ERR(pins_spi_clk);
		VEB_ERR("%s can't find veba5 pinctrl pins_spi_clk\n", __func__);
		return ret;
	}
		
	pins_spi_miso = pinctrl_lookup_state(pinctrl_gpios, "a5_spi_miso");
	if(IS_ERR(pins_spi_miso)){
		ret = PTR_ERR(pins_spi_miso);
		VEB_ERR("%s can't find veba5 pinctrl pins_spi_miso\n", __func__);
		return ret;
	}
	
	pins_spi_mosi = pinctrl_lookup_state(pinctrl_gpios, "a5_spi_mosi");
	if(IS_ERR(pins_spi_mosi)){
		ret = PTR_ERR(pins_spi_mosi);
		VEB_ERR("%s can't find veba5 pinctrl pins_spi_mosi\n", __func__);
		return ret;
	}
    //////////////
    pins_gpio_csb_low = pinctrl_lookup_state(pinctrl_gpios, "a5_gpio_csb_low");
	if(IS_ERR(pins_gpio_csb_low)){
		ret = PTR_ERR(pins_gpio_csb_low);
		VEB_ERR("%s can't find veba5 pinctrl pins_gpio_csb_low\n", __func__);
		return ret;
	}
		
	pins_gpio_clk = pinctrl_lookup_state(pinctrl_gpios, "a5_gpio_clk");
	if(IS_ERR(pins_gpio_clk)){
		ret = PTR_ERR(pins_gpio_clk);
		VEB_ERR("%s can't find veba5 pinctrl pins_gpio_clk\n", __func__);
		return ret;
	}
		
	pins_gpio_miso = pinctrl_lookup_state(pinctrl_gpios, "a5_gpio_miso");
	if(IS_ERR(pins_gpio_miso)){
		ret = PTR_ERR(pins_gpio_miso);
		VEB_ERR("%s can't find veba5 pinctrl pins_gpio_miso\n", __func__);
		return ret;
	}
	
	pins_gpio_mosi = pinctrl_lookup_state(pinctrl_gpios, "a5_gpio_mosi");
	if(IS_ERR(pins_gpio_mosi)){
		ret = PTR_ERR(pins_gpio_mosi);
		VEB_ERR("%s can't find veba5 pinctrl pins_gpio_mosi\n", __func__);
		return ret;
	}

    pins_wake_high = pinctrl_lookup_state(pinctrl_gpios, "a5_wake_high");
	if(IS_ERR(pins_wake_high)){
		ret = PTR_ERR(pins_wake_high);
		VEB_ERR("%s can't find veba5 pinctrl pins_wake_high\n", __func__);
		return ret;
	}
		
	pins_wake_low = pinctrl_lookup_state(pinctrl_gpios, "a5_wake_low");
	if(IS_ERR(pins_wake_low)){
		ret = PTR_ERR(pins_wake_low);
		VEB_ERR("%s can't find veba5 pinctrl pins_wake_low\n", __func__);
		return ret;
	}
	VEB_ERR("%s get pinctrl successful\n", __func__);
	return ret;
}

void veb_mtk_pin_init(void)
{

    veb_a5_get_gpio_dts_info();
	
	pinctrl_select_state(pinctrl_gpios, pins_spi_csb_high);
	pinctrl_select_state(pinctrl_gpios, pins_spi_clk);
	pinctrl_select_state(pinctrl_gpios, pins_spi_miso);
	pinctrl_select_state(pinctrl_gpios, pins_spi_mosi);
	
    pinctrl_select_state(pinctrl_gpios, pins_power_high);
	mdelay(5);
    pinctrl_select_state(pinctrl_gpios, pins_wake_high);
	pinctrl_select_state(pinctrl_gpios, pins_irq_inpull);
    pinctrl_select_state(pinctrl_gpios, pins_reset_low);
    mdelay(5);
    pinctrl_select_state(pinctrl_gpios, pins_reset_high);    
    
    
}

void veb_mtk_pin_resume(void)
{
	pinctrl_select_state(pinctrl_gpios, pins_spi_csb_high);
	pinctrl_select_state(pinctrl_gpios, pins_spi_clk);
	pinctrl_select_state(pinctrl_gpios, pins_spi_miso);
	pinctrl_select_state(pinctrl_gpios, pins_spi_mosi);
}

void veb_mtk_pin_suspend(void)
{

    pinctrl_select_state(pinctrl_gpios, pins_gpio_csb_low);
	pinctrl_select_state(pinctrl_gpios, pins_gpio_clk);
	pinctrl_select_state(pinctrl_gpios, pins_gpio_miso);
	pinctrl_select_state(pinctrl_gpios, pins_gpio_mosi);

}

#ifndef CONFIG_MACH_MT6771
static ssize_t spi_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct spi_device *spi;
    struct mt_chip_conf *chip_config;
    int len;

    spi = container_of(dev, struct spi_device, dev);
    VEB_DBG("SPIDEV name is:%s\n", spi->modalias);
    chip_config = (struct mt_chip_conf *) spi->controller_data;
    if (!chip_config) 
    {
        VEB_DBG("chip_config is NULL.\n");
        chip_config = kzalloc (sizeof(struct mt_chip_conf), GFP_KERNEL);
        if (!chip_config)
            return -ENOMEM;
    }

    len = snprintf(buf, PAGE_SIZE, 
            " setuptime: %d\n holdtime: %d \
            \n high_time: %d\n low_time: %d \
            \n cs_idletime: %d\n ulthgh_thrsh: %d \
            \n cpol: %d\n cpha: %d \
            \n tx_mlsb: %d\n rx_mlsb: %d \
            \n tx_endian: %d\n rx_endian: %d \
            \n com_mod: %d\n pause: %d \
            \n finish_intr: %d\n deassert: %d \
            \n ulthigh: %d\n tckdly: %d\n", 
            chip_config->setuptime, chip_config->holdtime,
            chip_config->high_time, chip_config->low_time,
            chip_config->cs_idletime, chip_config->ulthgh_thrsh,
            chip_config->cpol, chip_config->cpha,
            chip_config->tx_mlsb, chip_config->rx_mlsb,
            chip_config->tx_endian, chip_config->rx_endian,
            chip_config->com_mod, chip_config->pause,
            chip_config->finish_intr, chip_config->deassert,
            chip_config->ulthigh, chip_config->tckdly);

    return (len >= PAGE_SIZE) ? (PAGE_SIZE - 1) : len;
}

static ssize_t spi_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct spi_device *spi;
    struct mt_chip_conf *chip_config;
    u32 setuptime, holdtime, high_time, low_time;
    u32 cs_idletime, ulthgh_thrsh;
    int cpol, cpha,tx_mlsb, rx_mlsb, tx_endian;
    int rx_endian, com_mod, pause, finish_intr;
    int deassert, tckdly, ulthigh;
    
    spi = container_of(dev, struct spi_device, dev);
    VEB_DBG("SPIDEV name is:%s\n", spi->modalias);
    VEB_DBG("==== buf:%s\n", buf);

    chip_config = (struct mt_chip_conf *) spi->controller_data;
    if (!chip_config) 
    {
        VEB_DBG( "chip_config is NULL.\n");
        chip_config = kzalloc ( sizeof ( struct mt_chip_conf ), GFP_KERNEL );
        if ( !chip_config ) 
            return -ENOMEM;
    }    

    if (!strncmp(buf, "-h", 2 ) ) 
    {
        VEB_DBG("Please input the parameters for this device.\n");
    } 
    else if ( !strncmp(buf, "-w", 2 ) ) 
    {
        buf += 3;
        if (!buf) 
        {
            VEB_DBG("buf is NULL.\n");
            goto out;
        }
        if (!strncmp(buf, "setuptime=", 10) && (1 == sscanf(buf + 10, "%d", &setuptime))) 
        {
            VEB_DBG("setuptime is:%d\n", setuptime);
            chip_config->setuptime = setuptime;
        }
        else if (!strncmp(buf, "holdtime=", 9)&&(1 == sscanf(buf + 9, "%d", &holdtime))) 
        {
            VEB_DBG("Set holdtime is:%d\n", holdtime);
            chip_config->holdtime = holdtime;    
        }
        else if (!strncmp(buf, "high_time=", 10)&&(1 == sscanf(buf + 10, "%d", &high_time))) 
        {
            VEB_DBG("Set high_time is:%d\n", high_time);
            chip_config->high_time = high_time;    
        }
        else if (!strncmp(buf, "low_time=", 9)&&(1 == sscanf(buf + 9, "%d", &low_time))) 
        {
            VEB_DBG("Set low_time is:%d\n", low_time);
            chip_config->low_time = low_time;
        }
        else if (!strncmp(buf, "cs_idletime=", 12)&&(1 == sscanf(buf + 12, "%d", &cs_idletime))) 
        {
            VEB_DBG("Set cs_idletime is:%d\n", cs_idletime);
            chip_config->cs_idletime = cs_idletime;    
        }
        else if (!strncmp(buf, "ulthgh_thrsh=", 13)&&(1 == sscanf(buf + 13, "%d", &ulthgh_thrsh))) 
        {
            VEB_DBG("Set slwdown_thrsh is:%d\n", ulthgh_thrsh);
            chip_config->ulthgh_thrsh = ulthgh_thrsh; 
        }
        else if (!strncmp(buf, "cpol=", 5) && (1 == sscanf(buf + 5, "%d", &cpol)))
        {
            VEB_DBG("Set cpol is:%d\n", cpol);
            chip_config->cpol = cpol;
        }
        else if (!strncmp(buf, "cpha=", 5) && (1 == sscanf(buf + 5, "%d", &cpha))) 
        {
            VEB_DBG("Set cpha is:%d\n", cpha);
            chip_config->cpha = cpha;
        }
        else if (!strncmp(buf, "tx_mlsb=", 8)&&(1 == sscanf(buf + 8, "%d", &tx_mlsb))) 
        {
            VEB_DBG("Set tx_mlsb is:%d\n", tx_mlsb);
            chip_config->tx_mlsb = tx_mlsb;    
        }
        else if (!strncmp(buf, "rx_mlsb=", 8)&&(1 == sscanf(buf + 8, "%d", &rx_mlsb))) 
        {
            VEB_DBG("Set rx_mlsb is:%d\n", rx_mlsb);
            chip_config->rx_mlsb = rx_mlsb;    
        }
        else if (!strncmp(buf, "tx_endian=", 10)&&(1 == sscanf(buf + 10, "%d", &tx_endian))) 
        {
            VEB_DBG("Set tx_endian is:%d\n", tx_endian);
            chip_config->tx_endian = tx_endian;    
        }
        else if (!strncmp(buf, "rx_endian=", 10)&&(1 == sscanf(buf + 10, "%d", &rx_endian))) 
        {
            VEB_DBG("Set rx_endian is:%d\n", rx_endian);
            chip_config->rx_endian = rx_endian;    
        }
        else if (!strncmp(buf, "com_mod=", 8)&&(1 == sscanf(buf + 8, "%d", &com_mod))) 
        {
            chip_config->com_mod = com_mod;
            VEB_DBG("Set com_mod is:%d\n", com_mod);
        }
        else if (!strncmp(buf, "pause=", 6)&&(1 == sscanf(buf + 6, "%d", &pause))) 
        {
            VEB_DBG("Set pause is:%d\n", pause);
            chip_config->pause = pause;
        }
        else if (!strncmp(buf, "finish_intr=", 12)&&(1==sscanf(buf + 12, "%d", &finish_intr))) 
        {
            VEB_DBG("Set finish_intr is:%d\n", finish_intr);
            chip_config->finish_intr = finish_intr;
        }
        else if (!strncmp(buf, "deassert=", 9)&&(1 == sscanf(buf + 9, "%d", &deassert))) 
        {
            VEB_DBG("Set deassert is:%d\n", deassert);
            chip_config->deassert = deassert;    
        }
        else if (!strncmp(buf, "ulthigh=", 8 ) && ( 1 == sscanf(buf + 8, "%d", &ulthigh))) 
        {
            VEB_DBG("Set ulthigh is:%d\n", ulthigh);    
            chip_config->ulthigh = ulthigh;
        }
        else if (!strncmp(buf, "tckdly=",7) && ( 1 == sscanf(buf + 7, "%d", &tckdly))) 
        {
            VEB_DBG("Set tckdly is:%d\n", tckdly);
            chip_config->tckdly = tckdly;
        }
        else 
        {
            VEB_DBG("Wrong parameters.\n");
            goto out;
        }
        spi->controller_data = chip_config;
//            spi_setup(spi);
    }
out:
    return count;
}

static ssize_t spi_msg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{    
    int ret = 0, i = 0;
    unsigned char version[256];

    memset(version, 0, sizeof(version));
    VEB_DBG("+++++++++++++++++spi_msg_store");
    if ( !strncmp ( buf, "-n", 2 ) ) 
    {
        //ret = send_cmd_getsn(dev, version);
        if (ret < 0)
        {
            VEB_DBG("send_cmd_getsn error\n" );
        }
        VEB_DBG("===++++ sn:");
        for(i = 0; i < 16; i++)
        {
            VEB_DBG("0x%x, ", version[i]);
        }
        VEB_DBG("\n");
    }

    return ret;
}

static DEVICE_ATTR(spi, 0660, spi_show, spi_store);
static DEVICE_ATTR(spi_msg, 0660, NULL, spi_msg_store);

static struct device_attribute *spi_attribute[]={
    &dev_attr_spi,
    &dev_attr_spi_msg,    
};

static int spi_create_attribute(struct device *dev)
{
    int num,idx;
    int err = 0;
    
    num = (int)(sizeof(spi_attribute)/sizeof(spi_attribute[0]));
    for (idx = 0; idx < num; idx ++) {
        if ((err = device_create_file(dev, spi_attribute[idx])))
            break;
    }

    return err;
}

static void spi_remove_attribute(struct device *dev)
{
    int num, idx;

    num = (int)(sizeof(spi_attribute) / sizeof(spi_attribute[0]));
    for (idx = 0; idx < num; idx ++) 
    {        
        device_remove_file(dev, spi_attribute[idx]);        
    }
}
#endif //spi_show
#else

static int wake_gpio;
static int status_gpio;
static int reset_gpio;
static int power_en_gpio;

void veb_qual_pin_init(struct spi_device *spi)
{
	if (spi->dev.of_node){
		reset_gpio = of_get_named_gpio_flags(spi->dev.of_node, \
                        "veb_spi_vebv3,reset-gpio", 0, NULL);
		power_en_gpio = of_get_named_gpio_flags(spi->dev.of_node, \
                           "veb_spi_vebv3,power-en-gpio", 0, NULL);
		status_gpio = of_get_named_gpio_flags(spi->dev.of_node, \
                      "veb_spi_vebv3,status-gpio", 0, NULL);
		wake_gpio = of_get_named_gpio_flags(spi->dev.of_node, \
                      "veb_spi_vebv3,wake-gpio", 0, NULL);
	}

    // power on A5 ldo
    gpio_direction_output(power_en_gpio, 0);
    msleep(15);
    gpio_direction_output(power_en_gpio, 1);

    gpio_direction_output(reset_gpio, 0);
    msleep(15);
    gpio_direction_output(reset_gpio, 1);

    gpio_direction_input(status_gpio);

    /**
     * wake chipset up
     */
    gpio_direction_output(wake_gpio, 0);
    mdelay(5);
    gpio_direction_output(wake_gpio, 1);
}

void veb_mtk_pin_suspend(void)
{
 //add later
}

#endif

void veb_spi_wake_hardware(int count)
{
    int i = 0;

    VEB_TRACE_IN();

#ifdef CFG_PLATFORM_MTK
    //mt_set_gpio_out(VEB_PIN_WAKE, GPIO_OUT_ZERO);
    pinctrl_select_state(pinctrl_gpios, pins_wake_low);
#else
	gpio_direction_output(wake_gpio, 0);
#endif
    for(i=0; i< count;i++)
    {
        udelay(100);
    }
#ifdef CFG_PLATFORM_MTK
    //mt_set_gpio_out(VEB_PIN_WAKE,  GPIO_OUT_ONE);
    pinctrl_select_state(pinctrl_gpios, pins_wake_high);
#else
	gpio_direction_output(wake_gpio, 1);
#endif

    VEB_TRACE_OUT();
    return;
}

int veb_spi_wake(int count)
{
    int try_count = 0;

#ifdef CFG_PLATFORM_MTK
    //while (mt_get_gpio_in(VEB_PIN_STATUS) != 0)
    while (gpio_get_value(irq_gpio) != 0)
#else
    while (gpio_get_value(status_gpio) != 0)
#endif
    {
        if (try_count)
        {
            msleep(500);  //to solve reset fail problem
        }

#ifdef CFG_PLATFORM_MTK
       // if (0 != mt_get_gpio_in(VEB_PIN_STATUS))
       if (0 != gpio_get_value(irq_gpio))
#else
        if (0 != gpio_get_value(status_gpio))
#endif
        {
            veb_spi_wake_hardware(count);
        }
        else
        {
            break;
        }       

        mdelay(2);
#ifdef CFG_PLATFORM_MTK
        //if(0 == mt_get_gpio_in(VEB_PIN_STATUS))
        if(0 == gpio_get_value(irq_gpio))
#else
        if(0 == gpio_get_value(status_gpio))
#endif
        {
            break;
        }

        if(try_count++ > 4)
        {
            veb_spi_reset();
            printk("Reset failed, power off and on!\n");
            return -1;
        }
    }

    return 0;
}

int veb_wait_ready(void)
{
    unsigned int time = 0;

    VEB_TRACE_IN();

    udelay(1);
#ifdef CFG_PLATFORM_MTK
    //while(mt_get_gpio_in(VEB_PIN_STATUS) != 0)
    while(gpio_get_value(irq_gpio) != 0)
#else
    while(gpio_get_value(status_gpio) != 0)
#endif
    {    
        time++;
        udelay(1);

        if(time > 1000000)
        {
            veb_spi_wake(WAKE_TIME);
            return -1; 
        }
    }

#ifdef CFG_VEB_DEBUG
    if(time > 0)
    {
        VEB_DBG("wait busy %d us....\n", time);
    }
#endif

    VEB_TRACE_OUT();
    return 0;
}

int veb_rsa_wait_ready(int msUnit)
{
    unsigned int time = 0;

    VEB_TRACE_IN();

    udelay(1);
#ifdef CFG_PLATFORM_MTK
    //while(0 != mt_get_gpio_in(VEB_PIN_STATUS))
    while(0 != gpio_get_value(irq_gpio))
#else
    while(gpio_get_value(status_gpio) != 0)
#endif
    {
        time++;
        msleep(msUnit);

        if(time > 300)
        {
            veb_spi_wake(WAKE_TIME);
            VEB_ERR("timeout!\n");
            return VEB_ERR_RSA_WAIT_READY_TIMEOUT; 
        }
    }

#ifdef CFG_VEB_DEBUG
    if(time > 0)
    {
        VEB_DBG("wait busy %d ms....\n", time * 200);
    }
#endif

    VEB_TRACE_OUT();
    return 0;
}

int veb_smartcard_wait_ready(int msUnit)
{
    unsigned int time = 0;
    unsigned int time2 = 0;

    VEB_TRACE_IN();

    udelay(1);
#ifdef CFG_PLATFORM_MTK
    //while(0 != mt_get_gpio_in(VEB_PIN_STATUS))
    while(0 != gpio_get_value(irq_gpio))
#else
    while(gpio_get_value(status_gpio) != 0)
#endif
    {
        time++;
        #ifdef CFG_PLATFORM_MTK
        //while(mt_get_gpio_in(VEB_PIN_STATUS) != 0)
        while(gpio_get_value(irq_gpio) != 0)
        #else
        while(gpio_get_value(status_gpio) != 0)
        #endif
        {    
            time2++;
            //udelay(1);
            usleep_range(50, 52);
            if(time2 > 20*msUnit)
            {
                time2 = 0;
                break;               
            }
        }
        if(time > 1000)
        {
            veb_spi_wake(WAKE_TIME);
            VEB_ERR("timeout!\n");
            return VEB_ERR_RSA_WAIT_READY_TIMEOUT; 
        }
    }

#ifdef CFG_VEB_DEBUG
    if(time > 0)
    {
        VEB_DBG("wait busy %d ms....\n", time * 200);
    }
#endif
    printk("smartcard ready--hisy--time=%d---time2=%d\n",time,time2);
    VEB_TRACE_OUT();
    return 0;
}

void veb_spi_reset(void)
{
    //int ret = VEB_OK;
    VEB_TRACE_IN();
#ifdef CFG_PLATFORM_MTK
  //#if (defined(CONFIG_ARCH_MT6572) || defined (CONFIG_ARCH_MT6735))
    //mt_set_gpio_out(VEB_PIN_RESET, GPIO_OUT_ZERO);
  //  msleep(50);
    //mt_set_gpio_out(VEB_PIN_RESET,  GPIO_OUT_ONE);
    printk("veb-----reset-------in\n");
	pinctrl_select_state(pinctrl_gpios, pins_reset_low);
    printk("veb-----reset-------low\n");
    msleep(5000);
    //gpio_direction_output(reset_gpio,  1);
    pinctrl_select_state(pinctrl_gpios, pins_reset_high);
    printk("veb-----reset-------out\n");
  //#endif
#else
    gpio_direction_output(reset_gpio, 0);
    msleep(50);
    gpio_direction_output(reset_gpio,  1);
#endif

    msleep(10);
#if 0   // only rest, veb_wait_ready will call dead incall.
    ret = veb_wait_ready();
    if (ret != 0)
    {
        VEB_ERR("wait ready failed!(%d)!\n", ret);
    }
#endif
    VEB_TRACE_OUT();
}


int veb_platform_init(struct spi_device *spi)
{
    int ret = VEB_OK;

#if defined(CFG_PLATFORM_MTK)
    veb_mtk_pin_init();
#ifndef CONFIG_MACH_MT6771  //hisy
    ret = spi_create_attribute(&(spi->dev));
    if(ret != 0)
    {
        VEB_ERR("create spi attribute failed(%d)!\n", ret);
        return ret;
    }
#endif
    spi->controller_data = (void*)&veb_mtk_spi_conf;
#else
    veb_qual_pin_init(spi);
#endif

    return ret;
}

int veb_platform_exit(struct spi_device *spi)
{
#if defined(CFG_PLATFORM_MTK)
#ifndef CONFIG_MACH_MT6771  //hisy
    spi_remove_attribute(&(spi->dev));
#endif
#endif

    return 0;
}

