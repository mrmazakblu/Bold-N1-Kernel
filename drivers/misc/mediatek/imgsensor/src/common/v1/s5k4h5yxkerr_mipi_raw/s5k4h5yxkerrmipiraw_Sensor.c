/*******************************************************************************************/


/*******************************************************************************************/
/*BEGIN PN:SPBB-1222 ,MODIFIED BY W00167383,2012/12/26 */

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>    
#include <linux/fs.h>
#include <asm/atomic.h>
//#include <asm/system.h>
#include "kd_camera_typedef.h"

//#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "s5k4h5yxkerrmipiraw_Sensor.h"
#include "s5k4h5yxkerrmipiraw_Camera_Sensor_para.h"
#include "s5k4h5yxkerrmipiraw_CameraCustomized.h"
static DEFINE_SPINLOCK(s5k4h5yxkerrmipiraw_drv_lock);

//#define S5K4H5YXKERR_DEBUG
//#define S5K4H5YXKERR_DEBUG_SOFIA

#ifdef S5K4H5YXKERR_DEBUG
    #define S5K4H5YXKERRDB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[S5K4H5YXKERRMIPI]" , fmt, ##arg)
#else
    #define S5K4H5YXKERRDB(x,...)
#endif

#ifdef S5K4H5YXKERR_DEBUG_SOFIA
    #define S5K4H5YXKERRDBSOFIA(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[S5K4H5YXKERRMIPI]", fmt, ##arg)
#else
    #define S5K4H5YXKERRDBSOFIA(x,...)
#endif

#define mDELAY(ms)  mdelay(ms)
#define Capture8M
#define Capture8M_New

/*
#if defined(Capture8M_New)
    #ifndef Capture8M
    #define Capture8M
#endif
*/

//add this globle data
kal_uint32 s5k4h5yxkerr_video_frame_length = 2060;
kal_uint32 s5k4h5yxkerr_video_line_length = 8088;
kal_uint32 s5k4h5yxkerr_preview_frame_length = 2260;
kal_uint32 s5k4h5yxkerr_preview_line_length = 7376;
#ifdef Capture8M_New
kal_uint32 s5k4h5yxkerr_capture_frame_length =2498;
kal_uint32 s5k4h5yxkerr_capture_line_length = 8112;
#else
kal_uint32 s5k4h5yxkerr_capture_frame_length =2486;
kal_uint32 s5k4h5yxkerr_capture_line_length = 8088;
#endif

kal_uint32 S5K4H5YXKERR_FeatureControl_PERIOD_PixelNum;
kal_uint32 S5K4H5YXKERR_FeatureControl_PERIOD_LineNum;

kal_uint32 cont_preview_line_length_kerr = 7376;
kal_uint32 cont_preview_frame_length_kerr = 2260;
#ifdef Capture8M_New
kal_uint32 cont_capture_line_length_kerr = 8112;
kal_uint32 cont_capture_frame_length_kerr = 2498;
#else
kal_uint32 cont_capture_line_length_kerr = 8088;
kal_uint32 cont_capture_frame_length_kerr = 2486;
#endif

MSDK_SENSOR_CONFIG_STRUCT S5K4H5YXKERRSensorConfigData;

kal_uint32 S5K4H5YXKERR_FAC_SENSOR_REG;

MSDK_SCENARIO_ID_ENUM S5K4H5YXKERRCurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;

/* FIXME: old factors and DIDNOT use now. s*/
SENSOR_REG_STRUCT S5K4H5YXKERRSensorCCT[]=CAMERA_SENSOR_CCT_DEFAULT_VALUE;
SENSOR_REG_STRUCT S5K4H5YXKERRSensorReg[ENGINEER_END]=CAMERA_SENSOR_REG_DEFAULT_VALUE;
/* FIXME: old factors and DIDNOT use now. e*/

static S5K4H5YXKERR_PARA_STRUCT s5k4h5yxkerr;

static kal_uint16 S5K4H5YXKERR_read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte = 0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };

	iReadRegI2C(pu_send_cmd, 2, (u8 *)&get_byte, 1, S5K4H5YXKERRMIPI_WRITE_ID);

	return get_byte;
}



static void S5K4H5YXKERR_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};

	iWriteRegI2C(pu_send_cmd, 3, S5K4H5YXKERRMIPI_WRITE_ID);
}

#define Sleep(ms) mdelay(ms)


//20130129 add otp driver
//enble 3a00 bit[0] 1, enble , 0 disable
//reg0x3a02 elect page (0 - d)
//one paga have 64 byte (3a04 to 3a43 have 64 byte)
//20130123 jerry check:
//page 3 to page d, total 11 page for lsc use
//page 2 , total 1 page for awb use
//page 0, 1, total 2 page, for version message use

//page 2  for awb use
//R/G , B/G for 4 byte
//0x3a04  R/G H
//0x3a05  R/G L
//0x3a06  B/G H
//0x3a07  B/G L

#if 0
#if defined(S5K4H5YXKERR_USE_AWB_OTP)
//index:index of otp group.(0,1,2)
//return:   0:group index is empty.
//      1.group index has invalid data
    //      2.group index has valid data
kal_uint16 S5K4H5YXKERR_check_otp_wb(kal_uint16 index)
{
    kal_uint16 temp,flag;
    kal_uint32 address;

    OV5647MIPI_write_cmos_sensor(0x0100, 0x01);  
     mdelay(1);  
    //read otp into buffer
    OV5647MIPI_write_cmos_sensor(0x3d21,0x01);

    //read flag
    address = 0x3d05+index*9;
    flag = OV5647MIPI_read_cmos_sensor(address);

    //SENSORDB("OV5647MIPI_check_otp_wb=%d\r\n",flag);

    //do not clear OTP buffer
    //clear otp buffer
    //for(temp=0;temp<32;temp++){
    //  OV5647MIPI_write_cmos_sensor(0x3d00+temp,0x00);
    //}
    
    
    //disable otp read
    OV5647MIPI_write_cmos_sensor(0x3d21,0x0);
    OV5647MIPI_write_cmos_sensor(0x0100, 0x00); 

    if(NULL == flag)
        {
            
            SENSORDB("[OV5647MIPI_check_otp_awb]index[%x]read flag[%x][0]\n",index,flag);
            return 0;
            
        }
    else if(!(flag&0x80) && (flag&0x7f))
        {
            SENSORDB("[OV5647MIPI_check_otp_awb]index[%x]read flag[%x][2]\n",index,flag);
            return 2;
        }
    else
        {
            SENSORDB("[OV5647MIPI_check_otp_awb]index[%x]read flag[%x][1]\n",index,flag);
            return 1;
        }
    
}

//index:index of otp group.(0,1,2)
//return: 0
kal_uint16 OV5647MIPI_read_otp_wb(kal_uint16 index, struct OV5647MIPI_otp_struct *otp)
{
    kal_uint16 temp;
    kal_uint32 address;
       OV5647MIPI_write_cmos_sensor(0x0100, 0x01);   
     mdelay(1);  
    //read otp into buffer
    OV5647MIPI_write_cmos_sensor(0x3d21,0x01);

    address = 0x3d05 +index*9;

    ////4 modified the start address
    //address = 0x05 + index*9 +1;

    otp->customer_id = (OV5647MIPI_read_cmos_sensor(address)&0x7f);
    SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]module_integrator_id[%x]\n",address,otp->customer_id);
    
    otp->module_integrator_id = OV5647MIPI_read_cmos_sensor(address);
    SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]module_integrator_id[%x]\n",address,otp->module_integrator_id);
    
    otp->lens_id = OV5647MIPI_read_cmos_sensor(address+1);
    SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]lens_id[%x]\n",address,otp->lens_id);
    
    otp->rg_ratio = OV5647MIPI_read_cmos_sensor(address+2);
    SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]rg_ratio[%x]\n",address,otp->rg_ratio);

    otp->bg_ratio = OV5647MIPI_read_cmos_sensor(address+3);
    SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]bg_ratio[%x]\n",address,otp->bg_ratio);

    otp->user_data[0] = OV5647MIPI_read_cmos_sensor(address+4);
    SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]user_data[0][%x]\n",address,otp->user_data[0]);

    otp->user_data[1] = OV5647MIPI_read_cmos_sensor(address+5);
    SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]user_data[1][%x]\n",address,otp->user_data[1]);

    otp->user_data[2] = OV5647MIPI_read_cmos_sensor(address+6); 
    SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]user_data[2][%x]\n",address,otp->user_data[2]);

    otp->user_data[3] = OV5647MIPI_read_cmos_sensor(address+7); 
    SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]user_data[3][%x]\n",address,otp->user_data[3]);

    otp->user_data[4] = OV5647MIPI_read_cmos_sensor(address+8); 
    SENSORDB("[OV5647MIPI_read_otp_wb]address[%x]user_data[3][%x]\n",address,otp->user_data[3]);

    //disable otp read
    OV5647MIPI_write_cmos_sensor(0x3d21,00);
    
       OV5647MIPI_write_cmos_sensor(0x0100, 0x00);   
    
    //do not clear OTP buffer
        //clear otp buffer
    //for(temp=0;temp<32;temp++){
    //  OV5647MIPI_write_cmos_sensor(0x3d00+temp,0x00);
    //}

    return 0;
    
    
}

//R_gain: red gain of sensor AWB, 0x400 = 1
//G_gain: green gain of sensor AWB, 0x400 = 1
//B_gain: blue gain of sensor AWB, 0x400 = 1
//reutrn 0

//20130129
//020e(H) 020f (L), Gr 
//0210(H) 0211(L),  R
//0212(H) 0213 (L), B 
//0214(H) 0215(L),  Gb

kal_uint16 OV5647MIPI_update_wb_gain(kal_uint32 R_gain, kal_uint32 G_gain, kal_uint32 B_gain)
{

    SENSORDB("[OV5647MIPI_update_wb_gain]R_gain[%x]G_gain[%x]B_gain[%x]\n",R_gain,G_gain,B_gain);

    if(R_gain > 0x400)
        {
            OV5647MIPI_write_cmos_sensor(0x5186,R_gain >> 8);
            OV5647MIPI_write_cmos_sensor(0x5187,(R_gain&0x00ff));
        }
    if(G_gain > 0x400)
        {
            OV5647MIPI_write_cmos_sensor(0x5188,G_gain >> 8);
            OV5647MIPI_write_cmos_sensor(0x5189,(G_gain&0x00ff));
        }
    if(B_gain >0x400)
        {
            OV5647MIPI_write_cmos_sensor(0x518a,B_gain >> 8);
            OV5647MIPI_write_cmos_sensor(0x518b,(B_gain&0x00ff));
        }
    return 0;
}

//R/G and B/G ratio of typical camera module is defined here

kal_uint32 tRG_Ratio_typical = RG_TYPICAL;
kal_uint32 tBG_Ratio_typical = BG_TYPICAL;

//call this function after OV5647MIPI initialization
//return value: 0 update success
//              1 no    OTP

kal_uint16 OV5647MIPI_update_wb_register_from_otp(void)
{
    kal_uint16 temp, i, otp_index;
    struct OV5647MIPI_otp_struct current_otp;
    kal_uint32 R_gain, B_gain, G_gain, G_gain_R,G_gain_B;

    SENSORDB("OV5647MIPI_update_wb_register_from_otp\n");

    //update white balance setting from OTP
    //check first wb OTP with valid OTP
    for(i = 0; i < 3; i++)
        {
            temp = OV5647MIPI_check_otp_wb(i);
            if(temp == 2)
                {
                    otp_index = i;
                    break;
                }
        }
    if( 3 == i)
        {
            SENSORDB("[OV5647MIPI_update_wb_register_from_otp]no valid wb OTP data!\r\n");
            return 1;
        }
    OV5647MIPI_read_otp_wb(otp_index,&current_otp);

    //calculate gain
    //0x400 = 1x gain
    if(current_otp.bg_ratio < tBG_Ratio_typical)
        {
            if(current_otp.rg_ratio < tRG_Ratio_typical)
                {
                    //current_opt.bg_ratio < tBG_Ratio_typical &&
                    //cuttent_otp.rg < tRG_Ratio_typical

                    G_gain = 0x400;
                    B_gain = 0x400 * tBG_Ratio_typical / current_otp.bg_ratio;
                    R_gain = 0x400 * tRG_Ratio_typical / current_otp.rg_ratio;
                }
            else
                {
                    //current_otp.bg_ratio < tBG_Ratio_typical &&
                    //current_otp.rg_ratio >= tRG_Ratio_typical
                    R_gain = 0x400;
                    G_gain = 0x400 * current_otp.rg_ratio / tRG_Ratio_typical;
                    B_gain = G_gain * tBG_Ratio_typical / current_otp.bg_ratio;
                    
                    
                }
        }
    else
        {
            if(current_otp.rg_ratio < tRG_Ratio_typical)
                {
                    //current_otp.bg_ratio >= tBG_Ratio_typical &&
                    //current_otp.rg_ratio < tRG_Ratio_typical
                    B_gain = 0x400;
                    G_gain = 0x400 * current_otp.bg_ratio / tBG_Ratio_typical;
                    R_gain = G_gain * tRG_Ratio_typical / current_otp.rg_ratio;
                    
                }
            else
                {
                    //current_otp.bg_ratio >= tBG_Ratio_typical &&
                    //current_otp.rg_ratio >= tRG_Ratio_typical
                    G_gain_B = 0x400*current_otp.bg_ratio / tBG_Ratio_typical;
                    G_gain_R = 0x400*current_otp.rg_ratio / tRG_Ratio_typical;
                    
                    if(G_gain_B > G_gain_R)
                        {
                            B_gain = 0x400;
                            G_gain = G_gain_B;
                            R_gain = G_gain * tRG_Ratio_typical / current_otp.rg_ratio;
                        }
                    else

                        {
                            R_gain = 0x400;
                            G_gain = G_gain_R;
                            B_gain = G_gain * tBG_Ratio_typical / current_otp.bg_ratio;
                        }
                    
                }
            
        }
    //write sensor wb gain to register
    OV5647MIPI_update_wb_gain(R_gain,G_gain,B_gain);

    //success
    return 0;
}

#endif

#if defined(OV5647MIPI_USE_LENC_OTP)    //copy form OV5650 LENC OTP

//index:index of otp group.(0,1,2)
//return:   0.group index is empty.
//      1.group index has invalid data
//      2.group index has valid data

