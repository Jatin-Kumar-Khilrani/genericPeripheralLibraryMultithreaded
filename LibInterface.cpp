
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <linux/input.h>
#include "LibInterface.h"

//global
static int gSysfsRegistrationCount = 1024;
static int gEventRegistrationCount = 1024;
static Table *gpTable = NULL;

#define CALLBACK_FLOW_SEQUENTIAL 0

#define TIME_STRING_LENGTH 19
#define INT_STRING_LENGTH 11
#define FLOAT_STRING_LENGTH 40
#define MINIMUM_STRING_LENGTH 63

#define FILE_NOTIFICATION_FLAGS (IN_CLOSE_WRITE+IN_DELETE)
#define RWFILE_NOTIFICATION_FLAGS (IN_CLOSE_WRITE+IN_DELETE+IN_ACCESS)
#define CREATEMODE (S_IRUSR + S_IWUSR + S_IRGRP + S_IWGRP + S_IROTH + S_IWOTH)
#define CREATEFILEMODE CREATEMODE
#define CREATEDIRMODE (CREATEMODE +  S_IXUSR + S_IXGRP + S_IXOTH)
//*** #define MISSING_FILE_STRING "MISSING FILE"
#define MISSING_FILE_STRING "0"

#define CACHE_TYPE_NONE     0
#define CACHE_TYPE_INT      1
#define CACHE_TYPE_FLOAT    2
#define CACHE_TYPE_STRING   4
#define CACHE_TYPE_TIME     8

#define VALID_NONE      CACHE_TYPE_NONE
#define VALID_INT       CACHE_TYPE_INT
#define VALID_FLOAT     CACHE_TYPE_FLOAT
#define VALID_STRING    CACHE_TYPE_STRING
#define VALID_TIME      CACHE_TYPE_TIME

#define INTVALID(var) (((CachedValue*)(var))->fValid & VALID_INT)
#define FLOATVALID(var) (((CachedValue*)(var))->fValid & VALID_FLOAT)
#define STRINGVALID(var) (((CachedValue*)(var))->fValid & VALID_STRING)
#define TIMEVALID(var) (((CachedValue*)(var))->fValid & VALID_TIME)


int FileExists(char* pFileName, int var_type);
void config_FreeTable(Table* pTable);
Table* config_CreateTable(void);
TableEntry* config_AddRegisteredVariable(Table* pTable, char* varstring, int VarType);
CALLBACK config_AddCallback(TableEntry *pVar, void* pCB, void* pCBD);
CachedValue* RefreshCache(CachedValue* pCache, char* pFileName, int notify_fd, int VarType);
CachedValue* RefreshCacheFromFile(CachedValue* pCache, char* pFileName);
CachedValue* RefreshCacheFromSysfs(CachedValue* pCache, char* pFileName);
CachedValue* RefreshCacheFromEventNode(CachedValue* pCache, char* pFileName, int mode);
TableEntry* FindTableEntryByRegistration(Table* pTable, int RegID);
void CallRegisteredVariableCallbacks(TableEntry *pVar, CACHEDVALUE pCache);
void FreeNotify(Table* pTable);
void StartNotifyThread(Table *pTable);
TableEntry* FindTableEntryByName(Table* pTable, char* varName);

void FreeCache(CachedValue* pCache)
{
	if (pCache && ISCACHE(pCache))
	{
		if (pCache->pString) free(pCache->pString);
		free(pCache);
	}
}

E_JATIN_ERROR_CODES DeInitLibrary(void)
{
	E_JATIN_ERROR_CODES ret = E_JATIN_FAILURE;
	if(gpTable) {
		config_FreeTable(gpTable);
		ret = E_JATIN_SUCCESS;
	}

	FreeEventTimerEntry();

	/* Deinitialize the syslog */
	closelog();
	return ret;
}

