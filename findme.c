#include <project.h>

#define LED_ON				(0u)
#define LED_OFF				(1u)

#define NO_ALERT			(0u)
#define MILD_ALERT			(1u)
#define HIGH_ALERT			(2u)

#define MATCH0				(0x800u)
#define MATCH1				(0x8000u)

#define	MODE_NONE			0
#define	MODE_ADVERTISING	1
#define	MODE_CONNECTED		2
#define	MODE_ALERT			3

#define	TIMESET(x)	((int32)millisecond == (int32)-(x) ? 1 : millisecond + (x))
#define	TIMEOUT(x)	((x) && (int32)(millisecond - (x)) > 0)

uint8	beatcounter;
uint32	millisecond;
uint8	mode = 0;
uint32	tmoLed;

static CY_ISR(Beat) {
	CySysWdtClearInterrupt(CY_SYS_WDT_COUNTER0_INT);
	beatcounter++;
	switch (mode) {
	case MODE_ADVERTISING:
		if (!(beatcounter & 31)) {
			tmoLed = TIMESET(8);
			LED_Write(LED_ON);
		}
		break;
	case MODE_CONNECTED:
		LED_Write(LED_ON);
		break;
	case MODE_ALERT:
		if (!(beatcounter & 3)) {
			tmoLed = TIMESET(20);
			LED_Write(LED_ON);
		}
		break;
	}
}

static CY_ISR(Mill) {
	millisecond++;
	if (TIMEOUT(tmoLed)) {
		tmoLed = 0;
		LED_Write(LED_OFF);
	}
}

static void IasEvent(uint32 event, void *param) {
	if (event == CYBLE_EVT_IASS_WRITE_CHAR_CMD) {
		uint8 a;
		CyBle_IassGetCharacteristicValue(CYBLE_IAS_ALERT_LEVEL, sizeof a, &a);
		mode = a ? MODE_ALERT : MODE_CONNECTED;
	}
}

static void StackEvent(uint32 event, void *param) {
	switch(event) {
	case CYBLE_EVT_STACK_ON:
	case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
		LED_Write(LED_OFF);
		CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
		mode = MODE_ADVERTISING;
		break;
	case CYBLE_EVT_GAP_DEVICE_CONNECTED:
		mode = MODE_CONNECTED;
		break;
	case CYBLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
//		if (CyBle_GetState() == CYBLE_STATE_DISCONNECTED) {
//			Advertising_LED_Write(LED_OFF);
//			LED_Write(LED_ON);
//			CySysPmSetWakeupPolarity(CY_PM_STOP_WAKEUP_ACTIVE_HIGH);
//			CySysPmStop();
//		}
		break;
	}
}

int main() {
	CyGlobalIntEnable;

	CySysTickStart();
	CySysTickSetCallback(0, Mill);

	CySysWdtSetInterruptCallback(CY_SYS_WDT_COUNTER0, Beat);
	CySysWdtWriteMode(CY_SYS_WDT_COUNTER0, CY_SYS_WDT_MODE_INT);
	CySysWdtWriteMatch(CY_SYS_WDT_COUNTER0, MATCH0);
	CySysWdtWriteClearOnMatch(CY_SYS_WDT_COUNTER0, 1u);
	CySysWdtEnable(CY_SYS_WDT_COUNTER0_MASK);

	CySysWdtWriteMatch(CY_SYS_WDT_COUNTER1, MATCH1);
	CySysWdtWriteMode(CY_SYS_WDT_COUNTER1, CY_SYS_WDT_MODE_RESET);
	CySysWdtWriteClearOnMatch(CY_SYS_WDT_COUNTER1, 1u);
	CySysWdtEnable(CY_SYS_WDT_COUNTER1_MASK);
	if (CyBle_Start(StackEvent) != CYBLE_ERROR_OK)
		CYASSERT(0);
	CyBle_IasRegisterAttrCallback(IasEvent);

	for (;;) {
		CyBle_ProcessEvents();
		CySysWatchdogFeed(CY_SYS_WDT_COUNTER1);
//		CYBLE_BLESS_STATE_T blessState;
//		uint8 intrStatus;
//		CyBle_EnterLPM(CYBLE_BLESS_DEEPSLEEP);
//		intrStatus = CyEnterCriticalSection();
//		blessState = CyBle_GetBleSsState();
//		if (blessState == CYBLE_BLESS_STATE_ECO_ON || blessState == CYBLE_BLESS_STATE_DEEPSLEEP)
//			CySysPmDeepSleep();
//		else if (blessState != CYBLE_BLESS_STATE_EVENT_CLOSE)
//			CySysPmSleep();
//		CyExitCriticalSection(intrStatus);
	}
}