kal_uint16 OV5647MIPI_check_otp_lenc(kal_uint16 index)
{
   kal_uint16 temp,flag;
   kal_uint32 address;

   address = 0x20 + index*71;
   OV5647MIPI_write_cmos_sensor(0x3d00,address);
   
   flag = OV5647MIPI_read_cmos_sensor(0x3d04);
   flag = flag & 0xc0;

   OV5647MIPI_write_cmos_sensor(0x3d00,0);

   if(NULL == flag)
   {
        SENSORDB("[OV5647MIPI_check_otp_lenc]index[%x]read flag[%x][0]\n",index,flag);
       return 0;
   }
   else if(0x40 == flag)
   {
        SENSORDB("[OV5647MIPI_check_otp_lenc]index[%x]read flag[%x][2]\n",index,flag);
       return 2;
   }
   else
   {
        SENSORDB("[OV5647MIPI_check_otp_lenc]index[%x]read flag[%x][1]\n",index,flag);
        return 1;
   }
}


kal_uint16 OV5647MIPI_read_otp_lenc(kal_uint16 index,struct OV5647MIPI_otp_struct *otp)
{
    kal_uint16 bank,temp1,temp2,i;
    kal_uint32 address;

    address = 0x20 + index*71 +1;
    
    

    //read lenc_g
    for(i = 0; i < 36; i++)
        {
            OV5647MIPI_write_cmos_sensor(0x3d00,address);
            otp->lenc_g[i] = OV5647MIPI_read_cmos_sensor(0x3d04);
            
            SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]otp->lenc_g[%d][%x]\n",address,i,otp->lenc_g[i]);
            address++;
        }
    //read lenc_b
    for(i = 0; i <8; i++)
        {
            OV5647MIPI_write_cmos_sensor(0x3d00,address);
            temp1 = OV5647MIPI_read_cmos_sensor(0x3d04);

            SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);

            address++;
            OV5647MIPI_write_cmos_sensor(0x3d00,address);
            temp2 = OV5647MIPI_read_cmos_sensor(0x3d04);

            SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp2[%x]\n",address,temp2);

            address++;

            otp->lenc_b[i*3] = temp1&0x1f;
            otp->lenc_b[i*3+1] = temp2&0x1f;
            otp->lenc_b[i*3+2] = (((temp1 >> 2)&0x18) | (temp2 >> 5));
        }
    OV5647MIPI_write_cmos_sensor(0x3d00,address);
    temp1 = OV5647MIPI_read_cmos_sensor(0x3d04);
    SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);
    otp->lenc_b[24] = temp1&0x1f;
    address++;

    //read lenc_r
    for(i = 0; i <8; i++)
        {
           OV5647MIPI_write_cmos_sensor(0x3d00,address);
           temp1 = OV5647MIPI_read_cmos_sensor(0x3d04);

           SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);
           
           address++;

           OV5647MIPI_write_cmos_sensor(0x3d00,address);
           temp2 = OV5647MIPI_read_cmos_sensor(0x3d04);
           
            
           SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp2[%x]\n",address,temp2);
           address++;

           otp->lenc_r[i*3] = temp1&0x1f;
           otp->lenc_r[i*3+1] = temp2&0x1f;
           otp->lenc_r[i*3+2] = (((temp1 >> 2)&0x18) | (temp2 >>5));
        }
    OV5647MIPI_write_cmos_sensor(0x3d00,address);
    temp1 = OV5647MIPI_read_cmos_sensor(0x3d04);
    SENSORDB("[OV5647MIPI_read_otp_lenc]address[%x]temp1[%x]\n",address,temp1);
    otp->lenc_r[24] = temp1 & 0x1f;

    OV5647MIPI_write_cmos_sensor(0x3d00,0);

    return 0;
}


//return 0
kal_uint16 OV5647MIPI_update_lenc(struct OV5647MIPI_otp_struct *otp)
{
    kal_uint16 i, temp;
    //lenc g
    for(i = 0; i < 36; i++)
        {
            OV5647MIPI_write_cmos_sensor(0x5800+i,otp->lenc_g[i]);
            
            SENSORDB("[OV5647MIPI_update_lenc]otp->lenc_g[%d][%x]\n",i,otp->lenc_g[i]);
        }
    //lenc b
    for(i = 0; i < 25; i++)
        {
            OV5647MIPI_write_cmos_sensor(0x5824+i,otp->lenc_b[i]);
            SENSORDB("[OV5647MIPI_update_lenc]otp->lenc_b[%d][%x]\n",i,otp->lenc_b[i]);
        }
    //lenc r
    for(i = 0; i < 25; i++)
        {
            OV5647MIPI_write_cmos_sensor(0x583d+i,otp->lenc_r[i]);
            SENSORDB("[OV5647MIPI_update_lenc]otp->lenc_r[%d][%x]\n",i,otp->lenc_r[i]);
        }
    return 0;
}

//call this function after OV5647MIPI initialization
//return value: 0 update success
//              1 no otp

kal_uint16 OV5647MIPI_update_lenc_register_from_otp(void)
{
    kal_uint16 temp,i,otp_index;
    struct OV5647MIPI_otp_struct current_otp;

    for(i = 0; i < 3; i++)
        {
            temp = OV5647MIPI_check_otp_lenc(i);
            if(2 == temp)
                {
                    otp_index = i;
                    break;
                }
        }
    if(3 == i)
        {
            SENSORDB("[OV5647MIPI_update_lenc_register_from_otp]no valid wb OTP data!\r\n");
            return 1;
        }
    OV5647MIPI_read_otp_lenc(otp_index,&current_otp);

    OV5647MIPI_update_lenc(&current_otp);

    //at last should enable the shading enable register
    OV5647MIPI_read_cmos_sensor(0x5000);
    temp |= 0x80;
    OV5647MIPI_write_cmos_sensor(0x5000,temp);
    

    //success
    return 0;
}

#endif
#endif


void S5K4H5YXKERR_write_shutter(kal_uint32 shutter)
{
    kal_uint32 frame_length = 0;//,line_length=0;
    //kal_uint32 extra_lines = 0;
    //kal_uint32 max_exp_shutter = 0;
    //unsigned long flags;
    kal_uint32 line_length_read;

    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_write_shutter] shutter=%d\n", shutter);

  if (shutter < 3)
      shutter = 3;

  if (s5k4h5yxkerr.sensorMode == SENSOR_MODE_PREVIEW) 
  {
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_write_shutter] SENSOR_MODE_PREVIEW \n");
      if(shutter > (cont_preview_frame_length_kerr - 16))
          frame_length = shutter + 16;
      else 
          frame_length = cont_preview_frame_length_kerr;

  }
  else if(s5k4h5yxkerr.sensorMode==SENSOR_MODE_VIDEO)
  {
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_write_shutter] SENSOR_MODE_VIDEO \n");
       frame_length = s5k4h5yxkerr_video_frame_length;
       if(shutter > (frame_length - 16))
       shutter = frame_length - 16;
  }
  else
  {
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_write_shutter] SENSOR_MODE_CAPTURE \n");
      if(shutter > (cont_capture_frame_length_kerr - 16))
          frame_length = shutter + 16;
      else 
          frame_length = cont_capture_frame_length_kerr;
  }
  
    S5K4H5YXKERR_write_cmos_sensor(0x0104, 0x01);    //Grouped parameter hold    

    S5K4H5YXKERR_write_cmos_sensor(0x0340, (frame_length >>8) & 0xFF);
    S5K4H5YXKERR_write_cmos_sensor(0x0341, frame_length & 0xFF);    

    S5K4H5YXKERR_write_cmos_sensor(0x0202, (shutter >> 8) & 0xFF);
    S5K4H5YXKERR_write_cmos_sensor(0x0203, shutter  & 0xFF);
 
    S5K4H5YXKERR_write_cmos_sensor(0x0104, 0x00);    //Grouped parameter release

    line_length_read = ((S5K4H5YXKERR_read_cmos_sensor(0x0342)<<8)+S5K4H5YXKERR_read_cmos_sensor(0x0343));

    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_write_shutter] shutter=%d,  line_length_read=%d, frame_length=%d\n", shutter, line_length_read, frame_length);
}   /* write_S5K4H5YXKERR_shutter */


void write_S5K4H5YXKERR_gain(kal_uint16 gain)
{
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [write_S5K4H5YXKERR_gain] gain=%d\n", gain);
    S5K4H5YXKERR_write_cmos_sensor(0x0104, 0x01);   
    S5K4H5YXKERR_write_cmos_sensor(0x0204,(gain>>8));
    S5K4H5YXKERR_write_cmos_sensor(0x0205,(gain&0xff));
    S5K4H5YXKERR_write_cmos_sensor(0x0104, 0x00);
    return;
}

/*************************************************************************
* FUNCTION
*    S5K4H5YXKERR_SetGain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    gain : sensor global gain(base: 0x40)
*
* RETURNS
*    the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
void S5K4H5YXKERR_SetGain(UINT16 iGain)
{
    unsigned long flags;
    spin_lock_irqsave(&s5k4h5yxkerrmipiraw_drv_lock,flags);
    s5k4h5yxkerr.realGain = iGain;
    spin_unlock_irqrestore(&s5k4h5yxkerrmipiraw_drv_lock,flags);
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_SetGain] gain=%d\n", iGain);
    write_S5K4H5YXKERR_gain(iGain);
}   /*  S5K4H5YXKERR_SetGain_SetGain  */


/*************************************************************************
* FUNCTION
*    read_S5K4H5YXKERR_gain
*
* DESCRIPTION
*    This function is to set global gain to sensor.
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 read_S5K4H5YXKERR_gain(void)
{
    kal_uint16 read_gain=0;

    read_gain=((S5K4H5YXKERR_read_cmos_sensor(0x0204) << 8) | S5K4H5YXKERR_read_cmos_sensor(0x0205));
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [read_S5K4H5YXKERR_gain] gain=%d\n", read_gain);
    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
    s5k4h5yxkerr.sensorGlobalGain = read_gain;
    spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
    return s5k4h5yxkerr.sensorGlobalGain;
}  /* read_S5K4H5YXKERR_gain */


void S5K4H5YXKERR_camera_para_to_sensor(void)
{
    kal_uint32    i;
    for(i=0; 0xFFFFFFFF!=S5K4H5YXKERRSensorReg[i].Addr; i++)
    {
        S5K4H5YXKERR_write_cmos_sensor(S5K4H5YXKERRSensorReg[i].Addr, S5K4H5YXKERRSensorReg[i].Para);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=S5K4H5YXKERRSensorReg[i].Addr; i++)
    {
        S5K4H5YXKERR_write_cmos_sensor(S5K4H5YXKERRSensorReg[i].Addr, S5K4H5YXKERRSensorReg[i].Para);
    }
    for(i=FACTORY_START_ADDR; i<FACTORY_END_ADDR; i++)
    {
        S5K4H5YXKERR_write_cmos_sensor(S5K4H5YXKERRSensorCCT[i].Addr, S5K4H5YXKERRSensorCCT[i].Para);
    }
}


