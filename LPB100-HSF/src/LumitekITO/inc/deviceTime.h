#ifndef __LUMI_DEVICE_TIME_H__
#define __LUMI_DEVICE_TIME_H__

#include <hsf.h>


#define START_TIMER_INTERVAL		5
#define MIN_ABSENCE_OPEN_INTERVAL		20
#define MAX_ABSENCE_OPEN_INTERVAL		30
#define MIN_ABSENCE_CLOSE_INTERVAL		30
#define MAX_ABSENCE_CLOSE_INTERVAL		50

//#define MAX_TIME_THREAD_SLEEP			120
//#define MAX_TIME_THREAD_SLEEP			30
//#define ABSENCE_GAP_BY_SECOND




typedef struct
{
	U16 year;
	U8 month;
	U8 day;
	U8 week;
	U8 hour;
	U8 minute;
	U8 second;
	U16 dayCount;
}TIME_DATA_INFO;



void USER_FUNC checkAbsenceTimerAfterChange(U8 index);
void USER_FUNC checkCountDownTimerAfterChange(U8 index);

//void USER_FUNC deviceTimeThread(void *arg);
void USER_FUNC initTimeCheck(void);

#ifdef LUMITEK_DEBUG_SWITCH
void USER_FUNC getLocalTimeString(S8* timeStr, BOOL needDate);
#endif //LUMITEK_DEBUG_SWITCH

#endif