E_JATIN_ERROR_CODES InitLibrary(char *appName)
{
	gpTable = config_CreateTable();
	if (gpTable == NULL) {
		printf("Failed to create Config Table");
		return E_JATIN_MEM_ALLOC_FAILED;
	}
	gpTable->pExFdentry = NULL;
	gpTable->pEvtFdentry = NULL;

	/* Initialize syslog for debug logs */
	setlogmask (LOG_UPTO (LOG_DEBUG));
	openlog(appName, LOG_PID|LOG_CONS, LOG_USER);

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES RegisterCallback(char *pVarName, void (*FxnCallback)(CachedValue *, void *), void *pCallbackData)
{
	E_JATIN_ERROR_CODES status = E_JATIN_FAILURE;
	TableEntry *pTE;
	CALLBACK ret = NULL;
	int type = VAR_TYPE_REGISTERED;

	if (!gpTable) {
		INFO_ERROR("Library is not initialized\n");
		//return -E_INIT_FAILED;
		return E_JATIN_FAILURE;
	}

	if (!pVarName) {
		INFO_ERROR("Filename is not defined\n");
		//return -E_PARAM_ERR;
		return E_JATIN_FAILURE;
	}

	/* Parse variable name for identifying device nodes */
	if (!strncmp(pVarName, "/dev/input", strlen("/dev/input")) ) {
		type = VAR_TYPE_EVT;
		INFO_VERBOSE("Variable type selected = VAR_TYPE_EVT\n");

	} else if (!strncmp(pVarName, "/sys/", strlen("/sys/")) ) {
		INFO_VERBOSE("Variable type selected = VAR_TYPE_SYSFS\n");
		type = VAR_TYPE_SYSFS;

	} else {
		INFO_VERBOSE("Default variable type selected = VAR_TYPE_REGISTERED\n");
	}

	pTE = config_AddRegisteredVariable(gpTable, pVarName, type);
	if(pTE) {
		ret = config_AddCallback(pTE, (Callback *)FxnCallback, pCallbackData);
		if(ret != NULL){
			status = E_JATIN_SUCCESS;
		}
	}

	return status;
}

CachedValue* CreateCache(int strlen)
{
	// try to allocate a cache structure
	CachedValue* retVal = (CachedValue* )malloc(sizeof(CachedValue));
	if (retVal)
	{
		// set everything to 0
		memset(retVal, 0, sizeof(CachedValue));
		TYPE(retVal) = TYPE_CACHED_DATA;
		// it doesn't make sense to have tiny strings (in case we need to change values)
		if (MINIMUM_STRING_LENGTH > strlen) strlen = MINIMUM_STRING_LENGTH;
		// allocate space for a string and NULL
		retVal->pString = (char* )malloc(strlen+1);
		if (retVal->pString)
		{
			// set buffer size
			retVal->strBufferSize = strlen+1;
			// 0 everything
			memset(&retVal->pString[0], 0, strlen+1);
		}
		else
		{
			FreeCache(retVal);
			retVal = NULL;
		}
	}
	return retVal;
}

void ConvertStrToCache(CachedValue* cvConvert)
{
	// if time conversion works
	if (sscanf(cvConvert->pString, "%d/%d/%d %d:%d:%d",
			&cvConvert->Time.tm.tm_mon, &cvConvert->Time.tm.tm_mday, &cvConvert->Time.tm.tm_year,
			&cvConvert->Time.tm.tm_hour, &cvConvert->Time.tm.tm_min, &cvConvert->Time.tm.tm_sec) == 6)
	{
		// convert from human to computer
		cvConvert->Time.tm.tm_mon--; // 0-11
		cvConvert->Time.tm.tm_year -= 1900; // years since 1900
		// convert to comparable value
		cvConvert->Time.time = mktime(&cvConvert->Time.tm);
		// did it convert?
		if (cvConvert->Time.time != -1)
		{
			cvConvert->fValid |= VALID_TIME;
		}
	}
	// if float conversion works
	else if (sscanf(cvConvert->pString, "%f", &cvConvert->fValue) == 1)
	{
		cvConvert->iValue = cvConvert->fValue;
		cvConvert->fValid |= VALID_FLOAT+VALID_INT;
	}
	// if int conversion works
	else if (sscanf(cvConvert->pString, "%d", &cvConvert->iValue) == 1)
	{
		cvConvert->fValue = cvConvert->iValue;
		cvConvert->fValid |= VALID_FLOAT+VALID_INT;
	}
}

CachedValue* StrToCache(char* pStr, CachedValue* cvConvert)
{
	CachedValue* retVal = cvConvert;
	if (!cvConvert)
	{
		retVal = CreateCache(strlen(pStr));
	}
	// if space was passed in or malloced
	if (retVal)
	{
		// if we don't have enough room for the whole string
		if (retVal->strBufferSize <= strlen(pStr))
		{
			// save current string
			char* pTemp = retVal->pString;
			// try to duplicate the new string
			retVal->pString = strdup(pStr);
			// string duplicated OK
			if (retVal->pString)
			{
				// free previous string
				free(pTemp);
				// set buffer size
				retVal->strBufferSize = strlen(pStr)+1;
			}
			else
			{
				// restore previous string
				retVal->pString = pTemp;
				// copy only as much as we have space for
				strncpy(retVal->pString, pStr, retVal->strBufferSize-1);
				// be sure this is null terminated
				retVal->pString[retVal->strBufferSize-1] = 0;
			}
		}
		else
		{
			// copy it all
			strcpy(retVal->pString, pStr);
		}
		// right now, only the string is valid
		retVal->fValid = VALID_STRING;
		// try to convert to all types
		ConvertStrToCache(retVal);
	}
	return retVal;
}

int CacheToInt(CachedValue* pData, int** ppResult)
{
	int retVal = FALSE;
	if (pData && INTVALID(pData))
	{
		if (!ppResult[0])
		{
			ppResult[0] = (int*)malloc(sizeof(int));
		}
		if (ppResult[0])
		{
			*(ppResult[0]) = pData->iValue;
			retVal = TRUE;
		}
	}
	return retVal;
}
int CacheToFloat(CachedValue* pData, float** ppResult)
{
	int retVal = FALSE;
	if (pData && FLOATVALID(pData))
	{
		if (!ppResult[0])
		{
			ppResult[0] = (float*)malloc(sizeof(float));
		}
		if (ppResult[0])
		{
			*(ppResult[0]) = pData->fValue;
			retVal = TRUE;
		}
	}
	return retVal;
}

// if the char* is non-NULL, this also requires a buffer size value
int CacheToString(CachedValue* pData, char** ppResult, int buffersize)
{
	int retVal = FALSE;
	if (pData)
	{
		if (!ppResult[0])
		{
            if (strcmp((pData->pString), "")) {
                ppResult[0] = strdup(pData->pString);
                if (ppResult[0])
                    retVal = TRUE;
            }
		}
		else if (buffersize > pData->strBufferSize)
		{
			strcpy(ppResult[0], pData->pString);
			retVal = TRUE;
		}
	}
	return retVal;
}



void InitNotify(Table* pTable)
{
	if (pTable)
	{
		pTable->Notify.Descriptor = -1;
		pTable->Notify.NumberRegistered = 0;
		pTable->ExFdNumberRegistered = 0;
		pTable->EvtFdNumberRegistered = 0;
		pTable->Notify.Thread = 0;
		pTable->Notify.ThreadStarted = -1;
		pTable->Notify.DeathPipe.Parts.ReadFD = -1;
		pTable->Notify.DeathPipe.Parts.WriteFD = -1;
	}
}

void HandleNotifyEvent(TABLE pTable, struct inotify_event* pEventData)
{
	// no message to print yet
	char MessageBuffer[200];
	MessageBuffer[0] = 0;
    CachedValue* pTemp = NULL;

	pthread_mutex_lock(&pTable->mutex);

	// find the table entry for this variable
	TableEntry* pTE = FindTableEntryByRegistration(pTable, pEventData->wd);

	if (pTE)
	{
		pthread_mutex_lock(&pTE->mutex);

		pTemp = RefreshCache(pTE->pCachedValue, pTE->pVarString, TRUE, VAR_TYPE_REGISTERED);
		// if we were NOT able to read the file
		if (pTemp)
		{
			// call all the callbacks
			pTemp->event_mask = pEventData->mask;
			CallRegisteredVariableCallbacks(pTE, pTemp);
		} else {
			sprintf(MessageBuffer, "Couldn't refresh the cached value for %s", pTE->pVarString);
			// error stops here
		}

		pthread_mutex_unlock(&pTE->mutex);
	}
	else
	{
		sprintf(MessageBuffer, "Couldn't find match for %d in table", pEventData->wd);
	}

	pthread_mutex_unlock(&pTable->mutex);

	//// don't do the printf while holding the mutex
	//if (pTE)
	//{
	//	PrintWhatHappened(pTE, pEventData);
	//}

	// don't do the printf while holding the mutex
	if (strlen(MessageBuffer))
	{
		printf("%s\n",MessageBuffer);
	}
}

void HandleInputEvent(TABLE pTable, EventFDEntry *pEvtfd)
{
	struct input_event ev[10];
	int rd = 0, i;
	int fd = pEvtfd->Descriptor;
	char sTempFileName[256] = {0};
	CachedValue* pTemp = NULL;

	pthread_mutex_lock(&pTable->mutex);
	rd = read(fd, ev, sizeof(ev));
	for (i = 0; i < rd / sizeof(struct input_event); i++)
	{
		memset(sTempFileName, 256, sizeof(sTempFileName));
		snprintf(sTempFileName, 256, "%s^%d^%d", pEvtfd->sDeviceNodeName, ev[i].type, ev[i].code);

		// find the table entry for this variable
		TableEntry* pTE = FindTableEntryByName(pTable, sTempFileName);
		if (pTE)
		{
			pTemp = pTE->pCachedValue;
			pthread_mutex_lock(&pTE->mutex);
			//pTemp = RefreshCache(pTE->pCachedValue, pTE->pVarString, ev[i].value, VAR_TYPE_EVT);
			// if we were NOT able to read the file
			if (pTemp)
			{
				// call all the callbacks
				//pTemp->event_mask = pEventData->mask;
				pTemp->iValue = ev[i].value;
				CallRegisteredVariableCallbacks(pTE, pTemp);
			} else {
				//sprintf(MessageBuffer, "Couldn't refresh the cached value for %s", pTE->pVarString);
				// error stops here
			}

			pthread_mutex_unlock(&pTE->mutex);
		} else
		{
			//sprintf(MessageBuffer, "Couldn't find match for %d in table", pEventData->wd);
		}
	}
	pthread_mutex_unlock(&pTable->mutex);
}

void HandleSysfsEvent(TABLE pTable, int registrationID, int fd)
{
	// no message to print yet
	char MessageBuffer[200];
    CachedValue* pTemp = NULL;

	MessageBuffer[0] = 0;
	pthread_mutex_lock(&pTable->mutex);

	// find the table entry for this variable
	TableEntry* pTE = FindTableEntryByRegistration(pTable, registrationID);

	if (pTE)
	{
		pthread_mutex_lock(&pTE->mutex);

		pTemp = RefreshCache(pTE->pCachedValue, pTE->pVarString, TRUE, VAR_TYPE_SYSFS);
		// if we were NOT able to read the file
		if (pTemp)
		{
			// call all the callbacks
			pTemp->event_mask = 0;
			CallRegisteredVariableCallbacks(pTE, pTemp);
		} else {
			sprintf(MessageBuffer, "Couldn't refresh the cached value for %s", pTE->pVarString);
			// error stops here
		}

		pthread_mutex_unlock(&pTE->mutex);
	}
	else
	{
		sprintf(MessageBuffer, "Couldn't find match for %d in table", registrationID);
	}

	pthread_mutex_unlock(&pTable->mutex);

	// don't do the printf while holding the mutex
	if (strlen(MessageBuffer))
	{
		printf("%s\n",MessageBuffer);
	}

	// Once the read is complete the file is read again. This is added to
	// stop select function on read
	if (fd > 0) {
		lseek(fd, 0, SEEK_SET);
		read(fd, MessageBuffer, 50);
	}
}

// the buffer has to be bigger than the number of events in it right now
// BUGBUG this may not be big enough!
#define EVENT_BUFF_LEN (1024*4) // one memory page

int NotifyThread(Table* pTable)
{
	int retVal = FALSE;
	char cPipeData = 0;
	fd_set fdset;
	fd_set exfdset;
	ExceptionFDEntry *pExfd=NULL;
	EventFDEntry *pEvtfd=NULL;
	char buffer[10];

	FD_ZERO(&exfdset);
	FD_ZERO(&fdset);

	// if we should do stuff
	if (pTable)
	{
		// constant select information
		int nfds = ((pTable->Notify.Descriptor > pTable->Notify.DeathPipe.Parts.ReadFD) ?
							pTable->Notify.Descriptor :
							pTable->Notify.DeathPipe.Parts.ReadFD);

		// this will get changed when the other thread writes to death pipe
		while (pTable->Notify.Descriptor >= 0)
		{
			// Add Inotity specific files
			FD_ZERO(&fdset);
			FD_SET(pTable->Notify.Descriptor, &fdset);
			FD_SET(pTable->Notify.DeathPipe.Parts.ReadFD, &fdset);

			// Add Exceptional file descriptor
			if (pTable->EvtFdNumberRegistered > 0)
			{
				pEvtfd = pTable->pEvtFdentry;
				while (pEvtfd) {
					if (pEvtfd->Descriptor > 0) {
						FD_SET(pEvtfd->Descriptor, &fdset);
					}
					if (pEvtfd->Descriptor > nfds)
					{
						nfds = pEvtfd->Descriptor;
					}
					pEvtfd = (EventFDEntry *)pEvtfd->next;
				}
			}

			// wait for a change in a watched file or directory OR the death pipe
			if (select(nfds + 1, &fdset, NULL, &exfdset, NULL) > 0)
			{
				// to get here, select says one of our file descriptors is ready to read
				// if the death pipe was written to
				if (FD_ISSET(pTable->Notify.DeathPipe.Parts.ReadFD, &fdset))
				{
					// Read value to check if ExceptionFD is added
					read(pTable->Notify.DeathPipe.Parts.ReadFD, &cPipeData, 1);
					if (cPipeData == 1) {
						cPipeData = 0;
						if (pTable->ExFdNumberRegistered > 0)
						{
							FD_ZERO(&exfdset);
							pExfd = pTable->pExFdentry;
							while (pExfd) {
								FD_SET(pExfd->Descriptor, &exfdset);
								if (pExfd->Descriptor > nfds)
								{
									nfds = pExfd->Descriptor;
								}
								pExfd = (ExceptionFDEntry *)pExfd->next;
							}
						}
						continue;

					} else if(cPipeData == 2) {
						cPipeData = 0;
						if (pTable->EvtFdNumberRegistered > 0)
						{
							pEvtfd = pTable->pEvtFdentry;
							while (pEvtfd) {
								FD_SET(pEvtfd->Descriptor, &fdset);
								if (pEvtfd->Descriptor > nfds)
								{
									nfds = pEvtfd->Descriptor;
								}
								pEvtfd = (EventFDEntry *)pEvtfd->next;
							}
						}
						continue;

					} else {
						// if we have a descriptor
						if  (pTable->Notify.Descriptor >= 0)
						{
							//printf("close notify descriptor\n");
							// close the notify device
							close(pTable->Notify.Descriptor);
							// set descriptor to stop while loop
							pTable->Notify.Descriptor = -1;
						}
					}
				}
				// if something we are watching changed
				else if (FD_ISSET(pTable->Notify.Descriptor, &fdset))
				{
					char EventBuffer[EVENT_BUFF_LEN];
					// if we can read event data
					int amountread = read(pTable->Notify.Descriptor, EventBuffer, EVENT_BUFF_LEN);
					if (amountread != -1)
					{
						// set to start of buffer
						int indx = 0;
						// while we haven't exhausted the buffer contents
						while(indx < amountread)
						{
							// get a pointer to the current event data structure
							struct inotify_event* pEventData = (struct inotify_event*)(&EventBuffer[indx]);
							// set the index past the current event data buffer
							indx += sizeof(struct inotify_event) + pEventData->len;
							// do the event
							HandleNotifyEvent(pTable, pEventData);
						}
					}
					else
					{
						printf("bad read\n");
						printf("%s\n",strerror(errno));
						sched_yield();
					}
				}

				else
				{
					// Exceptional FD handling
					if (pTable->ExFdNumberRegistered > 0)
					{
						pExfd = pTable->pExFdentry;
						while (pExfd) {
							if (FD_ISSET(pExfd->Descriptor, &exfdset))
							{
								if (pExfd->SkipCallback) {
									HandleSysfsEvent(pTable, pExfd->RegistrationID, pExfd->Descriptor);
								} else {
									pExfd->SkipCallback++;

									// Dummy read
									lseek(pExfd->Descriptor, 0, SEEK_SET);
									read(pExfd->Descriptor, buffer, 10);
								}
							}
							pExfd = (ExceptionFDEntry *)pExfd->next;
						}
					}

					// Event FD handling
					if (pTable->EvtFdNumberRegistered > 0)
					{
						pEvtfd = pTable->pEvtFdentry;
						while (pEvtfd) {
							if (FD_ISSET(pEvtfd->Descriptor, &fdset))
							{
								HandleInputEvent(pTable, pEvtfd);
							}
							pEvtfd = (EventFDEntry *)pEvtfd->next;
						}
					}

				}
			}
		}
	}
	else
	{
		printf("Bad table pointer\n");
	}

	// clean up death pipe
	close(pTable->Notify.DeathPipe.Parts.ReadFD);
	close(pTable->Notify.DeathPipe.Parts.WriteFD);

	//printf("Notify thread exiting\n");

	return retVal;
}

#define DELIM_EVENTNODE_NAME	"^"

int AddEventNodeRegistration (Table *pTable, char *pFileName, int iFlags, int RegistrationToken, int type)
{
	int retVal = RegistrationToken;
	EventFDEntry *pEvtfd;
	char *token = NULL;
	int field = 0;
	char *pEvName = NULL;
	char *pDevNodeName = NULL;

	if ((retVal == BROKENREGISTRATION) && pTable)
	{
		StartNotifyThread(pTable);

		//
		pEvName = strdup (pFileName);
		/* Parse data from filename */
		// Sample:  pFileName = "/dev/input/event0^EV_KEY^123"
		token = strtok(pEvName, (const char *) DELIM_EVENTNODE_NAME);
		while( token != NULL ) {

			switch (field) {
				/* Device node */
				case 0:
					pDevNodeName = strdup(token);
					break;
			}
			token = strtok(NULL, (const char *) DELIM_EVENTNODE_NAME);
			field++;
		}

		/* Check of Node is already registered */
		pEvtfd = pTable->pEvtFdentry;
		while (pEvtfd) {
			if(!strcmp(pEvtfd->sDeviceNodeName, pDevNodeName))
			{
				retVal = ++gEventRegistrationCount;
				break;
			}
			pEvtfd = (EventFDEntry *)pEvtfd->next;
		}

		if (pDevNodeName) {
			free (pDevNodeName);
		}

		if(pEvtfd == NULL) {
			//pEvName = strdup (pFileName);
			/* Create new node */
			EventFDEntry *tmpEntry = (EventFDEntry *)malloc(sizeof(EventFDEntry));
			/* Populate node data */
			memset(tmpEntry, 0x0, sizeof(EventFDEntry));

			if (!tmpEntry) {
				if (pEvName)
					free (pEvName);

				return retVal;
			}

			/* Parse data from filename */
			// Sample:  pFileName = "/dev/input/event0^EV_KEY^123"
			token = strtok(pEvName, (const char *) DELIM_EVENTNODE_NAME);
			field = 0;

			while( token != NULL ) {

				switch (field) {
					/* Device node */
					case 0:
						INFO_VERBOSE("Device node = %s\n", token);
						tmpEntry->sDeviceNodeName = strdup(token);
						break;

					/* Input event type */
					case 1:
						INFO_VERBOSE("Event type = %s\n", token);
						tmpEntry->type = atol(token);
						break;

					/* Input event code */
					case 2:
						INFO_VERBOSE("Event code = %s\n", token);
						tmpEntry->code = atol(token);
						break;
					}

				token = strtok(NULL, (const char *) DELIM_EVENTNODE_NAME);
				field++;
			}

			tmpEntry->Descriptor = open(tmpEntry->sDeviceNodeName, O_RDONLY | O_NONBLOCK);
			//tmpEntry->filename = strdup(pFileName);
			pTable->EvtFdNumberRegistered++;
			retVal = tmpEntry->RegistrationID = ++gEventRegistrationCount;

			/* Add node to Head node */
			tmpEntry->next = pTable->pEvtFdentry;
			pTable->pEvtFdentry = tmpEntry;
		}

		if (pEvName != NULL) {
			free(pEvName);
		}
	}

	return retVal;
}

void StartNotifyThread(Table *pTable)
{
	if (pTable) {

		// if notify is not running
		if (pTable->Notify.Descriptor < 0)
		{
			// initialize the iNotify subsystem
			pTable->Notify.Descriptor = inotify_init();
			// if init worked
			if (pTable->Notify.Descriptor >= 0)
			{
				// try to create the death pipe
				if (pipe(pTable->Notify.DeathPipe.Descriptors.pfd) != -1)
				{
					pthread_attr_t attr;
					pthread_attr_init(&attr);

					// start the thread
					pTable->Notify.ThreadStarted = pthread_create(&pTable->Notify.Thread, &attr, (void *(*) (void *))NotifyThread, (void*)pTable);
					// did it fail to start?
					if (pTable->Notify.ThreadStarted != 0)
					{
						// clean up stuff from above
						close(pTable->Notify.Descriptor);
						close(pTable->Notify.DeathPipe.Parts.ReadFD);
						close(pTable->Notify.DeathPipe.Parts.WriteFD);
						// reset all notify table values
						InitNotify(pTable);
					}
					else
					{
						// let the other threads run
						sched_yield();
						sched_yield();
					}

					pthread_attr_destroy(&attr);
				}
				else
				{
					printf("Could not create death pipe [%s]\n", strerror(errno));
					// clean up stuff from above
					close(pTable->Notify.Descriptor);
					// reset all notify table values
					InitNotify(pTable);
				}
			}
		}
	}

	return;
}

int AddRegistration(Table* pTable, char* pFileName, int iFlags, int RegistrationToken, int type)
{
	int retVal = RegistrationToken;

	if ((retVal == BROKENREGISTRATION) && pTable)
	{
		StartNotifyThread(pTable);
		// if notify is running
		if (pTable->Notify.Descriptor >= 0)
		{
			switch (type) {

				case VAR_TYPE_REGISTERED:
					// add a watch for the name
					retVal = inotify_add_watch(pTable->Notify.Descriptor, pFileName, iFlags);
					if (retVal >= 0)
					{
						pTable->Notify.NumberRegistered++;
						//printf("Added a watch (%d) for %s\n", retVal, pFileName);
					}
					break;

				case VAR_TYPE_SYSFS:
					{
						ExceptionFDEntry *tmpEntry = (ExceptionFDEntry *)malloc(sizeof(ExceptionFDEntry));
						if (tmpEntry) {
							memset(tmpEntry, 0x0, sizeof(ExceptionFDEntry));
							tmpEntry->Descriptor = open(pFileName, O_RDONLY | O_NONBLOCK);
							pTable->ExFdNumberRegistered++;

							retVal = tmpEntry->RegistrationID = ++gSysfsRegistrationCount;

							if (pTable->pExFdentry == NULL)
							{ // first entry
								pTable->pExFdentry = tmpEntry;
							} else {
								tmpEntry->next = pTable->pExFdentry;
								pTable->pExFdentry = tmpEntry;
							}
						}
					}
					break;
			}
		}
	}

	return retVal;
}

int RemoveRegistration(Table* pTable, int RegistrationToken)
{
	int retVal = BROKENREGISTRATION;

	if (pTable &&
		pTable->Notify.NumberRegistered &&
		(RegistrationToken != BROKENREGISTRATION))
	{
		// if we can remove a previously added watch
		if (inotify_rm_watch(pTable->Notify.Descriptor, RegistrationToken) == 0)
		{
			pTable->Notify.NumberRegistered--;
		}
	}

	if (pTable->Notify.NumberRegistered == 0)
	{
		FreeNotify(pTable);
	}

	return retVal;
}

void FreeNotify(Table* pTable)
{
	if (pTable)
	{
		// did the thread start?
		if (pTable->Notify.ThreadStarted == 0)
		{
			// write to death pipe to make thread kill itself
			//printf("Writing to death pipe\n");
			write(pTable->Notify.DeathPipe.Parts.WriteFD, "", 1);

			//printf("Waiting for thread to exit\n");
			// join to wait for thread to end
			void* pResult;
			pthread_join(pTable->Notify.Thread, &pResult);

			// Free Except FDS registered for callback
			if (pTable->ExFdNumberRegistered) {

				ExceptionFDEntry *pExfd;

				while(pTable->pExFdentry) {
					pExfd = pTable->pExFdentry;
					pTable->pExFdentry = (ExceptionFDEntry *)pTable->pExFdentry->next;
					free(pExfd);
				}
			}

			if (pTable->EvtFdNumberRegistered) {

				EventFDEntry *pEvtfd;

				while(pTable->pEvtFdentry) {
					pEvtfd = pTable->pEvtFdentry;
					pTable->pEvtFdentry = (EventFDEntry *)pTable->pEvtFdentry->next;
					close(pEvtfd->Descriptor);
					free(pEvtfd->sDeviceNodeName);
					free(pEvtfd);
				}
			}

			// reset all notify table values
			InitNotify(pTable);
		}
	}
}

TableEntry* FindTableEntryByName(Table* pTable, char* varName)
{
	TableEntry* retVal = pTable->pTableEntries;
	while (retVal && (strcmp(retVal->pVarString, varName) != 0))
	{
		retVal = (TableEntry* )retVal->pNext;
	}

	return retVal;
}

TableEntry* FindTableEntryByRegistration(Table* pTable, int RegID)
{
	TableEntry* retVal = pTable->pTableEntries;
	while (retVal && (TOTABLE(retVal)->RegistrationID != RegID))
	{
		retVal = (TableEntry* )retVal->pNext;
	}
	return retVal;
}

int RegisterTableEntry(TableEntry* pTE)
{
	int retVal = BROKENREGISTRATION;
	int varType = VARTYPE(pTE);

	if (!pTE)
	{
		return retVal;
	}

	switch (varType)
	{
		case VAR_TYPE_REGISTERED:
		case VAR_TYPE_SYSFS:
			retVal = TOTABLE(pTE)->RegistrationID = AddRegistration(TOTABLE(pTE)->pTable,
														pTE->pVarString,
														FILE_NOTIFICATION_FLAGS,
														TOTABLE(pTE)->RegistrationID,
														VARTYPE(pTE));
			break;

		case VAR_TYPE_EVT:
			retVal = TOTABLE(pTE)->RegistrationID = AddEventNodeRegistration(TOTABLE(pTE)->pTable,
														pTE->pVarString,
														FILE_NOTIFICATION_FLAGS,
														TOTABLE(pTE)->RegistrationID,
														VARTYPE(pTE));
			break;
	}

	return retVal;
}

int UnRegisterTableEntry(TableEntry* pTE)
{
	int retVal = BROKENREGISTRATION;
	if (pTE && HASREGISTRATION(pTE))
	{
		retVal = RemoveRegistration(TOTABLE(pTE)->pTable, TOTABLE(pTE)->RegistrationID);
	}
	return retVal;
}

void AddCallback(Callback** ppFirst, Callback* pNew)
{
	pNew->pNext = ppFirst[0];
	pNew->pPrev = NULL;
	ppFirst[0] = pNew;
	if (pNew->pNext)
	{
		pNew->pNext->pPrev = pNew;
	}
}

void AddTableEntry(TableEntry* pTE)
{
	Table* pTable = pTE->pTable;
	// make this one the first in the list
	pTE->pNext = pTable->pTableEntries;
	pTable->pTableEntries = pTE;
}

void FreeCallbackChain(Callback* pCB)
{
	if (pCB)
	{
		// go to the bottom
		FreeCallbackChain(pCB->pNext);
		if(pCB->pCachedValue)
			FreeCache(pCB->pCachedValue);
		free(pCB);
	}
}

TableEntry* FreeTableEntry(TableEntry* pTE)
{
	if (pTE)
	{
		pthread_mutex_lock(&pTE->mutex);

		if (HASREGISTRATION(pTE) && !HASBROKENREGISTRATION(pTE))
		{
			TOTABLE(pTE)->RegistrationID = UnRegisterTableEntry(pTE);
		}
		FreeCallbackChain(pTE->pFirstCallback);

		if (HASCACHE(pTE) && pTE->pCachedValue)
		{
			FreeCache(pTE->pCachedValue);
		}
		if (pTE->pVarString)
		{
			free(pTE->pVarString);
		}
		pthread_mutex_unlock(&pTE->mutex);
		pthread_mutex_destroy(&pTE->mutex);
		free(pTE);
	}
	return NULL;
}

void FreeTableEntryRecursive(TableEntry* pTE)
{
	if (pTE)
	{
		// go all the way to the bottom
		FreeTableEntryRecursive((TableEntry* )pTE->pNext);
		// free this one
		FreeTableEntry(pTE);
	}
}

Callback* CreateCallback(TableEntry *pTE, void* pCB, void* pCBD)
{
	Callback* retVal = (Callback*)malloc(sizeof(Callback));
	if (retVal)
	{
		memset(retVal, 0, sizeof(Callback));
		VARTYPE(retVal) = TYPE_CALLBACK;
		retVal->pData = pCBD;
		retVal->Style.Function = pCB;
		//retVal->pVariable = pTE;
	}
	return retVal;
}

TableEntry *
CreateRegisteredTableEntry(
		Table* pTable,
		char* varstring,
		int var_type,
		int Register)
{
	TableEntry* pTE = TOTABLE(malloc(sizeof(TableEntry)));
	if (pTE)
	{
		// preset for failure
		int failed=TRUE;
		int success = 0;

		memset(pTE, 0, sizeof(TableEntry));
		pTE->pVarString = strdup(varstring);
		pTE->pTable = pTable;
		pTE->RegistrationID = BROKENREGISTRATION;

		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		// allow the same thread to lock/unlock the mutex multiple times
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&pTE->mutex, &attr);
		pthread_mutexattr_destroy(&attr);

		if (pTE->pVarString)
		{
			VARTYPE(pTE) = var_type;
			// make sure file exists
			if (FileExists(varstring, var_type))
			{
				// get the current value from the filesystem
				pTE->pCachedValue = RefreshCache(NULL, varstring, FALSE, var_type);
				if (pTE->pCachedValue)
				{
					// set for success before doing optional part
					failed = FALSE;

					// if we should register this TE
					if (Register)
					{
						// register this file
						RegisterTableEntry(TOTABLE(pTE));
						// registration failed
						if (HASBROKENREGISTRATION(pTE))
						{
							failed = TRUE;
						}
					}

					if (var_type == VAR_TYPE_SYSFS) {
						success = 1;
						// Send signal to Notify thread
						write(pTable->Notify.DeathPipe.Parts.WriteFD, &success, 1);
					}

					if (var_type == VAR_TYPE_EVT) {
						success = 2;
						// Send signal to Notify thread
						write(pTable->Notify.DeathPipe.Parts.WriteFD, &success, 1);
					}
				}
			}
		}

		// something bad happened
		if (failed == TRUE)
		{
			if (pTE) {
				pTE = TOTABLE(FreeTableEntry((TableEntry*)pTE));
				pTE = NULL;
			}
		}
	}

	return pTE;
}