/*************************************************************************
* FUNCTION
*    S5K4H5YXKERR_sensor_to_camera_para
*
* DESCRIPTION
*    // update camera_para from sensor register
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
void S5K4H5YXKERR_sensor_to_camera_para(void)
{
    kal_uint32    i, temp_data;
    for(i=0; 0xFFFFFFFF!=S5K4H5YXKERRSensorReg[i].Addr; i++)
    {
         temp_data = S5K4H5YXKERR_read_cmos_sensor(S5K4H5YXKERRSensorReg[i].Addr);
         spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
         S5K4H5YXKERRSensorReg[i].Para =temp_data;
         spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
    }
    for(i=ENGINEER_START_ADDR; 0xFFFFFFFF!=S5K4H5YXKERRSensorReg[i].Addr; i++)
    {
        temp_data = S5K4H5YXKERR_read_cmos_sensor(S5K4H5YXKERRSensorReg[i].Addr);
        spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
        S5K4H5YXKERRSensorReg[i].Para = temp_data;
        spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
    }
}

/*************************************************************************
* FUNCTION
*    S5K4H5YXKERR_get_sensor_group_count
*
* DESCRIPTION
*    //
*
* PARAMETERS
*    None
*
* RETURNS
*    gain : sensor global gain(base: 0x40)
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_int32  S5K4H5YXKERR_get_sensor_group_count(void)
{
    return GROUP_TOTAL_NUMS;
}

void S5K4H5YXKERR_get_sensor_group_info(kal_uint16 group_idx, kal_int8* group_name_ptr, kal_int32* item_count_ptr)
{
   switch (group_idx)
   {
        case PRE_GAIN:
            sprintf((char *)group_name_ptr, "CCT");
            *item_count_ptr = 2;
            break;
        case CMMCLK_CURRENT:
            sprintf((char *)group_name_ptr, "CMMCLK Current");
            *item_count_ptr = 1;
            break;
        case FRAME_RATE_LIMITATION:
            sprintf((char *)group_name_ptr, "Frame Rate Limitation");
            *item_count_ptr = 2;
            break;
        case REGISTER_EDITOR:
            sprintf((char *)group_name_ptr, "Register Editor");
            *item_count_ptr = 2;
            break;
        default:
            ASSERT(0);
    }
}

void S5K4H5YXKERR_get_sensor_item_info(kal_uint16 group_idx,kal_uint16 item_idx, MSDK_SENSOR_ITEM_INFO_STRUCT* info_ptr)
{
    kal_int16 temp_reg=0;
    kal_uint16 temp_gain=0, temp_addr=0, temp_para=0;

    switch (group_idx)
    {
        case PRE_GAIN:
           switch (item_idx)
          {
              case 0:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-R");
                  temp_addr = PRE_GAIN_R_INDEX;
              break;
              case 1:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gr");
                  temp_addr = PRE_GAIN_Gr_INDEX;
              break;
              case 2:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-Gb");
                  temp_addr = PRE_GAIN_Gb_INDEX;
              break;
              case 3:
                sprintf((char *)info_ptr->ItemNamePtr,"Pregain-B");
                  temp_addr = PRE_GAIN_B_INDEX;
              break;
              case 4:
                 sprintf((char *)info_ptr->ItemNamePtr,"SENSOR_BASEGAIN");
                 temp_addr = SENSOR_BASEGAIN;
              break;
              default:
                 ASSERT(0);
          }

            temp_para= S5K4H5YXKERRSensorCCT[temp_addr].Para;
            //temp_gain= (temp_para/s5k4h5yxkerr.sensorBaseGain) * 1000;

            info_ptr->ItemValue=temp_gain;
            info_ptr->IsTrueFalse=KAL_FALSE;
            info_ptr->IsReadOnly=KAL_FALSE;
            info_ptr->IsNeedRestart=KAL_FALSE;
            info_ptr->Min= S5K4H5YXKERR_MIN_ANALOG_GAIN * 1000;
            info_ptr->Max= S5K4H5YXKERR_MAX_ANALOG_GAIN * 1000;
            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Drv Cur[2,4,6,8]mA");

                    //temp_reg=MT9P017SensorReg[CMMCLK_CURRENT_INDEX].Para;
                    temp_reg = ISP_DRIVING_2MA;
                    if(temp_reg==ISP_DRIVING_2MA)
                    {
                        info_ptr->ItemValue=2;
                    }
                    else if(temp_reg==ISP_DRIVING_4MA)
                    {
                        info_ptr->ItemValue=4;
                    }
                    else if(temp_reg==ISP_DRIVING_6MA)
                    {
                        info_ptr->ItemValue=6;
                    }
                    else if(temp_reg==ISP_DRIVING_8MA)
                    {
                        info_ptr->ItemValue=8;
                    }

                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_TRUE;
                    info_ptr->Min=2;
                    info_ptr->Max=8;
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case FRAME_RATE_LIMITATION:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"Max Exposure Lines");
                    info_ptr->ItemValue=    111;  //MT9P017_MAX_EXPOSURE_LINES;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_TRUE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0;
                    break;
                case 1:
                    sprintf((char *)info_ptr->ItemNamePtr,"Min Frame Rate");
                    info_ptr->ItemValue=12;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_TRUE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0;
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case REGISTER_EDITOR:
            switch (item_idx)
            {
                case 0:
                    sprintf((char *)info_ptr->ItemNamePtr,"REG Addr.");
                    info_ptr->ItemValue=0;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0xFFFF;
                    break;
                case 1:
                    sprintf((char *)info_ptr->ItemNamePtr,"REG Value");
                    info_ptr->ItemValue=0;
                    info_ptr->IsTrueFalse=KAL_FALSE;
                    info_ptr->IsReadOnly=KAL_FALSE;
                    info_ptr->IsNeedRestart=KAL_FALSE;
                    info_ptr->Min=0;
                    info_ptr->Max=0xFFFF;
                    break;
                default:
                ASSERT(0);
            }
            break;
        default:
            ASSERT(0);
    }
}



kal_bool S5K4H5YXKERR_set_sensor_item_info(kal_uint16 group_idx, kal_uint16 item_idx, kal_int32 ItemValue)
{
//   kal_int16 temp_reg;
   kal_uint16  temp_gain=0,temp_addr=0, temp_para=0;

   switch (group_idx)
    {
        case PRE_GAIN:
            switch (item_idx)
            {
              case 0:
                temp_addr = PRE_GAIN_R_INDEX;
              break;
              case 1:
                temp_addr = PRE_GAIN_Gr_INDEX;
              break;
              case 2:
                temp_addr = PRE_GAIN_Gb_INDEX;
              break;
              case 3:
                temp_addr = PRE_GAIN_B_INDEX;
              break;
              case 4:
                temp_addr = SENSOR_BASEGAIN;
              break;
              default:
                 ASSERT(0);
          }

         temp_gain=((ItemValue*BASEGAIN+500)/1000);         //+500:get closed integer value

          if(temp_gain>=1*BASEGAIN && temp_gain<=16*BASEGAIN)
          {
//             temp_para=(temp_gain * s5k4h5yxkerr.sensorBaseGain + BASEGAIN/2)/BASEGAIN;
          }
          else
              ASSERT(0);
          spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
          S5K4H5YXKERRSensorCCT[temp_addr].Para = temp_para;
          spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
          S5K4H5YXKERR_write_cmos_sensor(S5K4H5YXKERRSensorCCT[temp_addr].Addr,temp_para);

            break;
        case CMMCLK_CURRENT:
            switch (item_idx)
            {
                case 0:
                    //no need to apply this item for driving current
                    break;
                default:
                    ASSERT(0);
            }
            break;
        case FRAME_RATE_LIMITATION:
            ASSERT(0);
            break;
        case REGISTER_EDITOR:
            switch (item_idx)
            {
                case 0:
                    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
                    S5K4H5YXKERR_FAC_SENSOR_REG=ItemValue;
                    spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
                    break;
                case 1:
                    S5K4H5YXKERR_write_cmos_sensor(S5K4H5YXKERR_FAC_SENSOR_REG,ItemValue);
                    break;
                default:
                    ASSERT(0);
            }
            break;
        default:
            ASSERT(0);
    }
    return KAL_TRUE;
}

static void S5K4H5YXKERR_SetDummy( const kal_uint32 iPixels, const kal_uint32 iLines )
{
kal_uint16 line_length = 0;
kal_uint16 frame_length = 0;

S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_SetDummy] iPixels=%d\n", iPixels);
S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_SetDummy] iLines=%d\n", iLines);

if ( SENSOR_MODE_PREVIEW == s5k4h5yxkerr.sensorMode )   
{
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_SetDummy] SENSOR_MODE_PREVIEW\n");
    line_length = s5k4h5yxkerr_preview_line_length ;
    frame_length = s5k4h5yxkerr_preview_frame_length + iLines;
}
else if( SENSOR_MODE_VIDEO == s5k4h5yxkerr.sensorMode )     
{
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_SetDummy] SENSOR_MODE_VIDEO\n");
    line_length = s5k4h5yxkerr_video_line_length;
    frame_length = s5k4h5yxkerr_video_frame_length;
}
else
{
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_SetDummy] SENSOR_MODE_CAPTURE\n");
    line_length = s5k4h5yxkerr_capture_line_length ;
    frame_length = s5k4h5yxkerr_capture_frame_length + iLines;
}

if(s5k4h5yxkerr.maxExposureLines > frame_length - 16)
{
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_SetDummy] maxExposureLines > frame_length - 16\n");
    return;
}

ASSERT(line_length < S5K4H5YXKERR_MAX_LINE_LENGTH);     //0xCCCC
ASSERT(frame_length < S5K4H5YXKERR_MAX_FRAME_LENGTH);   //0xFFFF

S5K4H5YXKERR_write_cmos_sensor(0x0104, 0x01);   //Grouped parameter hold

//Set total frame length
S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_SetDummy] frame_length = %d\n", frame_length);

S5K4H5YXKERR_write_cmos_sensor(0x0340, (frame_length >> 8) & 0xFF);
S5K4H5YXKERR_write_cmos_sensor(0x0341, frame_length & 0xFF);

//Set total line length
S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_SetDummy] line_length = %d\n", line_length);

S5K4H5YXKERR_write_cmos_sensor(0x0342, (line_length >> 8) & 0xFF);
S5K4H5YXKERR_write_cmos_sensor(0x0343, line_length & 0xFF);

S5K4H5YXKERR_write_cmos_sensor(0x0104, 0x00);   //Grouped parameter release
}   /*  S5K4H5YXKERR_SetDummy */

