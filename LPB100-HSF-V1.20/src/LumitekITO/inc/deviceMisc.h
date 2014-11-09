#ifndef __LUMI_DEVICE_MISC_H__
#define __LUMI_DEVICE_MISC_H__

#include <hsf.h>



void USER_FUNC getUtcTimeByMessage(void);
void USER_FUNC sendGetUtcTimeMsg(void);
void USER_FUNC closeNtpMode(void);

void USER_FUNC createHeartBeatTimer(void);
void USER_FUNC changeHeartBeatTimerPeriod(U16 interval);


BOOL USER_FUNC checkSmartlinkStatus(void);;
void USER_FUNC sendSmartLinkCmd(void);
void USER_FUNC checkNeedEnterSmartLink(void);
void USER_FUNC cancelCheckSmartLinkTimer(void);
void USER_FUNC deviceEnterSmartLink(void);

#endif