TableEntry* CreateDefaultTableEntry(Table* pTable, char* varstring, int var_type, int Register)
{
	return TOTABLE(CreateRegisteredTableEntry(pTable, varstring, var_type, Register));
}

#ifdef CALLBACK_FLOW_SEQUENTIAL
// call this while holding the table and variable mutex
void CallRegisteredVariableCallbacks(TableEntry *pVar, CACHEDVALUE pCache)
{
	CALLBACK pCB = pVar->pFirstCallback;

	while (pCB)
	{
		(pCB->Style.Registered.CB)(pCache, pCB->pData);
		pCB = pCB->pNext;
	}
}

#else

void *CallbackThread(void *arg)
{
	CALLBACK pCB = (CALLBACK)arg;

	(pCB->Style.Registered.CB)(pCB->pCachedValue, pCB->pData);

	if(pCB->pCachedValue)
		FreeCache(pCB->pCachedValue);
	free(pCB);
	return NULL;
}

// call this while holding the table and variable mutex
void CallRegisteredVariableCallbacks(TableEntry *pVar, CACHEDVALUE pCache)
{
	CALLBACK pCB = pVar->pFirstCallback;
    char *pTemp = NULL;
	pthread_t CallbackTid;
	int ret = -1;

	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	while (pCB)
	{

		CALLBACK pCB1 = (CALLBACK)malloc(sizeof(Callback));
		if(pCB1) {
			memcpy(pCB1,pCB,sizeof(Callback));
			pCB1->pCachedValue = CreateCache(4);
			pTemp = pCB1->pCachedValue->pString;
			memcpy(pCB1->pCachedValue, pCache, sizeof(CachedValue));
			pCB1->pCachedValue->pString = pTemp;
			strcpy(pCB1->pCachedValue->pString, pCache->pString);
			ret = pthread_create(&CallbackTid, &attr, CallbackThread, (void*)pCB1);
			if(ret != 0)
			{
				printf("Error: pthread_create error [%s]\n",strerror(errno));
				FreeCache(pCB1->pCachedValue);
				free(pCB1);
			}
		} else {
			// TODO: Free pCache
		}
		//		(pCB->Style.Registered.CB)(pCache, pCB->pData);
		pCB = pCB->pNext;
	}
}
#endif