void S5K4H5YXKERRPreviewSetting(void)
{
   S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRPreviewSetting]\n");
   #ifdef Capture8M_New
   S5K4H5YXKERR_write_cmos_sensor(0x0100, 0x00);       //  [0] mode_select
    S5K4H5YXKERR_write_cmos_sensor(0x0101, 0x00);       //  [1:0]   image_orientation ([0] mirror en, [1] flip en)
    S5K4H5YXKERR_write_cmos_sensor(0x0204, 0x00);       //  [15:8]  analogue_gain_code_global H
    S5K4H5YXKERR_write_cmos_sensor(0x0205, 0x20);       //  [7:0]   analogue_gain_code_global L
    S5K4H5YXKERR_write_cmos_sensor(0x0200, 0x0C);       //  [15:8]  fine_integration_time H  //0x0D--->0x0C MODIFY 20130313
    
    S5K4H5YXKERR_write_cmos_sensor(0x0201, 0x78);       //  [7:0]   fine_integration_time L
    S5K4H5YXKERR_write_cmos_sensor(0x0202, 0x04);       //  [15:8]  coarse_integration_time H
    S5K4H5YXKERR_write_cmos_sensor(0x0203, 0xE2);       //  [7:0]   coarse_integration_time L
    S5K4H5YXKERR_write_cmos_sensor(0x0340, 0x08);       //  [15:8]  frame_length_lines H
    S5K4H5YXKERR_write_cmos_sensor(0x0341, 0xD4);       //  [7:0]   frame_length_lines L
    S5K4H5YXKERR_write_cmos_sensor(0x0342, 0x1C);       //  [15:8]  line_length_pck H
    S5K4H5YXKERR_write_cmos_sensor(0x0343, 0xD0);       //  [7:0]   line_length_pck L
    S5K4H5YXKERR_write_cmos_sensor(0x0344, 0x00);       //  [11:8]  x_addr_start H
    S5K4H5YXKERR_write_cmos_sensor(0x0345, 0x00);       //  [7:0]   x_addr_start L
    S5K4H5YXKERR_write_cmos_sensor(0x0346, 0x00);       //  [11:8]  y_addr_start H
    S5K4H5YXKERR_write_cmos_sensor(0x0347, 0x00);       //  [7:0]   y_addr_start L
    S5K4H5YXKERR_write_cmos_sensor(0x0348, 0x0C);       //  [11:8]  x_addr_end H
    S5K4H5YXKERR_write_cmos_sensor(0x0349, 0xD1);       //  [7:0]   x_addr_end L
    S5K4H5YXKERR_write_cmos_sensor(0x034A, 0x09);       //  [11:8]  y_addr_end H
    S5K4H5YXKERR_write_cmos_sensor(0x034B, 0x9F);       //  [7:0]   y_addr_end L
    S5K4H5YXKERR_write_cmos_sensor(0x034C, 0x06);       //  [11:8]  x_output_size H
    S5K4H5YXKERR_write_cmos_sensor(0x034D, 0x68);       //  [7:0]   x_output_size L
    S5K4H5YXKERR_write_cmos_sensor(0x034E, 0x04);       //  [11:8]  y_output_size H
    S5K4H5YXKERR_write_cmos_sensor(0x034F, 0xD0);       //  [7:0]   y_output_size L
    S5K4H5YXKERR_write_cmos_sensor(0x0390, 0x01);       //  [7:0]   binning_mode ([0] binning enable)
    S5K4H5YXKERR_write_cmos_sensor(0x0391, 0x22);       //  [7:0]   binning_type (22h : 2x2 binning, 44h : 4x4 binning)
    S5K4H5YXKERR_write_cmos_sensor(0x0381, 0x01);       //  [4:0]   x_even_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0383, 0x03);       //  [4:0]   x_odd_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0385, 0x01);       //  [4:0]   y_even_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0387, 0x03);       //  [4:0]   y_odd_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0301, 0x02);       //  [3:0]   vt_pix_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x0303, 0x01);       //  [3:0]   vt_sys_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x0305, 0x06);       //  [5:0]   pre_pll_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x0306, 0x00);       //  [9:8]   pll_multiplier H
    S5K4H5YXKERR_write_cmos_sensor(0x0307, 0x7D);       //  [7:0]   pll_multiplier L
    S5K4H5YXKERR_write_cmos_sensor(0x0309, 0x02);       //  [3:0]   op_pix_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x030B, 0x01);       //  [3:0]   op_sys_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x3C59, 0x00);       //  [2:0]   reg_PLL_S
    S5K4H5YXKERR_write_cmos_sensor(0x030D, 0x06);       //  [5:0]   out_pre_pll_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x030E, 0x00);       //  [9:8]   out_pll_multiplier H
    S5K4H5YXKERR_write_cmos_sensor(0x030F, 0xA8);       //A5        //  [7:0]   out_pll_multiplier L
    S5K4H5YXKERR_write_cmos_sensor(0x3C5A, 0x00);       //  [2:0]   reg_out_PLL_S
    S5K4H5YXKERR_write_cmos_sensor(0x0310, 0x01);       //  [0] pll_mode (01h : 2-PLL, 00h : 1-PLL)
    S5K4H5YXKERR_write_cmos_sensor(0x3C50, 0x53);       //  [7:4]   reg_DIV_DBR
    S5K4H5YXKERR_write_cmos_sensor(0x3C62, 0x02);       //  [31:24] requested_link_bit_rate_mbps HH
    S5K4H5YXKERR_write_cmos_sensor(0x3C63, 0xA0);       //  [23:16] requested_link_bit_rate_mbps HL
    S5K4H5YXKERR_write_cmos_sensor(0x3C64, 0x00);       //  [15:8]  requested_link_bit_rate_mbps LH
    S5K4H5YXKERR_write_cmos_sensor(0x3C65, 0x00);       //  [7:0]   requested_link_bit_rate_mbps LL
    S5K4H5YXKERR_write_cmos_sensor(0x3C1E, 0x00);       //  [3] reg_isp_fe_TN_SMIA_sync_sel
    S5K4H5YXKERR_write_cmos_sensor(0x0202, 0x00);   //coarse_integration_time_up
    S5K4H5YXKERR_write_cmos_sensor(0x302A, 0x0A);   //vda_width
    S5K4H5YXKERR_write_cmos_sensor(0x303D, 0x06);   //off_rst   //06-->04  //modify for powernoise 0315
    
       S5K4H5YXKERR_write_cmos_sensor(0x0114, 0x01);  //mipi 2 lane
    
    S5K4H5YXKERR_write_cmos_sensor(0x304B, 0x2A);   //ADC_SAT 490mV
    S5K4H5YXKERR_write_cmos_sensor(0x0204, 0x02);   //analog gain x16
    S5K4H5YXKERR_write_cmos_sensor(0x0205, 0x00);   //analog gain x16
    S5K4H5YXKERR_write_cmos_sensor(0x3205, 0x84);   //adc_offset_odd0
    S5K4H5YXKERR_write_cmos_sensor(0x3207, 0x85);   //adc_offset_even1
    S5K4H5YXKERR_write_cmos_sensor(0x3214, 0x94);   //adc_offset_odd0_MS
    S5K4H5YXKERR_write_cmos_sensor(0x3216, 0x95);   //adc_offset_even1_MS
    S5K4H5YXKERR_write_cmos_sensor(0x303a, 0x9f);   //clp_lvl
    S5K4H5YXKERR_write_cmos_sensor(0x3201, 0x07);   //dither_sel[2]: fob, dither_sel[1]: lob, dither_sel[0]: active
    S5K4H5YXKERR_write_cmos_sensor(0x3051, 0xff);   //blst 
    S5K4H5YXKERR_write_cmos_sensor(0x3052, 0xff);   //blst
    S5K4H5YXKERR_write_cmos_sensor(0x3054, 0xF0);   //rdv_option[7]=1 (caution of address overwrite)
    S5K4H5YXKERR_write_cmos_sensor(0x305C, 0x8F);   //cds_option (reduce s3/s4 buffer strength, s4_rpt_enable)
    S5K4H5YXKERR_write_cmos_sensor(0x302D, 0x7F); //[4] dshut_en=1
    
    //sensor anolog setting  modify for power noise 20130315
    #if 1
    S5K4H5YXKERR_write_cmos_sensor(0x305E, 0x11); //
    S5K4H5YXKERR_write_cmos_sensor(0x305F, 0x11); //
    S5K4H5YXKERR_write_cmos_sensor(0x3060, 0x10); //
    S5K4H5YXKERR_write_cmos_sensor(0x3091, 0x03); //
    S5K4H5YXKERR_write_cmos_sensor(0x3092, 0x03); //
    S5K4H5YXKERR_write_cmos_sensor(0x303D, 0x05); //
    S5K4H5YXKERR_write_cmos_sensor(0x3038, 0x99); //
    #endif
    
    S5K4H5YXKERR_write_cmos_sensor(0x3B29, 0x01); //OTP enable
    
    S5K4H5YXKERR_write_cmos_sensor(0x3903, 0x1F);
    
    //20130227 add new setting start
    S5K4H5YXKERR_write_cmos_sensor(0x3002, 0x0d);
    S5K4H5YXKERR_write_cmos_sensor(0x300a, 0x0d);
    S5K4H5YXKERR_write_cmos_sensor(0x3045, 0x04);
    S5K4H5YXKERR_write_cmos_sensor(0x300c, 0x78);
    S5K4H5YXKERR_write_cmos_sensor(0x300d, 0x80);
    S5K4H5YXKERR_write_cmos_sensor(0x305c, 0x82);
    S5K4H5YXKERR_write_cmos_sensor(0x3010, 0x0a);
    //20130227 add new setting  end
    S5K4H5YXKERR_write_cmos_sensor(0x0100, 0x01);       //  [0] mode_select
   #else
   S5K4H5YXKERR_write_cmos_sensor(0x0100, 0x00);    //  [0] mode_select
   S5K4H5YXKERR_write_cmos_sensor(0x0101, 0x00);    //  [1:0]   image_orientation ([0] mirror en, [1] flip en)
   S5K4H5YXKERR_write_cmos_sensor(0x0204, 0x00);    //  [15:8]  analogue_gain_code_global H
   S5K4H5YXKERR_write_cmos_sensor(0x0205, 0x20);    //  [7:0]   analogue_gain_code_global L
   S5K4H5YXKERR_write_cmos_sensor(0x0200, 0x0C);    //  [15:8]  fine_integration_time H  //0x0D--->0x0C MODIFY 20130313
   
   S5K4H5YXKERR_write_cmos_sensor(0x0201, 0x78);    //  [7:0]   fine_integration_time L
   S5K4H5YXKERR_write_cmos_sensor(0x0202, 0x04);    //  [15:8]  coarse_integration_time H
   S5K4H5YXKERR_write_cmos_sensor(0x0203, 0xE2);    //  [7:0]   coarse_integration_time L
   S5K4H5YXKERR_write_cmos_sensor(0x0340, 0x08);//04    //  [15:8]  frame_length_lines H
   S5K4H5YXKERR_write_cmos_sensor(0x0341, 0xD4);//EE    //  [7:0]   frame_length_lines L
   S5K4H5YXKERR_write_cmos_sensor(0x0342, 0x1C);    //  [15:8]  line_length_pck H
   S5K4H5YXKERR_write_cmos_sensor(0x0343, 0xD0);    //  [7:0]   line_length_pck L
   S5K4H5YXKERR_write_cmos_sensor(0x0344, 0x00);    //  [11:8]  x_addr_start H
   S5K4H5YXKERR_write_cmos_sensor(0x0345, 0x08);    //  [7:0]   x_addr_start L
   S5K4H5YXKERR_write_cmos_sensor(0x0346, 0x00);    //  [11:8]  y_addr_start H
   S5K4H5YXKERR_write_cmos_sensor(0x0347, 0x06);    //  [7:0]   y_addr_start L
   S5K4H5YXKERR_write_cmos_sensor(0x0348, 0x0C);    //  [11:8]  x_addr_end H
   S5K4H5YXKERR_write_cmos_sensor(0x0349, 0xC7);    //  [7:0]   x_addr_end L
   S5K4H5YXKERR_write_cmos_sensor(0x034A, 0x09);    //  [11:8]  y_addr_end H
   S5K4H5YXKERR_write_cmos_sensor(0x034B, 0x99);    //  [7:0]   y_addr_end L
   S5K4H5YXKERR_write_cmos_sensor(0x034C, 0x06);    //  [11:8]  x_output_size H
   S5K4H5YXKERR_write_cmos_sensor(0x034D, 0x60);    //  [7:0]   x_output_size L
   S5K4H5YXKERR_write_cmos_sensor(0x034E, 0x04);    //  [11:8]  y_output_size H
   S5K4H5YXKERR_write_cmos_sensor(0x034F, 0xCA);    //  [7:0]   y_output_size L
   S5K4H5YXKERR_write_cmos_sensor(0x0390, 0x01);    //  [7:0]   binning_mode ([0] binning enable)
   S5K4H5YXKERR_write_cmos_sensor(0x0391, 0x22);    //  [7:0]   binning_type (22h : 2x2 binning, 44h : 4x4 binning)
   S5K4H5YXKERR_write_cmos_sensor(0x0381, 0x01);    //  [4:0]   x_even_inc
   S5K4H5YXKERR_write_cmos_sensor(0x0383, 0x03);    //  [4:0]   x_odd_inc
   S5K4H5YXKERR_write_cmos_sensor(0x0385, 0x01);    //  [4:0]   y_even_inc
   S5K4H5YXKERR_write_cmos_sensor(0x0387, 0x03);    //  [4:0]   y_odd_inc
   S5K4H5YXKERR_write_cmos_sensor(0x0301, 0x02);   //  [3:0]   vt_pix_clk_div                     
   S5K4H5YXKERR_write_cmos_sensor(0x0303, 0x01);   //  [3:0]   vt_sys_clk_div                     
   S5K4H5YXKERR_write_cmos_sensor(0x0305, 0x06);   //  [5:0]   pre_pll_clk_div                        
   S5K4H5YXKERR_write_cmos_sensor(0x0306, 0x00);   //  [9:8]   pll_multiplier H                   
   S5K4H5YXKERR_write_cmos_sensor(0x0307, 0x7D);   //  [7:0]   pll_multiplier L                   
   S5K4H5YXKERR_write_cmos_sensor(0x0309, 0x02);   //  [3:0]   op_pix_clk_div                     
   S5K4H5YXKERR_write_cmos_sensor(0x030B, 0x01);   //  [3:0]   op_sys_clk_div                     
   S5K4H5YXKERR_write_cmos_sensor(0x3C59, 0x00);   //  [2:0]   reg_PLL_S                          
   S5K4H5YXKERR_write_cmos_sensor(0x030D, 0x06);   //  [5:0]   out_pre_pll_clk_div                    
   S5K4H5YXKERR_write_cmos_sensor(0x030E, 0x00);   //  [9:8]   out_pll_multiplier H               
   S5K4H5YXKERR_write_cmos_sensor(0x030F, 0xA8);   //A5    //  [7:0]   out_pll_multiplier L 
   S5K4H5YXKERR_write_cmos_sensor(0x3C5A, 0x00);   //  [2:0]   reg_out_PLL_S                      
   S5K4H5YXKERR_write_cmos_sensor(0x0310, 0x01);   //  [0] pll_mode (01h : 2-PLL, 00h : 1-PLL)   
   S5K4H5YXKERR_write_cmos_sensor(0x3C50, 0x53);   //  [7:4]   reg_DIV_DBR                            
   S5K4H5YXKERR_write_cmos_sensor(0x3C62, 0x02);   //  [31:24] requested_link_bit_rate_mbps HH       
   S5K4H5YXKERR_write_cmos_sensor(0x3C63, 0xA0);   //  [23:16] requested_link_bit_rate_mbps HL       
   S5K4H5YXKERR_write_cmos_sensor(0x3C64, 0x00);   //  [15:8]  requested_link_bit_rate_mbps LH       
   S5K4H5YXKERR_write_cmos_sensor(0x3C65, 0x00);   //  [7:0]   requested_link_bit_rate_mbps LL    
   S5K4H5YXKERR_write_cmos_sensor(0x3C1E, 0x00);    //  [3] reg_isp_fe_TN_SMIA_sync_se
   S5K4H5YXKERR_write_cmos_sensor(0x0202, 0x00);
   S5K4H5YXKERR_write_cmos_sensor(0x302A, 0x0A);
   S5K4H5YXKERR_write_cmos_sensor(0x303D, 0x06); //off_rst   //06-->04  //modify for powernoise 0315

        S5K4H5YXKERR_write_cmos_sensor(0x0114, 0x01);  //mipi 2 lane
        
           
   S5K4H5YXKERR_write_cmos_sensor(0x304B, 0x2A);
   S5K4H5YXKERR_write_cmos_sensor(0x0204, 0x02);
   S5K4H5YXKERR_write_cmos_sensor(0x0205, 0x00);
   S5K4H5YXKERR_write_cmos_sensor(0x3205, 0x84);
   S5K4H5YXKERR_write_cmos_sensor(0x3207, 0x85);
   S5K4H5YXKERR_write_cmos_sensor(0x3214, 0x94);
   S5K4H5YXKERR_write_cmos_sensor(0x3216, 0x95);
   S5K4H5YXKERR_write_cmos_sensor(0x303a, 0x9f);
   S5K4H5YXKERR_write_cmos_sensor(0x3201, 0x07);
   S5K4H5YXKERR_write_cmos_sensor(0x3051, 0xff);
   S5K4H5YXKERR_write_cmos_sensor(0x3052, 0xff);
   S5K4H5YXKERR_write_cmos_sensor(0x3054, 0xF0);
   S5K4H5YXKERR_write_cmos_sensor(0x305C, 0x8F);
   S5K4H5YXKERR_write_cmos_sensor(0x302D, 0x7F);

   //sensor anolog setting  modify for power noise 20130315
   #if 1
    S5K4H5YXKERR_write_cmos_sensor(0x305E, 0x11); //
    S5K4H5YXKERR_write_cmos_sensor(0x305F, 0x11); //
    S5K4H5YXKERR_write_cmos_sensor(0x3060, 0x10); //
    S5K4H5YXKERR_write_cmos_sensor(0x3091, 0x03); //
    S5K4H5YXKERR_write_cmos_sensor(0x3092, 0x03); //
    S5K4H5YXKERR_write_cmos_sensor(0x303D, 0x05); //
    S5K4H5YXKERR_write_cmos_sensor(0x3038, 0x99); //

    #endif
    
   S5K4H5YXKERR_write_cmos_sensor(0x3B29, 0x01); //OTP enable
   
   S5K4H5YXKERR_write_cmos_sensor(0x3903, 0x1F);
   //20130227 add new setting start
    S5K4H5YXKERR_write_cmos_sensor(0x3002, 0x0d);
    S5K4H5YXKERR_write_cmos_sensor(0x300a, 0x0d);
    S5K4H5YXKERR_write_cmos_sensor(0x3045, 0x04);
    S5K4H5YXKERR_write_cmos_sensor(0x300c, 0x78);
    S5K4H5YXKERR_write_cmos_sensor(0x300d, 0x80);
    S5K4H5YXKERR_write_cmos_sensor(0x305c, 0x82);
    S5K4H5YXKERR_write_cmos_sensor(0x3010, 0x0a);
    //20130227 add new setting end
   S5K4H5YXKERR_write_cmos_sensor(0x0100, 0x01);    //  [0] mode_select
   #endif
}

    
void S5K4H5YXKERRVideoSetting(void)
{
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRVideoSetting]\n");
    S5K4H5YXKERR_write_cmos_sensor(0x0100, 0x00);        //  [0] mode_select
    S5K4H5YXKERR_write_cmos_sensor(0x0101, 0x00);        //  [1:0]   image_orientation ([0] mirror en, [1] flip en)
    S5K4H5YXKERR_write_cmos_sensor(0x0204, 0x00);        //  [15:8]  analogue_gain_code_global H
    S5K4H5YXKERR_write_cmos_sensor(0x0205, 0x20);        //  [7:0]   analogue_gain_code_global L
    S5K4H5YXKERR_write_cmos_sensor(0x0200, 0x0D);        //  [15:8]  fine_integration_time H
    S5K4H5YXKERR_write_cmos_sensor(0x0201, 0xE8);        //  [7:0]   fine_integration_time L  //0x78--->0xE8 MODIFY 20130313
    
    S5K4H5YXKERR_write_cmos_sensor(0x0202, 0x04);        //  [15:8]  coarse_integration_time H
    S5K4H5YXKERR_write_cmos_sensor(0x0203, 0xE2);        //  [7:0]   coarse_integration_time L
    S5K4H5YXKERR_write_cmos_sensor(0x0340, 0x08);        //  [15:8]  frame_length_lines H
    S5K4H5YXKERR_write_cmos_sensor(0x0341, 0x0C);//E2        //  [7:0]   frame_length_lines L
    S5K4H5YXKERR_write_cmos_sensor(0x0342, 0x1F);        //  [15:8]  line_length_pck H
    S5K4H5YXKERR_write_cmos_sensor(0x0343, 0x98);        //  [7:0]   line_length_pck L
    S5K4H5YXKERR_write_cmos_sensor(0x0344, 0x00);        //  [11:8]  x_addr_start H
    S5K4H5YXKERR_write_cmos_sensor(0x0345, 0x08);        //  [7:0]   x_addr_start L
    S5K4H5YXKERR_write_cmos_sensor(0x0346, 0x01);        //  [11:8]  y_addr_start H
    S5K4H5YXKERR_write_cmos_sensor(0x0347, 0x3A);        //  [7:0]   y_addr_start L
    S5K4H5YXKERR_write_cmos_sensor(0x0348, 0x0C);        //  [11:8]  x_addr_end H
    S5K4H5YXKERR_write_cmos_sensor(0x0349, 0xC7);        //  [7:0]   x_addr_end L
    S5K4H5YXKERR_write_cmos_sensor(0x034A, 0x08);        //  [11:8]  y_addr_end H
    S5K4H5YXKERR_write_cmos_sensor(0x034B, 0x65);        //  [7:0]   y_addr_end L
    S5K4H5YXKERR_write_cmos_sensor(0x034C, 0x0C);        //  [11:8]  x_output_size H
    S5K4H5YXKERR_write_cmos_sensor(0x034D, 0xC0);        //  [7:0]   x_output_size L
    S5K4H5YXKERR_write_cmos_sensor(0x034E, 0x07);        //  [11:8]  y_output_size H
    S5K4H5YXKERR_write_cmos_sensor(0x034F, 0x2C);        //  [7:0]   y_output_size L
    S5K4H5YXKERR_write_cmos_sensor(0x0390, 0x00);        //  [7:0]   binning_mode ([0] binning enable)
    S5K4H5YXKERR_write_cmos_sensor(0x0391, 0x00);        //  [7:0]   binning_type (22h : 2x2 binning, 44h : 4x4 binning)
    S5K4H5YXKERR_write_cmos_sensor(0x0381, 0x01);        //  [4:0]   x_even_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0383, 0x01);        //  [4:0]   x_odd_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0385, 0x01);        //  [4:0]   y_even_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0387, 0x01);        //  [4:0]   y_odd_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0301, 0x02);        //  [3:0]   vt_pix_clk_div                      
    S5K4H5YXKERR_write_cmos_sensor(0x0303, 0x01);        //  [3:0]   vt_sys_clk_div                      
    S5K4H5YXKERR_write_cmos_sensor(0x0305, 0x06);        //  [5:0]   pre_pll_clk_div                     
    S5K4H5YXKERR_write_cmos_sensor(0x0306, 0x00);        //  [9:8]   pll_multiplier H                    
    S5K4H5YXKERR_write_cmos_sensor(0x0307, 0x7D);//8C        //  [7:0]   pll_multiplier L                    
    S5K4H5YXKERR_write_cmos_sensor(0x0309, 0x02);        //  [3:0]   op_pix_clk_div                      
    S5K4H5YXKERR_write_cmos_sensor(0x030B, 0x01);        //  [3:0]   op_sys_clk_div                      
    S5K4H5YXKERR_write_cmos_sensor(0x3C59, 0x00);        //  [2:0]   reg_PLL_S                           
    S5K4H5YXKERR_write_cmos_sensor(0x030D, 0x06);        //  [5:0]   out_pre_pll_clk_div                 
    S5K4H5YXKERR_write_cmos_sensor(0x030E, 0x00);        //  [9:8]   out_pll_multiplier H                
    S5K4H5YXKERR_write_cmos_sensor(0x030F, 0xA8);        //A5        //  [7:0]   out_pll_multiplier L
    S5K4H5YXKERR_write_cmos_sensor(0x3C5A, 0x00);        //  [2:0]   reg_out_PLL_S                       
    S5K4H5YXKERR_write_cmos_sensor(0x0310, 0x01);        //  [0] pll_mode (01h : 2-PLL, 00h : 1-PLL) 
    S5K4H5YXKERR_write_cmos_sensor(0x3C50, 0x53);        //  [7:4]   reg_DIV_DBR                          
    S5K4H5YXKERR_write_cmos_sensor(0x3C62, 0x02);        //  [31:24] requested_link_bit_rate_mbps HH     
    S5K4H5YXKERR_write_cmos_sensor(0x3C63, 0xA0);        //  [23:16] requested_link_bit_rate_mbps HL     
    S5K4H5YXKERR_write_cmos_sensor(0x3C64, 0x00);        //  [15:8]  requested_link_bit_rate_mbps LH     
    S5K4H5YXKERR_write_cmos_sensor(0x3C65, 0x00);        //  [7:0]   requested_link_bit_rate_mbps LL     
    S5K4H5YXKERR_write_cmos_sensor(0x3C1E, 0x00);        //  [3] reg_isp_fe_TN_SMIA_sync_se
    S5K4H5YXKERR_write_cmos_sensor(0x0202, 0x00);
    S5K4H5YXKERR_write_cmos_sensor(0x302A, 0x0A);
    S5K4H5YXKERR_write_cmos_sensor(0x303D, 0x06);    //off_rst   //06-->04  //modify for powernoise 0315
    
           S5K4H5YXKERR_write_cmos_sensor(0x0114, 0x01);  //mipi 2 lane
           
    S5K4H5YXKERR_write_cmos_sensor(0x304B, 0x2A);
    S5K4H5YXKERR_write_cmos_sensor(0x0204, 0x02);
    S5K4H5YXKERR_write_cmos_sensor(0x0205, 0x00);
    S5K4H5YXKERR_write_cmos_sensor(0x3205, 0x84);
    S5K4H5YXKERR_write_cmos_sensor(0x3207, 0x85);
    S5K4H5YXKERR_write_cmos_sensor(0x3214, 0x94);
    S5K4H5YXKERR_write_cmos_sensor(0x3216, 0x95);
    S5K4H5YXKERR_write_cmos_sensor(0x303a, 0x9f);
    S5K4H5YXKERR_write_cmos_sensor(0x3201, 0x07);
    S5K4H5YXKERR_write_cmos_sensor(0x3051, 0xff);
    S5K4H5YXKERR_write_cmos_sensor(0x3052, 0xff);
    S5K4H5YXKERR_write_cmos_sensor(0x3054, 0xF0);
    S5K4H5YXKERR_write_cmos_sensor(0x305C, 0x8F);
    S5K4H5YXKERR_write_cmos_sensor(0x302D, 0x7F);

        //sensor anolog setting  modify for power noise 20130315
        #if 1
        S5K4H5YXKERR_write_cmos_sensor(0x305E, 0x11); //
        S5K4H5YXKERR_write_cmos_sensor(0x305F, 0x11); //
        S5K4H5YXKERR_write_cmos_sensor(0x3060, 0x10); //
        S5K4H5YXKERR_write_cmos_sensor(0x3091, 0x03); //
        S5K4H5YXKERR_write_cmos_sensor(0x3092, 0x03); //
        S5K4H5YXKERR_write_cmos_sensor(0x303D, 0x05); //
        S5K4H5YXKERR_write_cmos_sensor(0x3038, 0x99); //

    #endif
    
    S5K4H5YXKERR_write_cmos_sensor(0x3B29, 0x01); //OTP enable
    
    S5K4H5YXKERR_write_cmos_sensor(0x3903, 0x1F);
    //20130227 add new setting start
    S5K4H5YXKERR_write_cmos_sensor(0x3002, 0x0d);
    S5K4H5YXKERR_write_cmos_sensor(0x300a, 0x0d);
    S5K4H5YXKERR_write_cmos_sensor(0x3045, 0x04);
    S5K4H5YXKERR_write_cmos_sensor(0x300c, 0x78);
    S5K4H5YXKERR_write_cmos_sensor(0x300d, 0x80);
    S5K4H5YXKERR_write_cmos_sensor(0x305c, 0x82);
    S5K4H5YXKERR_write_cmos_sensor(0x3010, 0x0a);
    //20130227 add new setting  end
    S5K4H5YXKERR_write_cmos_sensor(0x0100, 0x01);        //  [0] mode_select

}

