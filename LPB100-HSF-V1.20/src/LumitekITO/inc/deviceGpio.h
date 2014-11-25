/*
******************************
*Company:Lumitek
*Data:2014-10-07
*Author:Meiyusong
******************************
*/

#ifndef __DEVICE_GPIO_H__
#define __DEVICE_GPIO_H__

#include <hsf.h>
#include "itoCommon.h"


#ifdef DEEVICE_LUMITEK_P1
typedef struct
{
	time_t startTime;
}BUZZER_RING_INFO;
#endif


void USER_FUNC initDevicePin(BOOL initBeforNormal);

//switch status
SWITCH_STATUS USER_FUNC getSwitchStatus(void);
void USER_FUNC setSwitchStatus(SWITCH_STATUS action);


#ifdef LPB100_DEVLOPMENT_BOARD
//light status
void USER_FUNC switchLightStatus(void);
#elif defined(DEEVICE_LUMITEK_P1)
//buzzer status
void USER_FUNC switchBuzzerStatus(void);
void USER_FUNC initBuzzerRingInfo(void);
BOOL USER_FUNC checkNeedStopBuzzerRing(void);


#ifdef EXTRA_SWITCH_SUPPORT
void USER_FUNC extraSwitchInit(void);
#endif //EXTRA_SWITCH_SUPPORT

#endif //DEEVICE_LUMITEK_P1

#endif