CALLBACK config_AddCallback(TableEntry *pVar, void* pCB, void* pCBD)
{
	CALLBACK retVal = NULL;
	if (pVar && (ISEVT(pVar) || ISVAR(pVar)) && pCB)
	{
		pthread_mutex_lock(&pVar->mutex);

		retVal = CreateCallback(pVar, pCB, pCBD);
		if (retVal)
		{
			AddCallback(&pVar->pFirstCallback, retVal);
		}

		pthread_mutex_unlock(&pVar->mutex);

	}
	return retVal;
}


TableEntry* config_AddRegisteredVariable(Table* pTable, char* varstring, int VarType)
{
	TableEntry *retVal = NULL;
	TableEntry *pTE = NULL;

	// validate required inputs
	if (pTable && ISTABLE(pTable) && varstring)
	{
		pthread_mutex_lock(&pTable->mutex);

		pTE = FindTableEntryByName(pTable, varstring);
		if (!pTE)
		{
			pTE = CreateDefaultTableEntry(pTable, varstring, VarType, TRUE);
			if (pTE)
			{
				pthread_mutex_lock(&pTE->mutex);
				AddTableEntry((TableEntry*)pTE);
				pthread_mutex_unlock(&pTE->mutex);
			}
		}

		pthread_mutex_unlock(&pTable->mutex);

		retVal = TOTABLE(pTE);
	}

	return retVal;
}