void S5K4H5YXKERRCaptureSetting(void)
{
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRCaptureSetting]\n");
    #ifdef Capture8M_New
    S5K4H5YXKERR_write_cmos_sensor(0x0100, 0x00);       //  [0] mode_select
    S5K4H5YXKERR_write_cmos_sensor(0x0101, 0x00);       //  [1:0]   image_orientation ([0] mirror en, [1] flip en)
    S5K4H5YXKERR_write_cmos_sensor(0x0204, 0x00);       //  [15:8]  analogue_gain_code_global H
    S5K4H5YXKERR_write_cmos_sensor(0x0205, 0x20);       //  [7:0]   analogue_gain_code_global L
    S5K4H5YXKERR_write_cmos_sensor(0x0200, 0x0D);       //  [15:8]  fine_integration_time H 
    S5K4H5YXKERR_write_cmos_sensor(0x0201, 0xE8);       //  [7:0]   fine_integration_time L //0x78--->0xE8 MODIFY 20130313
    
    S5K4H5YXKERR_write_cmos_sensor(0x0202, 0x04);       //  [15:8]  coarse_integration_time H
    S5K4H5YXKERR_write_cmos_sensor(0x0203, 0xE2);       //  [7:0]   coarse_integration_time L
    S5K4H5YXKERR_write_cmos_sensor(0x0340, 0x09);       //  [15:8]  frame_length_lines H
    S5K4H5YXKERR_write_cmos_sensor(0x0341, 0xC2);       //  [7:0]   frame_length_lines L
    S5K4H5YXKERR_write_cmos_sensor(0x0342, 0x1F);       //  [15:8]  line_length_pck H
    S5K4H5YXKERR_write_cmos_sensor(0x0343, 0xB0);       //  [7:0]   line_length_pck L
    S5K4H5YXKERR_write_cmos_sensor(0x0344, 0x00);       //  [11:8]  x_addr_start H
    S5K4H5YXKERR_write_cmos_sensor(0x0345, 0x08);       //  [7:0]   x_addr_start L
    S5K4H5YXKERR_write_cmos_sensor(0x0346, 0x00);       //  [11:8]  y_addr_start H
    S5K4H5YXKERR_write_cmos_sensor(0x0347, 0x08);       //  [7:0]   y_addr_start L
    S5K4H5YXKERR_write_cmos_sensor(0x0348, 0x0C);       //  [11:8]  x_addr_end H
    S5K4H5YXKERR_write_cmos_sensor(0x0349, 0xC7);       //  [7:0]   x_addr_end L
    S5K4H5YXKERR_write_cmos_sensor(0x034A, 0x09);       //  [11:8]  y_addr_end H
    S5K4H5YXKERR_write_cmos_sensor(0x034B, 0x97);       //  [7:0]   y_addr_end L
    S5K4H5YXKERR_write_cmos_sensor(0x034C, 0x0C);       //  [11:8]  x_output_size H
    S5K4H5YXKERR_write_cmos_sensor(0x034D, 0xC0);       //  [7:0]   x_output_size L
    S5K4H5YXKERR_write_cmos_sensor(0x034E, 0x09);       //  [11:8]  y_output_size H
    S5K4H5YXKERR_write_cmos_sensor(0x034F, 0x90);       //  [7:0]   y_output_size L
    S5K4H5YXKERR_write_cmos_sensor(0x0390, 0x00);       //  [7:0]   binning_mode ([0] binning enable)
    S5K4H5YXKERR_write_cmos_sensor(0x0391, 0x00);       //  [7:0]   binning_type (22h : 2x2 binning, 44h : 4x4 binning)
    S5K4H5YXKERR_write_cmos_sensor(0x0381, 0x01);       //  [4:0]   x_even_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0383, 0x01);       //  [4:0]   x_odd_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0385, 0x01);       //  [4:0]   y_even_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0387, 0x01);       //  [4:0]   y_odd_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0301, 0x02);       //  [3:0]   vt_pix_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x0303, 0x01);       //  [3:0]   vt_sys_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x0305, 0x06);       //  [5:0]   pre_pll_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x0306, 0x00);       //  [9:8]   pll_multiplier H
    S5K4H5YXKERR_write_cmos_sensor(0x0307, 0x7D);       //  [7:0]   pll_multiplier L
    S5K4H5YXKERR_write_cmos_sensor(0x0309, 0x02);       //  [3:0]   op_pix_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x030B, 0x01);       //  [3:0]   op_sys_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x3C59, 0x00);       //  [2:0]   reg_PLL_S
    S5K4H5YXKERR_write_cmos_sensor(0x030D, 0x06);       //  [5:0]   out_pre_pll_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x030E, 0x00);       //  [9:8]   out_pll_multiplier H
    S5K4H5YXKERR_write_cmos_sensor(0x030F, 0xA8);       //A5        //  [7:0]   out_pll_multiplier L
    S5K4H5YXKERR_write_cmos_sensor(0x3C5A, 0x00);       //  [2:0]   reg_out_PLL_S
    S5K4H5YXKERR_write_cmos_sensor(0x0310, 0x01);       //  [0] pll_mode (01h : 2-PLL, 00h : 1-PLL)
    S5K4H5YXKERR_write_cmos_sensor(0x3C50, 0x53);       //  [7:4]   reg_DIV_DBR
    S5K4H5YXKERR_write_cmos_sensor(0x3C62, 0x02);       //  [31:24] requested_link_bit_rate_mbps HH
    S5K4H5YXKERR_write_cmos_sensor(0x3C63, 0xA0);       //  [23:16] requested_link_bit_rate_mbps HL
    S5K4H5YXKERR_write_cmos_sensor(0x3C64, 0x00);       //  [15:8]  requested_link_bit_rate_mbps LH
    S5K4H5YXKERR_write_cmos_sensor(0x3C65, 0x00);       //  [7:0]   requested_link_bit_rate_mbps LL
    S5K4H5YXKERR_write_cmos_sensor(0x3C1E, 0x00);       //  [3] reg_isp_fe_TN_SMIA_sync_sel
    S5K4H5YXKERR_write_cmos_sensor(0x0202, 0x00);   //coarse_integration_time_up
    S5K4H5YXKERR_write_cmos_sensor(0x302A, 0x0A);   //vda_width
    S5K4H5YXKERR_write_cmos_sensor(0x303D, 0x06);   //off_rst   //06-->04  //modify for powernoise 0315
    
           S5K4H5YXKERR_write_cmos_sensor(0x0114, 0x01);  //mipi 2 lane
           
    S5K4H5YXKERR_write_cmos_sensor(0x304B, 0x2A);   //ADC_SAT 490mV
    S5K4H5YXKERR_write_cmos_sensor(0x0204, 0x02);   //analog gain x16
    S5K4H5YXKERR_write_cmos_sensor(0x0205, 0x00);   //analog gain x16
    S5K4H5YXKERR_write_cmos_sensor(0x3205, 0x84);   //adc_offset_odd0
    S5K4H5YXKERR_write_cmos_sensor(0x3207, 0x85);   //adc_offset_even1
    S5K4H5YXKERR_write_cmos_sensor(0x3214, 0x94);   //adc_offset_odd0_MS
    S5K4H5YXKERR_write_cmos_sensor(0x3216, 0x95);   //adc_offset_even1_MS
    S5K4H5YXKERR_write_cmos_sensor(0x303a, 0x9f);   //clp_lvl
    S5K4H5YXKERR_write_cmos_sensor(0x3201, 0x07);   //dither_sel[2]: fob, dither_sel[1]: lob, dither_sel[0]: active
    S5K4H5YXKERR_write_cmos_sensor(0x3051, 0xff);   //blst 
    S5K4H5YXKERR_write_cmos_sensor(0x3052, 0xff);   //blst
    S5K4H5YXKERR_write_cmos_sensor(0x3054, 0xF0);   //rdv_option[7]=1 (caution of address overwrite)
    S5K4H5YXKERR_write_cmos_sensor(0x305C, 0x8F);   //cds_option (reduce s3/s4 buffer strength, s4_rpt_enable)
    S5K4H5YXKERR_write_cmos_sensor(0x302D, 0x7F); //[4] dshut_en=1

        //sensor anolog setting  modify for power noise 20130315
        #if 1
        S5K4H5YXKERR_write_cmos_sensor(0x305E, 0x11); //
        S5K4H5YXKERR_write_cmos_sensor(0x305F, 0x11); //
        S5K4H5YXKERR_write_cmos_sensor(0x3060, 0x10); //
        S5K4H5YXKERR_write_cmos_sensor(0x3091, 0x03); //
        S5K4H5YXKERR_write_cmos_sensor(0x3092, 0x03); //
        S5K4H5YXKERR_write_cmos_sensor(0x303D, 0x05); //
        S5K4H5YXKERR_write_cmos_sensor(0x3038, 0x99); //

    #endif
    
    S5K4H5YXKERR_write_cmos_sensor(0x3B29, 0x01); //OTP enable
    
    S5K4H5YXKERR_write_cmos_sensor(0x3903, 0x1F);
    //20130227 add new setting start
    S5K4H5YXKERR_write_cmos_sensor(0x3002, 0x0d);
    S5K4H5YXKERR_write_cmos_sensor(0x300a, 0x0d);
    S5K4H5YXKERR_write_cmos_sensor(0x3045, 0x04);
    S5K4H5YXKERR_write_cmos_sensor(0x300c, 0x78);
    S5K4H5YXKERR_write_cmos_sensor(0x300d, 0x80);
    S5K4H5YXKERR_write_cmos_sensor(0x305c, 0x82);
    S5K4H5YXKERR_write_cmos_sensor(0x3010, 0x0a);
    //20130227 add new setting  end
    S5K4H5YXKERR_write_cmos_sensor(0x0100, 0x01);       //  [0] mode_select
    #else
    S5K4H5YXKERR_write_cmos_sensor(0x0100, 0x00);        //  [0] mode_select
    S5K4H5YXKERR_write_cmos_sensor(0x0101, 0x00);        //  [1:0]   image_orientation ([0] mirror en, [1] flip en)
    S5K4H5YXKERR_write_cmos_sensor(0x0204, 0x00);        //  [15:8]  analogue_gain_code_global H
    S5K4H5YXKERR_write_cmos_sensor(0x0205, 0x20);        //  [7:0]   analogue_gain_code_global L
    S5K4H5YXKERR_write_cmos_sensor(0x0200, 0x0D);        //  [15:8]  fine_integration_time H
    S5K4H5YXKERR_write_cmos_sensor(0x0201, 0xE8);        //  [7:0]   fine_integration_time L  //0x78--->0xE8 MODIFY 20130313
    
    S5K4H5YXKERR_write_cmos_sensor(0x0202, 0x04);        //  [15:8]  coarse_integration_time H
    S5K4H5YXKERR_write_cmos_sensor(0x0203, 0xE2);        //  [7:0]   coarse_integration_time L
    S5K4H5YXKERR_write_cmos_sensor(0x0340, 0x09);        //  [15:8]  frame_length_lines H
    S5K4H5YXKERR_write_cmos_sensor(0x0341, 0xB6);        //  [7:0]   frame_length_lines L
    S5K4H5YXKERR_write_cmos_sensor(0x0342, 0x1F);        //  [15:8]  line_length_pck H
    S5K4H5YXKERR_write_cmos_sensor(0x0343, 0x98);        //  [7:0]   line_length_pck L
    S5K4H5YXKERR_write_cmos_sensor(0x0344, 0x00);        //  [11:8]  x_addr_start H
    S5K4H5YXKERR_write_cmos_sensor(0x0345, 0x08);        //  [7:0]   x_addr_start L
    S5K4H5YXKERR_write_cmos_sensor(0x0346, 0x00);        //  [11:8]  y_addr_start H
    S5K4H5YXKERR_write_cmos_sensor(0x0347, 0x06);        //  [7:0]   y_addr_start L
    S5K4H5YXKERR_write_cmos_sensor(0x0348, 0x0C);        //  [11:8]  x_addr_end H
    S5K4H5YXKERR_write_cmos_sensor(0x0349, 0xC7);        //  [7:0]   x_addr_end L
    S5K4H5YXKERR_write_cmos_sensor(0x034A, 0x09);        //  [11:8]  y_addr_end H
    S5K4H5YXKERR_write_cmos_sensor(0x034B, 0x99);        //  [7:0]   y_addr_end L
    S5K4H5YXKERR_write_cmos_sensor(0x034C, 0x0C);        //  [11:8]  x_output_size H
    S5K4H5YXKERR_write_cmos_sensor(0x034D, 0xC0);        //  [7:0]   x_output_size L
    S5K4H5YXKERR_write_cmos_sensor(0x034E, 0x09);        //  [11:8]  y_output_size H
    S5K4H5YXKERR_write_cmos_sensor(0x034F, 0x94);        //  [7:0]   y_output_size L
    S5K4H5YXKERR_write_cmos_sensor(0x0390, 0x00);        //  [7:0]   binning_mode ([0] binning enable)
    S5K4H5YXKERR_write_cmos_sensor(0x0391, 0x00);        //  [7:0]   binning_type (22h : 2x2 binning, 44h : 4x4 binning)
    S5K4H5YXKERR_write_cmos_sensor(0x0381, 0x01);        //  [4:0]   x_even_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0383, 0x01);        //  [4:0]   x_odd_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0385, 0x01);        //  [4:0]   y_even_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0387, 0x01);        //  [4:0]   y_odd_inc
    S5K4H5YXKERR_write_cmos_sensor(0x0301, 0x02);        //  [3:0]   vt_pix_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x0303, 0x01);        //  [3:0]   vt_sys_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x0305, 0x06);        //  [5:0]   pre_pll_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x0306, 0x00);        //  [9:8]   pll_multiplier H
    S5K4H5YXKERR_write_cmos_sensor(0x0307, 0x7D);        //  [7:0]   pll_multiplier L
    S5K4H5YXKERR_write_cmos_sensor(0x0309, 0x02);        //  [3:0]   op_pix_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x030B, 0x01);        //  [3:0]   op_sys_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x3C59, 0x00);        //  [2:0]   reg_PLL_S
    S5K4H5YXKERR_write_cmos_sensor(0x030D, 0x06);        //  [5:0]   out_pre_pll_clk_div
    S5K4H5YXKERR_write_cmos_sensor(0x030E, 0x00);        //  [9:8]   out_pll_multiplier H
    S5K4H5YXKERR_write_cmos_sensor(0x030F, 0xA8);        //A5        //  [7:0]   out_pll_multiplier L
    S5K4H5YXKERR_write_cmos_sensor(0x3C5A, 0x00);        //  [2:0]   reg_out_PLL_S
    S5K4H5YXKERR_write_cmos_sensor(0x0310, 0x01);        //  [0] pll_mode (01h : 2-PLL, 00h : 1-PLL)
    S5K4H5YXKERR_write_cmos_sensor(0x3C50, 0x53);        //  [7:4]   reg_DIV_DBR
    S5K4H5YXKERR_write_cmos_sensor(0x3C62, 0x02);        //  [31:24] requested_link_bit_rate_mbps HH
    S5K4H5YXKERR_write_cmos_sensor(0x3C63, 0xA0);        //  [23:16] requested_link_bit_rate_mbps HL
    S5K4H5YXKERR_write_cmos_sensor(0x3C64, 0x00);        //  [15:8]  requested_link_bit_rate_mbps LH
    S5K4H5YXKERR_write_cmos_sensor(0x3C65, 0x00);        //  [7:0]   requested_link_bit_rate_mbps LL
    S5K4H5YXKERR_write_cmos_sensor(0x3C1E, 0x00);        //  [3] reg_isp_fe_TN_SMIA_sync_
    S5K4H5YXKERR_write_cmos_sensor(0x0202, 0x00);
    S5K4H5YXKERR_write_cmos_sensor(0x302A, 0x0A);
    S5K4H5YXKERR_write_cmos_sensor(0x303D, 0x06);    //off_rst   //06-->04  //modify for powernoise 0315
    
           S5K4H5YXKERR_write_cmos_sensor(0x0114, 0x01);  //mipi 2 lane
           
    S5K4H5YXKERR_write_cmos_sensor(0x304B, 0x2A);
    S5K4H5YXKERR_write_cmos_sensor(0x0204, 0x02);
    S5K4H5YXKERR_write_cmos_sensor(0x0205, 0x00);
    S5K4H5YXKERR_write_cmos_sensor(0x3205, 0x84);
    S5K4H5YXKERR_write_cmos_sensor(0x3207, 0x85);
    S5K4H5YXKERR_write_cmos_sensor(0x3214, 0x94);
    S5K4H5YXKERR_write_cmos_sensor(0x3216, 0x95);
    S5K4H5YXKERR_write_cmos_sensor(0x303a, 0x9f);
    S5K4H5YXKERR_write_cmos_sensor(0x3201, 0x07);
    S5K4H5YXKERR_write_cmos_sensor(0x3051, 0xff);
    S5K4H5YXKERR_write_cmos_sensor(0x3052, 0xff);
    S5K4H5YXKERR_write_cmos_sensor(0x3054, 0xF0);
    S5K4H5YXKERR_write_cmos_sensor(0x305C, 0x8F);
    S5K4H5YXKERR_write_cmos_sensor(0x302D, 0x7F);
        //sensor anolog setting  modify for power noise 20130315
    #if 1
    S5K4H5YXKERR_write_cmos_sensor(0x305E, 0x11); //
    S5K4H5YXKERR_write_cmos_sensor(0x305F, 0x11); //
    S5K4H5YXKERR_write_cmos_sensor(0x3060, 0x10); //
    S5K4H5YXKERR_write_cmos_sensor(0x3091, 0x03); //
    S5K4H5YXKERR_write_cmos_sensor(0x3092, 0x03); //
    S5K4H5YXKERR_write_cmos_sensor(0x303D, 0x05); //
    S5K4H5YXKERR_write_cmos_sensor(0x3038, 0x99); //

    #endif
    
    S5K4H5YXKERR_write_cmos_sensor(0x3B29, 0x01); //OTP enable
    
    S5K4H5YXKERR_write_cmos_sensor(0x3903, 0x1F);
    //20130227 add new setting start
    S5K4H5YXKERR_write_cmos_sensor(0x3002, 0x0d);
    S5K4H5YXKERR_write_cmos_sensor(0x300a, 0x0d);
    S5K4H5YXKERR_write_cmos_sensor(0x3045, 0x04);
    S5K4H5YXKERR_write_cmos_sensor(0x300c, 0x78);
    S5K4H5YXKERR_write_cmos_sensor(0x300d, 0x80);
    S5K4H5YXKERR_write_cmos_sensor(0x305c, 0x82);
    S5K4H5YXKERR_write_cmos_sensor(0x3010, 0x0a);
    //20130227 add new setting  end
    S5K4H5YXKERR_write_cmos_sensor(0x0100, 0x01);        //  [0] mode_select
    #endif
}

