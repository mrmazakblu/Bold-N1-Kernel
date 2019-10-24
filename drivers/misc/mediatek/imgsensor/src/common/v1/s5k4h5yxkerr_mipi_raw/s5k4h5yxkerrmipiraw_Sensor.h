/*******************************************************************************************/


/*******************************************************************************************/
/*BEGIN PN:SPBB-1222 ,MODIFIED BY W00167383,2012/12/26 */

/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H

#define ZSD15FPS

typedef enum group_enum {
    PRE_GAIN=0,
    CMMCLK_CURRENT,
    FRAME_RATE_LIMITATION,
    REGISTER_EDITOR,
    GROUP_TOTAL_NUMS
} FACTORY_GROUP_ENUM;


#define ENGINEER_START_ADDR 10
#define FACTORY_START_ADDR 0

typedef enum engineer_index
{
    CMMCLK_CURRENT_INDEX=ENGINEER_START_ADDR,
    ENGINEER_END
} FACTORY_ENGINEER_INDEX;

typedef enum register_index
{
    SENSOR_BASEGAIN=FACTORY_START_ADDR,
    PRE_GAIN_R_INDEX,
    PRE_GAIN_Gr_INDEX,
    PRE_GAIN_Gb_INDEX,
    PRE_GAIN_B_INDEX,
    FACTORY_END_ADDR
} FACTORY_REGISTER_INDEX;

typedef struct
{
    SENSOR_REG_STRUCT    Reg[ENGINEER_END];
    SENSOR_REG_STRUCT    CCT[FACTORY_END_ADDR];
} SENSOR_DATA_STRUCT, *PSENSOR_DATA_STRUCT;

typedef enum {
    SENSOR_MODE_INIT = 0,
    SENSOR_MODE_PREVIEW,
    SENSOR_MODE_VIDEO,
    SENSOR_MODE_CAPTURE
} S5K4H5YXKERR_SENSOR_MODE;


