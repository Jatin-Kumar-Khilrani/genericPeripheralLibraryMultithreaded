#include <stdio.h>
#include <signal.h>
#include <time.h>
#include "LibInterface.h"

EventTimerEntry *pEventEntry = NULL;

E_JATIN_ERROR_CODES ValidateEvent (E_EVENTS event)
{
	if( (event < EVENT_BUTTON_COMMAND_BUTTON) || (event > EVENT_BUTTON_PTT) )
	{
		return E_JATIN_NOT_SUPPORTED;
	}
	return E_JATIN_SUCCESS;
}

static void TimerHandler(int sig, siginfo_t *si, void *uc)
{
	timer_t *pTimerId = (void **)si->si_value.sival_ptr;
	EventTimerEntry	*TmpEntry = pEventEntry;
	CACHEDVALUE pCache = CreateCache(4);

	while(TmpEntry)
	{
		if(TmpEntry->TimerId == *pTimerId)
		{
	//		printf("%s Event entry found\n",__func__);
			break;
		}
		TmpEntry = (EventTimerEntry*)TmpEntry->next;
	}

	if(TmpEntry)
	{
		pCache->iValue = TmpEntry->TimeInMsec;
		(TmpEntry->CB)(pCache, TmpEntry->CallbackData);
		timer_stop(TmpEntry->TimerId);
		if(TmpEntry->TimerId)
			TmpEntry->TimerId = NULL;
	}
	FreeCache(pCache);
}

void CallBackButton(CachedValue *pCachedVal, void *pData)
{
	int *event = (int *)pData;
	EventTimerEntry *TmpEntry = NULL;

	/* find list by event */
	TmpEntry = pEventEntry;
	while(TmpEntry)
	{
		if(TmpEntry->event == *event)
		{
	//		printf("Event entry found\n");
			break;
		}
		TmpEntry = (EventTimerEntry*)TmpEntry->next;
	}

	if(TmpEntry)
	{
		if(pCachedVal->iValue == 1)
		{
	//		printf("Button pressed, start timer of %d sec\n", TmpEntry->TimeInMsec);
			if(TmpEntry->TimerId == NULL)
			{
				timer_start(&(TmpEntry->TimerId), TmpEntry->TimeInMsec, (void *)TimerHandler, 1, 0);
	//			printf("Timer started\n");
			}
		} else if(pCachedVal->iValue == 0) {
	//		printf("Button released, stop timer if running\n");
			if(TmpEntry->TimerId != NULL)
			{
				timer_stop(TmpEntry->TimerId);
				TmpEntry->TimerId = NULL;
			}
		}
	}
}


E_JATIN_ERROR_CODES RegisterCallbackTimer(E_EVENTS event, void (*FxnCallback)(CachedValue *, void *), int TimeInMsec,  void *pCallbackData)
{
	EventTimerEntry *TmpEntry = NULL;
	int ret = E_JATIN_FAILURE;

	ret = ValidateEvent(event);
	if(ret != E_JATIN_SUCCESS)
	{
		printf("Invalid event for Timer callback\n");
		return E_JATIN_NOT_SUPPORTED;
	}

	TmpEntry = pEventEntry;
	while(TmpEntry)
	{
		if(TmpEntry->event == event)
		{
			printf("Event is already registered for Timer callback\n");
			return E_JATIN_FAILURE;
		}
		TmpEntry = (EventTimerEntry*)TmpEntry->next;
	}

	/* Create amd add data to link list */
	TmpEntry = (EventTimerEntry *) malloc(sizeof(EventTimerEntry));
	if(TmpEntry == NULL)
	{
		printf("Failed to allocate TmpEntry [%s]\n",strerror(errno));
		return E_JATIN_MEM_ALLOC_FAILED;
	}

	memset(TmpEntry, 0, sizeof(EventTimerEntry));

	TmpEntry->CB = FxnCallback;
	TmpEntry->event = event;
	TmpEntry->TimeInMsec = TimeInMsec;
	TmpEntry->CallbackData = pCallbackData;
	TmpEntry->TimerId = NULL;

	if(pEventEntry == NULL)
	{
		pEventEntry = TmpEntry;
	} else {
		TmpEntry->next = pEventEntry;
		pEventEntry = TmpEntry;
	}

	ret = RegisterCallback(EVENT2STR [event], CallBackButton, &(TmpEntry->event));
	if(ret != E_JATIN_SUCCESS) {
		printf("Error in RegisterCallback\n");
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES FreeEventTimerEntry(void)
{
	EventTimerEntry *pEvent;

	while(pEventEntry) {
		pEvent = pEventEntry;
		pEventEntry = (EventTimerEntry *)pEventEntry->next;
		free(pEvent);
	}
	return E_JATIN_SUCCESS;
}