/*************************************************************************
* FUNCTION
*   S5K4H5YXKERROpen
*
* DESCRIPTION
*   This function initialize the registers of CMOS sensor
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/

UINT32 S5K4H5YXKERROpen(void)
{

    volatile signed int i;
    kal_uint16 sensor_id = 0;

    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERROpen]\n");

    //  Read sensor ID to adjust I2C is OK?
    for(i=0;i<3;i++)
    {
        sensor_id = (S5K4H5YXKERR_read_cmos_sensor(0x0000)<<8)|S5K4H5YXKERR_read_cmos_sensor(0x0001);
        S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERROpen] sensor_id=%x\n",sensor_id);
        if(sensor_id != S5K4H5YXKERR_SENSOR_ID)
        {
            return ERROR_SENSOR_CONNECT_FAIL;
        }else
            break;
    }
    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
    s5k4h5yxkerr.sensorMode = SENSOR_MODE_INIT;
    s5k4h5yxkerr.S5K4H5YXKERRAutoFlickerMode = KAL_FALSE;
    s5k4h5yxkerr.S5K4H5YXKERRVideoMode = KAL_FALSE;
    s5k4h5yxkerr.DummyLines= 0;
    s5k4h5yxkerr.DummyPixels= 0;

    s5k4h5yxkerr.pvPclk =  (25000);  
    s5k4h5yxkerr.videoPclk = (25000); 
    
    spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);

    switch(S5K4H5YXKERRCurrentScenarioId)
        {
            case MSDK_SCENARIO_ID_CAMERA_ZSD:
                #if defined(ZSD15FPS)
                spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
                s5k4h5yxkerr.capPclk = (25000);
                spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
                #else
                spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
                s5k4h5yxkerr.capPclk = (25000);
                spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
                #endif
                break;
        default:
                spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
                s5k4h5yxkerr.capPclk = (25000);
                spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
                break;
          }
    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
    s5k4h5yxkerr.shutter = 0x4EA;
    s5k4h5yxkerr.pvShutter = 0x4EA;
    #ifdef Capture8M_New
    s5k4h5yxkerr.maxExposureLines =1232 -4;
    #else
    s5k4h5yxkerr.maxExposureLines =S5K4H5YXKERR_PV_PERIOD_LINE_NUMS -4;
    #endif

    s5k4h5yxkerr.ispBaseGain = BASEGAIN;//0x40
    s5k4h5yxkerr.sensorGlobalGain = 0x1f;//sensor gain read from 0x350a 0x350b; 0x1f as 3.875x
    s5k4h5yxkerr.pvGain = 0x1f;
    spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);

    //afPowerOn();

    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   S5K4H5YXKERRGetSensorID
*
* DESCRIPTION
*   This function get the sensor ID
*
* PARAMETERS
*   *sensorID : return the sensor ID
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 S5K4H5YXKERRGetSensorID(UINT32 *sensorID)
{
    int  retry = 1;
    int  MID = 0;
    unsigned char val=0;

    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRGetSensorID]\n");
    //S5K4H5YXKERR_write_cmos_sensor(0x0103,0x01);// Reset sensor
    //mDELAY(10);

    // check if sensor ID correct
    do {
        *sensorID = (S5K4H5YXKERR_read_cmos_sensor(0x0000)<<8)|S5K4H5YXKERR_read_cmos_sensor(0x0001);
        if (*sensorID == S5K4H5YXKERR_SENSOR_ID)
        {
             S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRGetSensorID] Sensor ID = 0x%04x\n", *sensorID);
              break;
        }
        S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRGetSensorID] Read Sensor ID Fail = 0x%04x\n", *sensorID);
        retry--;
    } while (retry > 0);

    // check if module ID correct TRULY ID 0x3A04 is 0x0002 a-kerr ID 0x3A05 is 0x0003
    S5K4H5YXKERR_write_cmos_sensor(0x3A02,0x00);
    S5K4H5YXKERR_write_cmos_sensor(0x3A00,0x01);
    mdelay(5);
    val=S5K4H5YXKERR_read_cmos_sensor(0x3A04);//flag of info and awb
    if((val&0xf0)==0x40)
    {
        MID = S5K4H5YXKERR_read_cmos_sensor(0x3A05);
    }
    else if((val&0xf0)==0xd0)
    {
        MID = S5K4H5YXKERR_read_cmos_sensor(0x3A1a);
    }
    else
    {
        MID =0x00;
    }
    S5K4H5YXKERR_write_cmos_sensor(0x3A00,0x00);

    if ((*sensorID != S5K4H5YXKERR_SENSOR_ID) || (MID != 0x0003))
    {
        *sensorID = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*   S5K4H5YXKERR_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of s5k4h5yxkerr to change exposure time.
*
* PARAMETERS
*   shutter : exposured lines
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void S5K4H5YXKERR_SetShutter(kal_uint32 iShutter)
{
//unsigned long flags;
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_SetShutter] shutter = %d\n", iShutter);
spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
s5k4h5yxkerr.shutter= iShutter;
spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
S5K4H5YXKERR_write_shutter(iShutter);
 return;
}   /*  S5K4H5YXKERR_SetShutter   */



