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
#include "../inc/asyncMessage.h"
#include "../inc/messageDispose.h"
#include "../inc/localSocketUdp.h"
#include "../inc/deviceGpio.h"
#include "../inc/serverSocketTcp.h"
#include "../inc/socketSendList.h"
#include "../inc/deviceTime.h"



//calibrate time interval
#define MAX_CALIBRATE_TIME_INTERVAL			3600000U	//3600*2*1000
#define MAX_FAILD_CALIBRATE_TIME_INTERVAL	10000	//5*60*1000
#define FROM_1900_TO_1970_SEC				2208988800U

#define SSID_TO_SMARTLINK			"TO_SMARTLINK"




static hftimer_handle_t getUtcTimer = NULL;
static hftimer_handle_t getHeartBeatTimer = NULL;
static hftimer_handle_t checkSmarkLinkTimer = NULL;




static void USER_FUNC setRtcTime(time_t time)
{
	struct timeval	iots_tm;

	
	iots_tm.tv_sec = (long)time;	
	iots_tm.tv_usec = 0;
	settimeofday(&iots_tm, NULL);
}


void USER_FUNC sendGetUtcTimeMsg(void)
{
	insertLocalMsgToList(MSG_LOCAL_EVENT, NULL, 0, MSG_CMD_LOCAL_GET_UTC_TIME);
}


static void USER_FUNC getUtcTimerCallback( hftimer_handle_t htimer )
{
	sendGetUtcTimeMsg();
}



static void USER_FUNC createGetUtcTimer(void)
{
	S32 period = 10000;
	
	getUtcTimer = hftimer_create("Get_UTC_Time",period, false, GET_UTC_TIMER_ID, getUtcTimerCallback, 0);
	if(getUtcTimer == NULL)
	{
		lumi_error("creatGetUtcTimer Faild\n");
		return;
	}
	//hftimer_start(getUtcTimer);
	hftimer_change_period(getUtcTimer, period);
}



void USER_FUNC getUtcTimeByMessage(void)
{
	BOOL getSucc = FALSE;
	U32 utcTime;
	U32 timerPeriod = MAX_FAILD_CALIBRATE_TIME_INTERVAL;


	if(getUtcTimer == NULL)
	{
		createGetUtcTimer();
	}
	else
	{
		if(ping(TCP_DATA_IP) != 0)
		{
			lumi_debug("Ping TCP_DATA_IP faild \n");
		}
		else
		{
			if(getUtcTimeFromNetwork(&utcTime))
			{
				if(utcTime > FROM_1900_TO_1970_SEC)
				{				
					utcTime -= FROM_1900_TO_1970_SEC;
					setRtcTime(utcTime);
					getSucc = TRUE;
				}
			}
			if(getSucc)
			{
				timerPeriod = MAX_CALIBRATE_TIME_INTERVAL;
			}
		}
		hftimer_change_period(getUtcTimer, timerPeriod);
		//hftimer_start(getUtcTimer);
	}
}



static void USER_FUNC heartBeatTimerCallback( hftimer_handle_t htimer )
{
	//lumi_debug("heartBeatTimerCallback \n");
	insertLocalMsgToList(MSG_LOCAL_EVENT, NULL, 0, MSG_CMD_HEART_BEAT);
	hftimer_change_period(htimer, 30000); //30S
	//hftimer_start(htimer);
}



void USER_FUNC changeHeartBeatTimerPeriod(U16 interval)
{
	S32 period;

	period = interval*1000; //S to ms
	hftimer_change_period(getHeartBeatTimer, period); //30S
	//hftimer_start(getHeartBeatTimer);
}


void USER_FUNC createHeartBeatTimer(void)
{
	if(getHeartBeatTimer == NULL)
	{
		S32 period = 1000;

		
		getHeartBeatTimer = hftimer_create("HeartBeat Timer",period, false, HEARTBEAT_TIMER_ID, heartBeatTimerCallback, 0);
		//hftimer_start(getHeartBeatTimer);
		hftimer_change_period(getHeartBeatTimer, period);
	}
	else
	{
		changeHeartBeatTimerPeriod(1);
	}
}


void USER_FUNC closeNtpMode(void)
{
	char *words[3]={NULL};
	char rsp[32]={0};
	

	memset(rsp, 0, sizeof(rsp));
	hfat_send_cmd("AT+NTPEN\r\n",sizeof("AT+NTPEN\r\n"),rsp,32);
	if(hfat_get_words(rsp,words, 2)>0)
	{
		if((rsp[0]=='+')&&(rsp[1]=='o')&&(rsp[2]=='k'))
		{
			lumi_debug("AT+NTPEN===>%s\n", words[1]);
			if(strncmp(words[1], "off", 3) != 0)
			{
				hfat_send_cmd("AT+NTPEN=off\r\n",sizeof("AT+NTPEN=off\r\n"),rsp,32);
				if(((rsp[0]=='+')&&(rsp[1]=='o')&&(rsp[2]=='k')))
				{
					hfsys_reset();
				}
			}
		}
	}
}


