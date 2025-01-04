/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef CZI_GIMBAL_LIGHT_H
#define CZI_GIMBAL_LIGHT_H

#ifdef __cplusplus
extern "C" {
#endif


typedef enum _GimbalHandle{
    
    GIMBAL_ANGLE    = 0xD0,
    GIMBAL_SWITCH   = 0xD3,
    GIMBAL_BRIGHT   = 0xD4
}E_GimbalHandle;

typedef enum _GimbalSwitch{
    
    SWITCH_CLOSE    = 0x00,
    SWITCH_OPEN     = 0x01,
    SWITCH_FLASH    = 0x02
}E_GimbalSwitch;

typedef struct _GimbalInfo
{
    int alightSwitch;
    int gLightBright;
    int gLightAngle;
    int breathLight;
    E_GimbalSwitch gLightSwitch;
} T_GimbalLightInfo, *PT_GimbalLightInfo;

// T_DjiReturnCode CziGimbal_ControlGimbalSwitch(uint8_t arg);
// T_DjiReturnCode CziGimbal_ControlGimbalBright(uint8_t arg);
// T_DjiReturnCode CziGimbal_ControlGimbalAngle(int arg, int GimbalAngle);
T_DjiReturnCode CziGimbal_ControlGimbalAngle(int GimbalAngle);

#ifdef __cplusplus
}
#endif

#endif
