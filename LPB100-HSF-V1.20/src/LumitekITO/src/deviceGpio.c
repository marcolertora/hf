/*
******************************
*Company:Lumitek
*Data:2014-10-07
*Author:Meiyusong
******************************
*/

#include "../inc/lumitekConfig.h"

#ifdef CONFIG_LUMITEK_DEVICE
#include <hsf.h>
#include <string.h>
#include <stdio.h>

#include "../inc/itoCommon.h"
#include "../inc/deviceMisc.h"
#include "../inc/asyncMessage.h"
#include "../inc/deviceGpio.h"


#ifdef DEEVICE_LUMITEK_P1
#define BUZZER_RING_PREIOD		60

static BUZZER_STATUS g_buzzer_status = BUZZER_CLOSE;
static BUZZER_RING_INFO buzzerRingInfo;


#ifdef EXTRA_SWITCH_SUPPORT
static BOOL extraSwitchIsHigh;
#endif //EXTRA_SWITCH_SUPPORT

#endif



SWITCH_STATUS USER_FUNC getSwitchStatus(void)
{
	if(hfgpio_fpin_is_high(HFGPIO_F_SWITCH))
	{
		return SWITCH_OPEN;
	}
	return SWITCH_CLOSE;
}


void USER_FUNC setSwitchStatus(SWITCH_STATUS action)
{
	SWITCH_STATUS switchStatus = getSwitchStatus();

	
	if(SWITCH_OPEN == action)
	{
		hfgpio_fset_out_high(HFGPIO_F_SWITCH);
	}
	else
	{
		hfgpio_fset_out_low(HFGPIO_F_SWITCH);
	}
	if(action != switchStatus)
	{
		U8 data = (action == SWITCH_OPEN)?1:0;
		
		insertLocalMsgToList(MSG_LOCAL_EVENT, &data, 1, MSG_CMD_REPORT_GPIO_CHANGE);
	}
}


void USER_FUNC changeSwitchStatus(void)
{
	if(getSwitchStatus() == SWITCH_OPEN)
	{
		setSwitchStatus(SWITCH_CLOSE);
	}
	else
	{
		setSwitchStatus(SWITCH_OPEN);
	}
}


#ifdef DEEVICE_LUMITEK_P1

static void USER_FUNC initBuzzerStatus(void)
{
	hfgpio_fset_out_low(HFGPIO_F_BUZZER);
	g_buzzer_status = BUZZER_CLOSE;
}


static void USER_FUNC setBuzzerStatus(BUZZER_STATUS buzzerStatus)
{
	if(buzzerStatus == BUZZER_OPEN)
	{
		hfgpio_pwm_enable(HFGPIO_F_BUZZER,buzzerRingInfo.pRindData[buzzerRingInfo.ringDataIndex].freq, 50);
		g_buzzer_status = BUZZER_OPEN;
		lumi_debug("fre = %d index=%d\n", buzzerRingInfo.pRindData[buzzerRingInfo.ringDataIndex].freq, buzzerRingInfo.ringDataIndex);
	}
	else
	{
		hfgpio_pwm_disable(HFGPIO_F_BUZZER);
		hfgpio_fset_out_low(HFGPIO_F_BUZZER);
		g_buzzer_status = BUZZER_CLOSE;
	}
}


void USER_FUNC switchBuzzerStatus(void)
{
#if 0
	if(g_buzzer_status == BUZZER_OPEN)
	{
		setBuzzerStatus(BUZZER_CLOSE);
	}
	else
	{
		setBuzzerStatus(BUZZER_OPEN);
	}
#else
	if(buzzerRingInfo.pRindData[buzzerRingInfo.ringDataIndex].freq == 0)
	{
		setBuzzerStatus(BUZZER_CLOSE);
	}
	else
	{
		setBuzzerStatus(BUZZER_OPEN);
	}
#endif
}


void USER_FUNC closeBuzzer(void)
{
	if(g_buzzer_status == BUZZER_OPEN)
	{
		setBuzzerStatus(BUZZER_CLOSE);
	}
}


void USER_FUNC buzzerRingNotice(S32 freq, S32 period, S32 ringTims)
{
	S32  i;

	for(i=0; i<ringTims; i++)
	{
		hfgpio_pwm_enable(HFGPIO_F_BUZZER,freq, 50);
		msleep(period);
		hfgpio_pwm_disable(HFGPIO_F_BUZZER);
		msleep(period);
	}
	hfgpio_fset_out_low(HFGPIO_F_BUZZER);
}


void USER_FUNC initBuzzerRingInfo(const BUZZER_RING_DATA* pRindData)
{
	buzzerRingInfo.startTime = time(NULL);
	buzzerRingInfo.pRindData = pRindData;
	buzzerRingInfo.ringDataIndex = 0;
}


S32 USER_FUNC getBuzzerRingPeriod(BOOL bInit)
{
	S32 period;


	if(bInit || buzzerRingInfo.pRindData[buzzerRingInfo.ringDataIndex].period == 0)
	{
		buzzerRingInfo.ringDataIndex = 0;
	}
	period = buzzerRingInfo.pRindData[buzzerRingInfo.ringDataIndex].period;
	buzzerRingInfo.ringDataIndex++;
	return period;
}


BOOL USER_FUNC checkNeedStopBuzzerRing(void)
{
	BOOL ret = FALSE;
	U32 period;
	
	time_t cutTime = time(NULL);


	period = cutTime - buzzerRingInfo.startTime;
	if(period > BUZZER_RING_PREIOD)
	{
		if(g_buzzer_status == BUZZER_OPEN)
		{
			setBuzzerStatus(BUZZER_CLOSE);
		}
		ret = TRUE;
	}
	return ret;
}



#ifdef EXTRA_SWITCH_SUPPORT

static BOOL USER_FUNC getExtraSwitchStatus(void)
{
	if(hfgpio_fpin_is_high(HFGPIO_F_EXTRA_SWITCH))
	{
		return TRUE;
	}
	return FALSE;
}


static void USER_FUNC extraSwitchIrq(U32 arg1,U32 arg2)
{
	S32 curPinStatus;

	curPinStatus = getExtraSwitchStatus();
	
	if(curPinStatus != extraSwitchIsHigh)
	{
		changeSwitchStatus();
		extraSwitchIsHigh = curPinStatus;
	}
}


static void USER_FUNC registerExtraSwitchInterrupt(void)
{
	//hfgpio_configure_fpin(HFGPIO_F_EXTRA_SWITCH, HFPIO_PULLUP | HFM_IO_TYPE_INPUT);
	hfgpio_configure_fpin(HFGPIO_F_EXTRA_SWITCH, HFM_IO_TYPE_INPUT);
	if(hfgpio_configure_fpin_interrupt(HFGPIO_F_EXTRA_SWITCH, HFPIO_IT_EDGE, extraSwitchIrq, 1)!= HF_SUCCESS)
	{
		lumi_debug("configure HFGPIO_F_EXTRA_SWITCH fail\n");
		return;
	}
	extraSwitchIsHigh = getExtraSwitchStatus();
}

#endif //EXTRA_SWITCH_SUPPORT

#endif //DEEVICE_LUMITEK_P1


void USER_FUNC initDevicePin(BOOL initBeforNormal)
{
	if(initBeforNormal)
	{
#ifdef DEEVICE_LUMITEK_P1
		initBuzzerStatus();
#endif
	}
	else
	{
#ifdef EXTRA_SWITCH_SUPPORT
		registerExtraSwitchInterrupt();
#endif
	}
}

#endif