typedef struct
{
    kal_uint32 DummyPixels;
    kal_uint32 DummyLines;
    
    kal_uint32 pvShutter;
    kal_uint32 pvGain;
    
    kal_uint32 pvPclk;  // x10 480 for 48MHZ
    kal_uint32 videoPclk;
    kal_uint32 capPclk; // x10
    
    kal_uint32 shutter;
    kal_uint32 maxExposureLines;

    kal_uint16 sensorGlobalGain;//sensor gain read from 0x350a 0x350b;
    kal_uint16 ispBaseGain;//64
    kal_uint16 realGain;//ispBaseGain as 1x

    kal_int16 imgMirror;

    S5K4H5YXKERR_SENSOR_MODE sensorMode;

    kal_bool S5K4H5YXKERRAutoFlickerMode;
    kal_bool S5K4H5YXKERRVideoMode;
    
    
}S5K4H5YXKERR_PARA_STRUCT,*PS5K4H5YXKERR_PARA_STRUCT;


    #define S5K4H5YXKERR_IMAGE_SENSOR_FULL_WIDTH                    (3264-64)   
    #define S5K4H5YXKERR_IMAGE_SENSOR_FULL_HEIGHT                   (2448-48)

    /* SENSOR PV SIZE */
    #define S5K4H5YXKERR_IMAGE_SENSOR_PV_WIDTH                  (1632-32)
    #define S5K4H5YXKERR_IMAGE_SENSOR_PV_HEIGHT                 (1224-24)

    #define S5K4H5YXKERR_IMAGE_SENSOR_VIDEO_WIDTH                   (3264-64)
    #define S5K4H5YXKERR_IMAGE_SENSOR_VIDEO_HEIGHT              (1836-36)
    

    /* SENSOR SCALER FACTOR */
    #define S5K4H5YXKERR_PV_SCALER_FACTOR                       3
    #define S5K4H5YXKERR_FULL_SCALER_FACTOR                     1
                                              
    /* SENSOR START/EDE POSITION */         
    #define S5K4H5YXKERR_FULL_X_START                                (2)
    #define S5K4H5YXKERR_FULL_Y_START                                (2)
    #define S5K4H5YXKERR_FULL_X_END                             (3264+150)     
    #define S5K4H5YXKERR_FULL_Y_END                             (2448) 
    #define S5K4H5YXKERR_PV_X_START                              (2)
    #define S5K4H5YXKERR_PV_Y_START                              (2)
    #define S5K4H5YXKERR_PV_X_END                                    (1632) 
    #define S5K4H5YXKERR_PV_Y_END                                    (1224) 
    
    #define S5K4H5YXKERR_VIDEO_X_START                              (2)
    #define S5K4H5YXKERR_VIDEO_Y_START                              (2)
    #define S5K4H5YXKERR_VIDEO_X_END                                    (2160) 
    #define S5K4H5YXKERR_VIDEO_Y_END                                    (1620) 

    #define S5K4H5YXKERR_MAX_ANALOG_GAIN                    (16)
    #define S5K4H5YXKERR_MIN_ANALOG_GAIN                    (1)
    #define S5K4H5YXKERR_ANALOG_GAIN_1X                     (0x0020)

    //#define S5K4H5YXKERR_MAX_DIGITAL_GAIN                 (8)
    //#define S5K4H5YXKERR_MIN_DIGITAL_GAIN                 (1)
    //#define S5K4H5YXKERR_DIGITAL_GAIN_1X                  (0x0100)

    /* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
    #ifdef Capture8M_New
    #define S5K4H5YXKERR_FULL_PERIOD_PIXEL_NUMS                 (0x1FB0) //(0x0Fd8) //4056
    #else
    #define S5K4H5YXKERR_FULL_PERIOD_PIXEL_NUMS                 (0x1F98) //(0x0FCC) //4044
    #endif
    
    #if defined(ZSD15FPS)
        #ifdef Capture8M_New
        #define S5K4H5YXKERR_FULL_PERIOD_LINE_NUMS                  0x9c2 //0x9c2   //2498
        #else
    #define S5K4H5YXKERR_FULL_PERIOD_LINE_NUMS                  0x99C //0x99C   //2452
        #endif
    #else
    //Add dummy lines for 13fps
    #define S5K4H5YXKERR_FULL_PERIOD_LINE_NUMS                  0xB78   //2936
    #endif
    
    #define S5K4H5YXKERR_PV_PERIOD_PIXEL_NUMS                   0x1CD0  //3688
    #define S5K4H5YXKERR_PV_PERIOD_LINE_NUMS                    0x4CA   //1226

    #define S5K4H5YXKERR_VIDEO_PERIOD_PIXEL_NUMS                0x1F98 //0x0FCC //4044
    #define S5K4H5YXKERR_VIDEO_PERIOD_LINE_NUMS             0x072C//0x072C  //1836
    

    #define S5K4H5YXKERR_MIN_LINE_LENGTH                        0x0AA4  //2724
    #define S5K4H5YXKERR_MIN_FRAME_LENGTH                       0x0214  //532
    
    #define S5K4H5YXKERR_MAX_LINE_LENGTH                        0xCCCC
    #define S5K4H5YXKERR_MAX_FRAME_LENGTH                       0xFFFF

    /* DUMMY NEEDS TO BE INSERTED */
    /* SETUP TIME NEED TO BE INSERTED */
    #define S5K4H5YXKERR_IMAGE_SENSOR_PV_INSERTED_PIXELS            2
    #define S5K4H5YXKERR_IMAGE_SENSOR_PV_INSERTED_LINES         2

    #define S5K4H5YXKERR_IMAGE_SENSOR_FULL_INSERTED_PIXELS      4
    #define S5K4H5YXKERR_IMAGE_SENSOR_FULL_INSERTED_LINES       4

#define S5K4H5YXKERRMIPI_WRITE_ID   (0x20)
#define S5K4H5YXKERRMIPI_READ_ID    (0x21)

// SENSOR CHIP VERSION

#define S5K4H5YXKERRMIPI_SENSOR_ID            S5K4H5YXKERR_SENSOR_ID

#define S5K4H5YXKERRMIPI_PAGE_SETTING_REG    (0xFF)

//s_add for porting
//s_add for porting
//s_add for porting

//export functions
UINT32 S5K4H5YXKERRMIPIOpen(void);
UINT32 S5K4H5YXKERRMIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 S5K4H5YXKERRMIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 S5K4H5YXKERRMIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 S5K4H5YXKERRMIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 S5K4H5YXKERRMIPIClose(void);

//#define Sleep(ms) mdelay(ms)
//#define RETAILMSG(x,...)
//#define TEXT

//e_add for porting
//e_add for porting
//e_add for porting
extern int iReadRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData, u16 a_sizeSendData, u16 i2cId);
extern void kdSetI2CSpeed(u16 i2cSpeed);
#endif /* __SENSOR_H */
/*END PN:SPBB-1222 ,MODIFIED BY W00167383,2012/12/26 */