BOOL USER_FUNC bRuningStaMode(void)
{
	char *words[3]={NULL};
	char rsp[32]={0};
	BOOL ret = FALSE;
	

	memset(rsp, 0, sizeof(rsp));
	hfat_send_cmd("AT+WMODE\r\n",sizeof("AT+WMODE\r\n"),rsp,32);
	if(hfat_get_words(rsp,words, 2)>0)
	{
		if((rsp[0]=='+')&&(rsp[1]=='o')&&(rsp[2]=='k'))
		{
			lumi_debug("AT+WMODE===>%s\n", words[1]);
			if(strncmp(words[1], "STA", 3) == 0)
			{
				ret = TRUE;
			}
		}
	}
	return ret;
}

//About SmarkLink



const BUZZER_RING_DATA buzzerRingData[] = {{2000, 0}, {600, 1500}, {200, 0}, {600, 1500}, {0, 0}};

static void USER_FUNC smartlinkTimerCallback( hftimer_handle_t htimer )
{
#ifdef LPB100_DEVLOPMENT_BOARD
	switchLightStatus();
#elif defined(DEEVICE_LUMITEK_P1)
	switchBuzzerStatus();
	if(checkNeedStopBuzzerRing())
	{
		hftimer_stop(htimer);
		hftimer_delete(htimer);
	}
	else
	{
		S32 preiod;

		preiod = getBuzzerRingPeriod(FALSE); 
		msleep(100);
		hftimer_change_period(htimer, preiod);
	}

#else
	//do nothing
#endif
}



void USER_FUNC deviceEnterSmartLink(void)
{
	hftimer_handle_t smartlinkTimer;
	S32 period = 300;

#ifdef DEEVICE_LUMITEK_P1
	initBuzzerRingInfo(buzzerRingData);
	period = getBuzzerRingPeriod(TRUE);
	smartlinkTimer = hftimer_create("SMARTLINK_TIMER", period, false, SMARTLINK_TIMER_ID, smartlinkTimerCallback, 0);
#else
	smartlinkTimer = hftimer_create("SMARTLINK_TIMER", period, true, SMARTLINK_TIMER_ID, smartlinkTimerCallback, 0);
#endif
	//hftimer_start(smartlinkTimer);
	hftimer_change_period(smartlinkTimer, period);
}


void USER_FUNC sendSmartLinkCmd(void)
{
	char rsp[64]= {0};

	hfat_send_cmd("AT+SMTLK\r\n",sizeof("AT+SMTLK\r\n"),rsp,64);

}



static void USER_FUNC checkSmartLinkTimerCallback( hftimer_handle_t htimer )
{
	lumi_debug("checkSmartLinkTimerCallback \n");
	hftimer_delete(htimer);
	checkSmarkLinkTimer = NULL;
	sendSmartLinkCmd();
}



void USER_FUNC clearDeviceSSIDForSmartLink(void)
{
	char rsp[64]= {0};
	S8 sendCmd[20];


	memset(sendCmd, 0, sizeof(sendCmd));
	sprintf(sendCmd, "AT+WSSID=%s\r\n", SSID_TO_SMARTLINK);
	hfat_send_cmd(sendCmd, sizeof(sendCmd),rsp,64);
	msleep(100);
}


static BOOL USER_FUNC getDeviceSSID(S8* ssidData)
{
	char *words[3]={NULL};
	char rsp[32]={0};
	

	hfat_send_cmd("AT+WSSSID\r\n",sizeof("AT+WSSSID\r\n"),rsp,32);
	if(hfat_get_words(rsp,words, 2)>0)
	{
		if((rsp[0]=='+')&&(rsp[1]=='o')&&(rsp[2]=='k'))
		{
			memcpy(ssidData,words[1], 18);
			lumi_debug("AT+WSSSID===>%s\n", ssidData);
			if(strlen(ssidData) > 0)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}



void USER_FUNC checkNeedEnterSmartLink(void)
{
	BOOL hasSSID;
	S8 ssidData[20];


	memset(ssidData, 0, sizeof(ssidData));
	
	hasSSID = getDeviceSSID(ssidData);	
	if(!hasSSID || strcmp(ssidData, SSID_TO_SMARTLINK) == 0)
	{
		sendSmartLinkCmd();
	}
	else
	{
		S32 period = 30000; //30S

		
		if(checkSmarkLinkTimer == NULL)
		{
			checkSmarkLinkTimer = hftimer_create("check SMARTLINK Timer", period, false, CHECK_SMARTLINK_TIMER_ID, checkSmartLinkTimerCallback, 0);
		}
		hftimer_change_period(checkSmarkLinkTimer, period);

		//hftimer_start(checkSmarkLinkTimer);
	}
}


void USER_FUNC cancelCheckSmartLinkTimer(void)
{
	if(checkSmarkLinkTimer != NULL)
	{
		//hftimer_stop(checkSmarkLinkTimer);
		hftimer_delete(checkSmarkLinkTimer);
		checkSmarkLinkTimer = NULL;
	}
}

#endif