/*************************************************************************
* FUNCTION
*   S5K4H5YXKERR_read_shutter
*
* DESCRIPTION
*   This function to  Get exposure time.
*
* PARAMETERS
*   None
*
* RETURNS
*   shutter : exposured lines
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 S5K4H5YXKERR_read_shutter(void)
{

//    kal_uint16 temp_reg1, temp_reg2 ,temp_reg3;
    UINT32 shutter =0;

    shutter = (S5K4H5YXKERR_read_cmos_sensor(0x0202) << 8) | S5K4H5YXKERR_read_cmos_sensor(0x0203);
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERR_read_shutter] shutter = %d\n", shutter);
    return shutter;
}

/*************************************************************************
* FUNCTION
*   S5K4H5YXKERR_night_mode
*
* DESCRIPTION
*   This function night mode of s5k4h5yxkerr.
*
* PARAMETERS
*   none
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void S5K4H5YXKERR_NightMode(kal_bool bEnable)
{
}/* S5K4H5YXKERR_NightMode */



/*************************************************************************
* FUNCTION
*   S5K4H5YXKERRClose
*
* DESCRIPTION
*   This function is to turn off sensor module power.
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 S5K4H5YXKERRClose(void)
{
    //afPowerOff();
    return ERROR_NONE;
}   /* S5K4H5YXKERRClose() */

void S5K4H5YXKERRSetFlipMirror(kal_int32 imgMirror)
{
//    kal_int16 mirror=0,flip=0;
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRSetFlipMirror] imgMirror = %d\n", imgMirror);
        switch (imgMirror)
    {
        case IMAGE_NORMAL: //B
            S5K4H5YXKERR_write_cmos_sensor(0x0101, 0x03); //Set normal
            break;
        case IMAGE_V_MIRROR: //Gr X
            S5K4H5YXKERR_write_cmos_sensor(0x0101, 0x01); //Set flip
            break;
        case IMAGE_H_MIRROR: //Gb
            S5K4H5YXKERR_write_cmos_sensor(0x0101, 0x02); //Set mirror
            break;
        case IMAGE_HV_MIRROR: //R
            S5K4H5YXKERR_write_cmos_sensor(0x0101, 0x00); //Set mirror and flip
            break;
    }
}


/*************************************************************************
* FUNCTION
*   S5K4H5YXKERRPreview
*
* DESCRIPTION
*   This function start the sensor preview.
*
* PARAMETERS
*   *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 S5K4H5YXKERRPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRPreview]\n");

    // preview size
    if(s5k4h5yxkerr.sensorMode == SENSOR_MODE_PREVIEW)
    {
    }
    else
    {
        S5K4H5YXKERRPreviewSetting();
    }
    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
    s5k4h5yxkerr.sensorMode = SENSOR_MODE_PREVIEW; // Need set preview setting after capture mode
    s5k4h5yxkerr.DummyPixels = 0;//define dummy pixels and lines
    s5k4h5yxkerr.DummyLines = 0 ;
    cont_preview_line_length_kerr=s5k4h5yxkerr_preview_line_length;
    cont_preview_frame_length_kerr=s5k4h5yxkerr_preview_frame_length+s5k4h5yxkerr.DummyLines;
    S5K4H5YXKERR_FeatureControl_PERIOD_PixelNum = s5k4h5yxkerr_preview_line_length;
    S5K4H5YXKERR_FeatureControl_PERIOD_LineNum = cont_preview_frame_length_kerr;
    spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);


    //set mirror & flip
    //S5K4H5YXKERRDB("[S5K4H5YXKERRPreview] mirror&flip: %d \n",sensor_config_data->SensorImageMirror);
    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
    s5k4h5yxkerr.imgMirror = sensor_config_data->SensorImageMirror;
    spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
    //S5K4H5YXKERRSetFlipMirror(sensor_config_data->SensorImageMirror);
    S5K4H5YXKERRSetFlipMirror(IMAGE_NORMAL);
    return ERROR_NONE;
}   /* S5K4H5YXKERRPreview() */



UINT32 S5K4H5YXKERRVideo(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRVideo]\n");

    if(s5k4h5yxkerr.sensorMode == SENSOR_MODE_VIDEO)
    {
        // do nothing
    }
    else
    {
        S5K4H5YXKERRVideoSetting();

    }
    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
    s5k4h5yxkerr.sensorMode = SENSOR_MODE_VIDEO;
    S5K4H5YXKERR_FeatureControl_PERIOD_PixelNum = s5k4h5yxkerr_video_line_length;
    S5K4H5YXKERR_FeatureControl_PERIOD_LineNum = s5k4h5yxkerr_video_frame_length;
    spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);

    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
    s5k4h5yxkerr.imgMirror = sensor_config_data->SensorImageMirror;
    spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
    //S5K4H5YXKERRSetFlipMirror(sensor_config_data->SensorImageMirror);
    S5K4H5YXKERRSetFlipMirror(IMAGE_NORMAL);
    return ERROR_NONE;
}


UINT32 S5K4H5YXKERRCapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                                                MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{

    kal_uint32 shutter = s5k4h5yxkerr.shutter;
    kal_uint32 temp_data;
    //kal_uint32 pv_line_length , cap_line_length,

    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRCapture]\n");

    if( SENSOR_MODE_CAPTURE== s5k4h5yxkerr.sensorMode)
    {
        S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRCapture] BusrtShot!!\n");
    }else{

    //Record Preview shutter & gain
    shutter=S5K4H5YXKERR_read_shutter();
    temp_data =  read_S5K4H5YXKERR_gain();
    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
    s5k4h5yxkerr.pvShutter =shutter;
    s5k4h5yxkerr.sensorGlobalGain = temp_data;
    s5k4h5yxkerr.pvGain =s5k4h5yxkerr.sensorGlobalGain;
    s5k4h5yxkerr.sensorMode = SENSOR_MODE_CAPTURE;  
    spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);

    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRCapture] s5k4h5yxkerr.shutter=%d, read_pv_shutter=%d, read_pv_gain = 0x%x\n",s5k4h5yxkerr.shutter, shutter,s5k4h5yxkerr.sensorGlobalGain);

    // Full size setting
    S5K4H5YXKERRCaptureSetting();

    //rewrite pixel number to Register ,for mt6589 line start/end;
    S5K4H5YXKERR_SetDummy(s5k4h5yxkerr.DummyPixels,s5k4h5yxkerr.DummyLines);

    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);

    s5k4h5yxkerr.imgMirror = sensor_config_data->SensorImageMirror;
    s5k4h5yxkerr.DummyPixels = 0;//define dummy pixels and lines
    s5k4h5yxkerr.DummyLines = 0 ;
    cont_capture_line_length_kerr = s5k4h5yxkerr_capture_line_length;
    cont_capture_frame_length_kerr = s5k4h5yxkerr_capture_frame_length + s5k4h5yxkerr.DummyLines;
    S5K4H5YXKERR_FeatureControl_PERIOD_PixelNum = s5k4h5yxkerr_capture_line_length;
    S5K4H5YXKERR_FeatureControl_PERIOD_LineNum = cont_capture_frame_length_kerr;
    spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);

    //S5K4H5YXKERRDB("[S5K4H5YXKERRCapture] mirror&flip: %d\n",sensor_config_data->SensorImageMirror);
    //S5K4H5YXKERRSetFlipMirror(sensor_config_data->SensorImageMirror);
    S5K4H5YXKERRSetFlipMirror(IMAGE_NORMAL);

    //#if defined(MT6575)||defined(MT6577)
    if(S5K4H5YXKERRCurrentScenarioId==MSDK_SCENARIO_ID_CAMERA_ZSD)
    {
        S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRCapture] S5K4H5YXKERRCapture exit ZSD!\n");
        return ERROR_NONE;
    }
    //#endif   
    }
    
    return ERROR_NONE;
}   /* S5K4H5YXKERRCapture() */

UINT32 S5K4H5YXKERRGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRGetResolution]\n");

    //1632*1226
    #ifdef Capture8M
    pSensorResolution->SensorPreviewWidth   = 1632;
    pSensorResolution->SensorPreviewHeight   = 1224;
    #else
    pSensorResolution->SensorPreviewWidth   = S5K4H5YXKERR_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight   = S5K4H5YXKERR_IMAGE_SENSOR_PV_HEIGHT;
    #endif

    //3264*2452
    #ifdef Capture8M
    pSensorResolution->SensorFullWidth      = 3264-40;
    pSensorResolution->SensorFullHeight      = 2448-32;
    #else
    pSensorResolution->SensorFullWidth       = S5K4H5YXKERR_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight      = S5K4H5YXKERR_IMAGE_SENSOR_FULL_HEIGHT;
    #endif
    
    //3264*1836
    pSensorResolution->SensorVideoWidth      = S5K4H5YXKERR_IMAGE_SENSOR_VIDEO_WIDTH;
    pSensorResolution->SensorVideoHeight    = S5K4H5YXKERR_IMAGE_SENSOR_VIDEO_HEIGHT;
    return ERROR_NONE;
}   /* S5K4H5YXKERRGetResolution() */


UINT32 S5K4H5YXKERRGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                                                MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
     S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRGetInfo]\n");
    #ifdef Capture8M
    pSensorInfo->SensorPreviewResolutionX= 1632;
    pSensorInfo->SensorPreviewResolutionY= 1224;
    #else
    pSensorInfo->SensorPreviewResolutionX= S5K4H5YXKERR_IMAGE_SENSOR_PV_WIDTH;
    pSensorInfo->SensorPreviewResolutionY= S5K4H5YXKERR_IMAGE_SENSOR_PV_HEIGHT;
    #endif
    
    #ifdef Capture8M
    pSensorInfo->SensorFullResolutionX= 3264;
    pSensorInfo->SensorFullResolutionY= 2448;
    #else
    pSensorInfo->SensorFullResolutionX= S5K4H5YXKERR_IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY= S5K4H5YXKERR_IMAGE_SENSOR_FULL_HEIGHT;
    #endif
    
    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
    s5k4h5yxkerr.imgMirror = pSensorConfigData->SensorImageMirror ;
    spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);



        pSensorInfo->SensorOutputDataFormat= SENSOR_OUTPUT_FORMAT_RAW_R; //Gb r

   
    pSensorInfo->SensorClockPolarity =SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;

    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;

    pSensorInfo->CaptureDelayFrame = 1;
    pSensorInfo->PreviewDelayFrame = 1;
    pSensorInfo->VideoDelayFrame = 2;

    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;
    pSensorInfo->AEShutDelayFrame = 0;//0;           /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0 ;//0;     /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;
            
            #if defined(Capture8M_New)
            pSensorInfo->SensorGrabStartX = S5K4H5YXKERR_PV_X_START;
            pSensorInfo->SensorGrabStartY = S5K4H5YXKERR_PV_Y_START+1;
            #elif defined(Capture8M)
            pSensorInfo->SensorGrabStartX = 0;
            pSensorInfo->SensorGrabStartY = 1+1;
            #else
            pSensorInfo->SensorGrabStartX = S5K4H5YXKERR_PV_X_START;
            pSensorInfo->SensorGrabStartY = S5K4H5YXKERR_PV_Y_START+1;
            #endif
    
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
             pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 6;//14
        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = S5K4H5YXKERR_VIDEO_X_START;
            pSensorInfo->SensorGrabStartY = S5K4H5YXKERR_VIDEO_Y_START+1;
            
            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
             pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 6;//14
        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            #if defined(Capture8M_New)
            pSensorInfo->SensorGrabStartX = S5K4H5YXKERR_FULL_X_START;  //2*S5K4H5YXKERR_IMAGE_SENSOR_PV_STARTX;
            pSensorInfo->SensorGrabStartY = S5K4H5YXKERR_FULL_Y_START+1;  //2*S5K4H5YXKERR_IMAGE_SENSOR_PV_STARTY;
            #elif defined(Capture8M)
            pSensorInfo->SensorGrabStartX = 0;  //2*S5K4H5YXKERR_IMAGE_SENSOR_PV_STARTX;
            pSensorInfo->SensorGrabStartY = 1+1;  //2*S5K4H5YXKERR_IMAGE_SENSOR_PV_STARTY;
            #else
            pSensorInfo->SensorGrabStartX = S5K4H5YXKERR_FULL_X_START;    //2*S5K4H5YXKERR_IMAGE_SENSOR_PV_STARTX;
            pSensorInfo->SensorGrabStartY = S5K4H5YXKERR_FULL_Y_START+1;  //2*S5K4H5YXKERR_IMAGE_SENSOR_PV_STARTY;
            #endif

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 6;//14
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
        default:
            pSensorInfo->SensorClockFreq=24;
            pSensorInfo->SensorClockRisingCount= 0;

            pSensorInfo->SensorGrabStartX = S5K4H5YXKERR_PV_X_START;
            pSensorInfo->SensorGrabStartY = S5K4H5YXKERR_PV_Y_START+1;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
             pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 6;//14
        pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorPacketECCOrder = 1;
            break;
    }

    memcpy(pSensorConfigData, &S5K4H5YXKERRSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));

    return ERROR_NONE;
}   /* S5K4H5YXKERRGetInfo() */


UINT32 S5K4H5YXKERRControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                                                MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
        spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
        S5K4H5YXKERRCurrentScenarioId = ScenarioId;
        spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
        //S5K4H5YXKERRDB("ScenarioId=%d\n",ScenarioId);
        S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRControl]\n");
        S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRControl] ScenarioId=%d\n",S5K4H5YXKERRCurrentScenarioId);
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            S5K4H5YXKERRPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            S5K4H5YXKERRVideo(pImageWindow, pSensorConfigData);
            break;   
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            S5K4H5YXKERRCapture(pImageWindow, pSensorConfigData);
            break;

        default:
            return ERROR_INVALID_SCENARIO_ID;

    }
    return ERROR_NONE;
} /* S5K4H5YXKERRControl() */