int config_CacheToInt(CachedValue* pCache, int** ppi)
{
	int retVal = FALSE;
	if (pCache && ISCACHE(pCache) && ppi)
	{
		retVal = CacheToInt(pCache, ppi);
	}
	return retVal;
}

int config_CacheToFloat(CachedValue* pCache, float** ppf)
{
	int retVal = FALSE;
	if (pCache && ISCACHE(pCache) && ppf)
	{
		retVal = CacheToFloat(pCache, ppf);
	}
	return retVal;
}

int config_CacheToString(CachedValue* pCache, char** ppStr, int size)
{
	int retVal = FALSE;
	if (pCache && ISCACHE(pCache) && ppStr)
	{
		retVal = CacheToString(pCache, ppStr, size);
	}
	return retVal;
}

void config_FreeCache(CachedValue* pCache)
{
	// if (pCache && ISCACHE(pCache)) this is done in the internal routine
	{
		FreeCache(pCache);
	}
}


// remove spaces, tabs, CR and LF from the front and back of the passed string
void StripWS(char* pStr)
{
	int indx=0;
	// while we have white space at the front
	while ((pStr[indx] == 0x0a) ||
			(pStr[indx] == 0x0d) ||
			(pStr[indx] == 0x09) ||
			(pStr[indx] == ' '))
	{
		indx++;
	}
	// if front not the same as back
	if (0 != indx)
	{
		strcpy(&pStr[0], &pStr[indx]);
	}

	indx = strlen(pStr);
	// while we have white space at the back
	while ( (indx) && // make sure we don't overrun the front
			((pStr[indx-1] == 0x0a) ||
			(pStr[indx-1] == 0x0d) ||
			(pStr[indx-1] == 0x09) ||
			(pStr[indx-1] == ' ')))
	{
		indx--;
	}
	// set one beyond first non-white space to 0
	pStr[indx]=0;

}