UINT32 S5K4H5YXKERRSetVideoMode(UINT16 u2FrameRate)
{

    kal_uint32 MIN_Frame_length =0,frameRate=0,extralines=0;
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRSetVideoMode] frame rate = %d\n", u2FrameRate);
    
    if(u2FrameRate==0)
    {
        S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRSetVideoMode] Disable Video Mode or dynimac fps\n");
        return KAL_TRUE;
    }
    if(u2FrameRate >30 || u2FrameRate <5)
        S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRSetVideoMode] error frame rate seting\n");

    if(s5k4h5yxkerr.sensorMode == SENSOR_MODE_VIDEO)//video ScenarioId recording
    {
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRSetVideoMode] SENSOR_MODE_VIDEO\n");
    if(s5k4h5yxkerr.S5K4H5YXKERRAutoFlickerMode == KAL_TRUE)
    {
         if (u2FrameRate==30)
                frameRate= 306;
            else if(u2FrameRate==15)
                frameRate= 148;//148;
            else
                frameRate=u2FrameRate*10;

            MIN_Frame_length = (s5k4h5yxkerr.videoPclk*10000)/(S5K4H5YXKERR_VIDEO_PERIOD_PIXEL_NUMS + s5k4h5yxkerr.DummyPixels)/frameRate*10;
    }
        else
            MIN_Frame_length = (s5k4h5yxkerr.videoPclk*10000) /(S5K4H5YXKERR_VIDEO_PERIOD_PIXEL_NUMS + s5k4h5yxkerr.DummyPixels)/u2FrameRate;

        if((MIN_Frame_length <=S5K4H5YXKERR_VIDEO_PERIOD_LINE_NUMS))
        {
            MIN_Frame_length = S5K4H5YXKERR_VIDEO_PERIOD_LINE_NUMS;
        }
        extralines = MIN_Frame_length - S5K4H5YXKERR_VIDEO_PERIOD_LINE_NUMS;

        spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
        s5k4h5yxkerr.DummyPixels = 0;//define dummy pixels and lines
        s5k4h5yxkerr.DummyLines = extralines ;
        spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);

        S5K4H5YXKERR_SetDummy(s5k4h5yxkerr.DummyPixels,extralines);
    }
    else if(s5k4h5yxkerr.sensorMode == SENSOR_MODE_CAPTURE)
    {
        S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRSetVideoMode] SENSOR_MODE_CAPTURE\n");
        if(s5k4h5yxkerr.S5K4H5YXKERRAutoFlickerMode == KAL_TRUE)
    {
            #if defined(ZSD15FPS)
         if (u2FrameRate==15)
                frameRate= 148;
            #else
         if (u2FrameRate==13)
                frameRate= 130;
            #endif
            else
                frameRate=u2FrameRate*10;
            
            MIN_Frame_length = (s5k4h5yxkerr.capPclk*10000) /(S5K4H5YXKERR_FULL_PERIOD_PIXEL_NUMS + s5k4h5yxkerr.DummyPixels)/frameRate*10;
    }
        else
            MIN_Frame_length = (s5k4h5yxkerr.capPclk*10000) /(S5K4H5YXKERR_FULL_PERIOD_PIXEL_NUMS + s5k4h5yxkerr.DummyPixels)/u2FrameRate;

        if((MIN_Frame_length <=S5K4H5YXKERR_FULL_PERIOD_LINE_NUMS))
        {
            MIN_Frame_length = S5K4H5YXKERR_FULL_PERIOD_LINE_NUMS;
            S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRSetVideoMode] current fps = %d\n", (s5k4h5yxkerr.capPclk*10000) /(S5K4H5YXKERR_FULL_PERIOD_PIXEL_NUMS)/S5K4H5YXKERR_FULL_PERIOD_LINE_NUMS);

        }
        S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRSetVideoMode] current fps (10 base)= %d\n", (s5k4h5yxkerr.pvPclk*10000)*10/(S5K4H5YXKERR_FULL_PERIOD_PIXEL_NUMS + s5k4h5yxkerr.DummyPixels)/MIN_Frame_length);

        extralines = MIN_Frame_length - S5K4H5YXKERR_FULL_PERIOD_LINE_NUMS;

        spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
        s5k4h5yxkerr.DummyPixels = 0;//define dummy pixels and lines
        s5k4h5yxkerr.DummyLines = extralines ;
        spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);

        S5K4H5YXKERR_SetDummy(s5k4h5yxkerr.DummyPixels,extralines);
    }
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRSetVideoMode] MIN_Frame_length=%d,s5k4h5yxkerr.DummyLines=%d\n",MIN_Frame_length,s5k4h5yxkerr.DummyLines);

    return KAL_TRUE;
}

UINT32 S5K4H5YXKERRSetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
    //return ERROR_NONE;
    if(bEnable) {   // enable auto flicker
        S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRSetAutoFlickerMode] enable\n");
        spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
        s5k4h5yxkerr.S5K4H5YXKERRAutoFlickerMode = KAL_TRUE;
        spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
    } else {
    S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRSetAutoFlickerMode] disable\n");
    spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
        s5k4h5yxkerr.S5K4H5YXKERRAutoFlickerMode = KAL_FALSE;
        spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
    }

    return ERROR_NONE;
}

UINT32 S5K4H5YXKERRSetTestPatternMode(kal_bool bEnable)
{
    return TRUE;
}

UINT32 S5K4H5YXKERRMIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate) 
{
    kal_uint32 pclk;
    kal_int16 dummyLine;
    kal_uint16 lineLength,frameHeight;
        
    switch (scenarioId) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRMIPISetMaxFramerateByScenario] MSDK_SCENARIO_ID_CAMERA_PREVIEW\n");
            #if 1
            pclk = 250000000;
            lineLength = S5K4H5YXKERR_PV_PERIOD_PIXEL_NUMS;
            frameHeight = (10 * pclk)/frameRate/lineLength;
            dummyLine = frameHeight - S5K4H5YXKERR_PV_PERIOD_LINE_NUMS;
            s5k4h5yxkerr.sensorMode = SENSOR_MODE_PREVIEW;
            S5K4H5YXKERR_SetDummy(0, dummyLine);    
            #endif
            //S5K4H5YXKERR_SetDummy(0, 0);
            break;          
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRMIPISetMaxFramerateByScenario] MSDK_SCENARIO_ID_VIDEO_PREVIEW\n");
            #if 1
            pclk = 250000000;
            lineLength = S5K4H5YXKERR_VIDEO_PERIOD_PIXEL_NUMS;
            frameHeight = (10 * pclk)/frameRate/lineLength;
            dummyLine = frameHeight - S5K4H5YXKERR_VIDEO_PERIOD_LINE_NUMS;
            s5k4h5yxkerr.sensorMode = SENSOR_MODE_VIDEO;
            S5K4H5YXKERR_SetDummy(0, dummyLine);
            #endif
            //S5K4H5YXKERR_SetDummy(0, 0);
            break;          
             break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:   
            S5K4H5YXKERRDB("[S5K4H5YXKERR] [S5K4H5YXKERRMIPISetMaxFramerateByScenario] MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG or MSDK_SCENARIO_ID_CAMERA_ZSD\n");
            #if 1
            pclk = 250000000;
            lineLength = S5K4H5YXKERR_FULL_PERIOD_PIXEL_NUMS;
            frameHeight = (10 * pclk)/frameRate/lineLength;
            dummyLine = frameHeight - S5K4H5YXKERR_FULL_PERIOD_LINE_NUMS;
            s5k4h5yxkerr.sensorMode = SENSOR_MODE_CAPTURE;
            S5K4H5YXKERR_SetDummy(0, dummyLine);
            #endif
            //S5K4H5YXKERR_SetDummy(0, 0);
            break;      
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
            break;      
        default:
            break;
    }   
    return ERROR_NONE;
}


UINT32 S5K4H5YXKERRMIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate) 
{

    switch (scenarioId) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
             *pframeRate = 300;
             break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
             *pframeRate = 250;
            break;      
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added   
             *pframeRate = 300;
            break;      
        default:
            break;
    }

    return ERROR_NONE;
}

//void S5K4H5YXKERRMIPIGetAEAWBLock(UINT32 *pAElockRet32,UINT32 *pAWBlockRet32)
//{
//    *pAElockRet32 = 1;
//  *pAWBlockRet32 = 1;
//    //SENSORDB("S5K8AAYX_MIPIGetAEAWBLock,AE=%d ,AWB=%d\n,",*pAElockRet32,*pAWBlockRet32);
//}


UINT32 S5K4H5YXKERRFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
                                                                UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 SensorRegNumber;
    UINT32 i;
    PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ENG_INFO_STRUCT  *pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            #ifdef Capture8M
            *pFeatureReturnPara16++= 3264;
            *pFeatureReturnPara16= 2448;
            #else
            *pFeatureReturnPara16++= S5K4H5YXKERR_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16= S5K4H5YXKERR_IMAGE_SENSOR_FULL_HEIGHT;
            #endif
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
                *pFeatureReturnPara16++= S5K4H5YXKERR_FeatureControl_PERIOD_PixelNum;
                *pFeatureReturnPara16= S5K4H5YXKERR_FeatureControl_PERIOD_LineNum;
                *pFeatureParaLen=4;
                break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            switch(S5K4H5YXKERRCurrentScenarioId)
            {
                case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
                    *pFeatureReturnPara32 = 250000000;
                    *pFeatureParaLen=4;
                    break;
                case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                    *pFeatureReturnPara32 = 250000000;
                    *pFeatureParaLen=4;
                    break;  
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                case MSDK_SCENARIO_ID_CAMERA_ZSD:
                    *pFeatureReturnPara32 = 250000000;
                    *pFeatureParaLen=4;
                    break;
                default:
                    *pFeatureReturnPara32 = 250000000;
                    *pFeatureParaLen=4;
                    break;
            }
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            S5K4H5YXKERR_SetShutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            S5K4H5YXKERR_NightMode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            S5K4H5YXKERR_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            //S5K4H5YXKERR_isp_master_clock=*pFeatureData32;
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            S5K4H5YXKERR_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = S5K4H5YXKERR_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            for (i=0;i<SensorRegNumber;i++)
            {
              spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
                S5K4H5YXKERRSensorCCT[i].Addr=*pFeatureData32++;
                S5K4H5YXKERRSensorCCT[i].Para=*pFeatureData32++;
                spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            SensorRegNumber=FACTORY_END_ADDR;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=S5K4H5YXKERRSensorCCT[i].Addr;
                *pFeatureData32++=S5K4H5YXKERRSensorCCT[i].Para;
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            for (i=0;i<SensorRegNumber;i++)
            {
              spin_lock(&s5k4h5yxkerrmipiraw_drv_lock);
                S5K4H5YXKERRSensorReg[i].Addr=*pFeatureData32++;
                S5K4H5YXKERRSensorReg[i].Para=*pFeatureData32++;
                spin_unlock(&s5k4h5yxkerrmipiraw_drv_lock);
            }
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            SensorRegNumber=ENGINEER_END;
            if (*pFeatureParaLen<(SensorRegNumber*sizeof(SENSOR_REG_STRUCT)+4))
                return FALSE;
            *pFeatureData32++=SensorRegNumber;
            for (i=0;i<SensorRegNumber;i++)
            {
                *pFeatureData32++=S5K4H5YXKERRSensorReg[i].Addr;
                *pFeatureData32++=S5K4H5YXKERRSensorReg[i].Para;
            }
            break;
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            if (*pFeatureParaLen>=sizeof(NVRAM_SENSOR_DATA_STRUCT))
            {
                pSensorDefaultData->Version=NVRAM_CAMERA_SENSOR_FILE_VERSION;
                pSensorDefaultData->SensorId=S5K4H5YXKERR_SENSOR_ID;
                memcpy(pSensorDefaultData->SensorEngReg, S5K4H5YXKERRSensorReg, sizeof(SENSOR_REG_STRUCT)*ENGINEER_END);
                memcpy(pSensorDefaultData->SensorCCTReg, S5K4H5YXKERRSensorCCT, sizeof(SENSOR_REG_STRUCT)*FACTORY_END_ADDR);
            }
            else
                return FALSE;
            *pFeatureParaLen=sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pSensorConfigData, &S5K4H5YXKERRSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
            *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
            S5K4H5YXKERR_camera_para_to_sensor();
            break;

        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            S5K4H5YXKERR_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            *pFeatureReturnPara32++=S5K4H5YXKERR_get_sensor_group_count();
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            S5K4H5YXKERR_get_sensor_group_info(pSensorGroupInfo->GroupIdx, pSensorGroupInfo->GroupNamePtr, &pSensorGroupInfo->ItemCount);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            S5K4H5YXKERR_get_sensor_item_info(pSensorItemInfo->GroupIdx,pSensorItemInfo->ItemIdx, pSensorItemInfo);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_SET_ITEM_INFO:
            S5K4H5YXKERR_set_sensor_item_info(pSensorItemInfo->GroupIdx, pSensorItemInfo->ItemIdx, pSensorItemInfo->ItemValue);
            *pFeatureParaLen=sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;

        case SENSOR_FEATURE_GET_ENG_INFO:
            pSensorEngInfo->SensorId = 129;
            pSensorEngInfo->SensorType = CMOS_SENSOR;
           
                    pSensorEngInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_RAW_R; //gb r

            *pFeatureParaLen=sizeof(MSDK_SENSOR_ENG_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
            *pFeatureParaLen=4;
            break;

        case SENSOR_FEATURE_INITIALIZE_AF:
            break;
        case SENSOR_FEATURE_CONSTANT_AF:
            break;
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:
            break;
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            S5K4H5YXKERRSetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            S5K4H5YXKERRGetSensorID(pFeatureReturnPara32);
            break;
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            S5K4H5YXKERRSetAutoFlickerMode((BOOL)*pFeatureData16, *(pFeatureData16+1));
            break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            S5K4H5YXKERRSetTestPatternMode((BOOL)*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
            S5K4H5YXKERRMIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
            break;
        case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
            S5K4H5YXKERRMIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
            break;
        //case SENSOR_FEATURE_GET_AE_AWB_LOCK_INFO:
            //S5K4H5YXKERRMIPIGetAEAWBLock((*pFeatureData32),*(pFeatureData32+1));
            //break;
        default:
            break;
    }
    return ERROR_NONE;
}   /* S5K4H5YXKERRFeatureControl() */


SENSOR_FUNCTION_STRUCT  SensorFuncS5K4H5YXKERR=
{
    S5K4H5YXKERROpen,
    S5K4H5YXKERRGetInfo,
    S5K4H5YXKERRGetResolution,
    S5K4H5YXKERRFeatureControl,
    S5K4H5YXKERRControl,
    S5K4H5YXKERRClose
};

UINT32 S5K4H5YXKERR_MIPI_RAW_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncS5K4H5YXKERR;

    return ERROR_NONE;
}   /* SensorInit() */