int CreateDirectory_r(char* pDir)
{
	int retVal = FALSE;

	// create directory structure in the file system

	int len = strlen(pDir);
	{
		// make a copy to work on
		char WorkDir[len+1];
		strcpy(WorkDir, pDir);
		// these are for parsing
		char rebuilt[len+1];
		char* section;
		char* saveptr = NULL;
		int result = -1;

		// root based directory?
		if (WorkDir[0] == '/')
		{
			// pre-pend the root indicator
			strcpy(rebuilt, "/");
		}
		else
		{
			// NULL rebuilt directory string
			rebuilt[0] = 0;
		}

		// find first section
		section = strtok_r(WorkDir, "/", &saveptr);

		errno = EEXIST;
		// section non-null, mkdir failed and the reason is it exists
		while (section && (result != 0) && (errno == EEXIST))
		{
			// add section
			strcat(rebuilt, section);
			strcat(rebuilt, "/");
			// try to create
			result = mkdir(rebuilt, CREATEDIRMODE);
			// find next section
			section = strtok_r(NULL, "/", &saveptr);
		}
		// above created a section
		if (result == 0)
		{
			while (section && (result == 0))
			{
				strcat(rebuilt, section);
				strcat(rebuilt, "/");
				// try to create
				result = mkdir(rebuilt, CREATEDIRMODE);
				// find next section
				section = strtok_r(NULL, "/", &saveptr);
			}

			if (result == 0)
			{
				retVal = TRUE;
			}
		}
	}

	return retVal;
}

CachedValue* RefreshCache(CachedValue* pCache, char* pFileName, int mode, int VarType)
{
	CachedValue *pCachedVal = NULL;

	switch (VarType)
	{
		case VAR_TYPE_REGISTERED:
			pCachedVal = RefreshCacheFromFile(pCache, pFileName);
			break;

		case VAR_TYPE_SYSFS:
			pCachedVal = RefreshCacheFromSysfs(pCache, pFileName);
			break;

		case VAR_TYPE_EVT:
			pCachedVal = RefreshCacheFromEventNode(pCache, pFileName, mode);
			break;
	}

	return pCachedVal;
}

#define DUMMY_EVENTNODE_REGISTER_VAL	"0"

CachedValue* RefreshCacheFromEventNode(CachedValue* pCache, char* pFileName, int fd)
{
	CachedValue* pData = NULL;
	int readlen = 0;
	off_t filelen = 50;

	// mode = FALSE during event node registration
	if (fd == FALSE) {
		return StrToCache(DUMMY_EVENTNODE_REGISTER_VAL, pCache);
	}

	// open the file for reading
	// allocate a buffer big enough for the file + 1 (null terminator)
	char filebuffer[filelen+1];

	// read the file into the space
	readlen = read(fd, filebuffer, filelen);
	if (readlen > 0) {
		// null terminate
		filebuffer[filelen] = 0;
		// remove white space from the front and back
		StripWS(filebuffer);
		// convert to cache
		pData = StrToCache(filebuffer, pCache);
	} else
	{
		//printf("Could not read all of the file %s\n", pFileName);
		//printf("%s\n",strerror(errno));
	}


	return pData;
}

CachedValue* RefreshCacheFromSysfs(CachedValue* pCache, char* pFileName)
{
	CachedValue* pData = NULL;
	int readlen = 0;
	off_t filelen = 50;

	// open the file for reading
	int fd = open(pFileName, O_RDONLY);
	if (fd >= 0)
	{
		{
			// allocate a buffer big enough for the file + 1 (null terminator)
			char filebuffer[filelen+1];

			// read the file into the space
			readlen = read(fd, filebuffer, filelen);
			if (readlen > 0) {
				// null terminate
				filebuffer[filelen] = 0;
				// remove white space from the front and back
				StripWS(filebuffer);
				// convert to cache
				pData = StrToCache(filebuffer, pCache);
			}
			else
			{
				//printf("Could not read all of the file %s\n", pFileName);
				//printf("%s\n",strerror(errno));
			}

		}

		close(fd);
	}
	else
	{
		//printf("Could not open file %s (RD_ONLY)\n", pFileName);
		//printf("%s\n",strerror(errno));
	}

	return pData;
}

CachedValue* RefreshCacheFromFile(CachedValue* pCache, char* pFileName)
{
	CachedValue* pData = NULL;
	int readlen = 0;
	off_t filelen = 50;

	// open the file for reading
	int fd = open(pFileName, O_RDONLY);
	if (fd >= 0)
	{
		lseek(fd, 0, SEEK_SET);
		filelen = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);

		{
			// allocate a buffer big enough for the file + 1 (null terminator)
			char filebuffer[filelen+1];

			// read the file into the space
			readlen = read(fd, filebuffer, filelen);
			if (readlen == filelen) {
				// null terminate
				filebuffer[filelen] = 0;
				// remove white space from the front and back
				StripWS(filebuffer);
				// convert to cache
				pData = StrToCache(filebuffer, pCache);
			}
			else
			{
				//printf("Could not read all of the file %s\n", pFileName);
				//printf("%s\n",strerror(errno));
			}
		}

		close(fd);
	}
	else
	{
		//printf("Could not open file %s (RD_ONLY)\n", pFileName);
		//printf("%s\n",strerror(errno));
	}

	return pData;
}

int DirExists(char* pDirName)
{
	int retVal = FALSE;

	DIR *pDir = opendir(pDirName);
	if (pDir)
	{
		retVal = TRUE;
		closedir(pDir);
	}
	else
	{
		//printf("Could not open directory %s\n", pDirName);
		//printf("%s\n",strerror(errno));

		if (CreateDirectory_r(pDirName) == TRUE)
		{
			// retry
			pDir = opendir(pDirName);
			if (pDir)
			{
				retVal = TRUE;
				closedir(pDir);
				//printf("Created %s\n", pDirName);
			}
			else
			{
				//printf("Not created\n");
			}
		}
		else
		{
			//printf("Not created\n");
		}

	}

	return retVal;
}

#define MAX_TMPSTR_LEN	(100)

int FileExists(char* pFileName, int var_type)
{
	int retVal = FALSE;
	char tmpStrVar[MAX_TMPSTR_LEN] = {0};
	char *token = NULL;

	/* In case of SYSFS enries, check the existance of file is sysfs */
	if ((var_type == VAR_TYPE_SYSFS)
	) {
		return !access(pFileName, R_OK) ? TRUE : FALSE;
	}

	/* In case of event entries, check the event node only */
	if (var_type == VAR_TYPE_EVT) {
		/* Parse string to extract device node. Read the first 10 bytes of string */
		memset(tmpStrVar, 0x0, sizeof(tmpStrVar));
		strcpy(tmpStrVar, pFileName);
		token = strtok(tmpStrVar, (const char *) DELIM_EVENTNODE_NAME);
		if (token)
			return (access(tmpStrVar, F_OK) == 0) ? TRUE : FALSE;
		else
			return FALSE;
	}

	// open the file for reading
	int fd = open(pFileName, O_RDONLY);
	if (fd < 0)
	{
		//printf("Could not open file %s (RD_ONLY)\n", pFileName);
		//printf("%s\n",strerror(errno));

		int strLength = strlen(pFileName);
		{
			char WorkDir[strLength+1];
			// make a copy to work on
			strcpy(WorkDir, pFileName);
			// find last slash in path (directory structure)
			char* lastslash = rindex(WorkDir, '/');
			// if there is a path
			if (lastslash)
			{
				// null terminate at slash
				lastslash[0] = 0;
				// try the path
				DirExists(WorkDir);
			}
		}

		// try to create the file
		fd = open(pFileName, O_CREAT | O_TRUNC | O_RDWR, CREATEFILEMODE);
		if (fd >= 0)
		{
			// save data to file
			write(fd, MISSING_FILE_STRING, strlen(MISSING_FILE_STRING));
			//printf("Created %s\n", pFileName);
			retVal = TRUE;
			close(fd);
		}
		else
		{
			//printf("Could not open file %s (RDWR)\n", pFileName);
			//printf("%s\n",strerror(errno));
		}
	}
	else
	{
		retVal = TRUE;
        close(fd);
	}

	return retVal;
}

// read the contents of a file
char* ReadFile(char* pFilename)
{
	char *filecontents = NULL;

	// open the file for reading
	int fd = open(pFilename, O_RDONLY);
	if (fd >= 0)
	{
		// determine the file length
		lseek(fd, 0, SEEK_SET);
		off_t filelen = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
		{
			// allocate a buffer big enough for the file + 1 (null terminator)
			char filebuffer[filelen+1];
			// read the file into the space
			if (read(fd, filebuffer, filelen) == filelen)
			{
				// null terminate
				filebuffer[filelen] = 0;
				// remove white space from the front and back
				StripWS(filebuffer);
				filecontents = filebuffer;
				// print to stdout
				printf("%s\n", filebuffer);
			}
			else
			{
				//printf("Could not read all of the file %s\n", pFilename);
				//printf("%s\n",strerror(errno));
			}
		}
		close(fd);
	}
	else
	{
		//printf("Could not open file %s for reading\n", pFilename);
		//printf("%s\n", strerror(errno));
		exit(1);
	}

	return filecontents;
}

// write a value to the file
void WriteFile(char* pFilename, char* pFileval)
{
	// open the file for writing
	int fd = open(pFilename, O_TRUNC | O_WRONLY);
	if (fd >= 0)
	{
		int strLen = strlen(pFileval);

		if (write(fd, pFileval, strLen) == strLen)
		{
			printf("Succeeded to write \"%s\" into the file %s\n", pFileval, pFilename);
		}
		else
		{
			printf("Could not write all of \"%s\" into the file %s\n", pFileval, pFilename);
			printf("%s\n",strerror(errno));
		}
		close(fd);
	}
	else
	{
		//printf("Could not open file %s for writing\n", pFilename);
		//printf("%s\n",strerror(errno));
		exit(1);
	}

}

void FreeTable(Table* pTable)
{
	pthread_mutex_lock(&pTable->mutex);

	// kill the notification stuff
	FreeNotify(pTable);
	// free the table entries
	FreeTableEntryRecursive(pTable->pTableEntries);

	pthread_mutex_unlock(&pTable->mutex);
	pthread_mutex_destroy(&pTable->mutex);

	free(pTable);
}

Table* CreateTable(void)
{
	Table* pTable = (Table*)malloc(sizeof(Table));
	if (pTable)
	{
		memset(pTable, 0, sizeof(Table));
		TYPE(pTable) = TYPE_TABLE;
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		// allow the same thread to lock/unlock the mutex multiple times
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&pTable->mutex, &attr);
		pthread_mutexattr_destroy(&attr);
		InitNotify(pTable);
	}
	return pTable;
}

Table* config_CreateTable(void)
{
	// set the umask to allow us to do anything to files
	umask(0);
	// return the created table structure
	return CreateTable();
}

// pass a pointer to a table structure
// frees all the structures and data
void config_FreeTable(Table* pTable)
{
	if (pTable && ISTABLE(pTable))
	{
		FreeTable(pTable);
	}
}

