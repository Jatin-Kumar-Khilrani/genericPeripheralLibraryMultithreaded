#include <stdio.h>
#include "LibInterface.h"
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#define ID_NAME_SIZE 20

E_JATIN_ERROR_CODES StartStopwatch(int id)
{
	char IdFileName[ID_NAME_SIZE];
	struct timeval tv;
	int ret = -1;
	FILE *fp = NULL;

	memset(IdFileName,0,sizeof(IdFileName));
	memset(&tv,0,sizeof(struct timeval));

	sprintf(IdFileName,"/tmp/%d",id);

	ret = gettimeofday(&tv, NULL);
	if(ret == -1)
	{
		INFO_ERROR("gettimeofday failed [%s]\n",strerror(errno));
		return E_JATIN_FAILURE;
	}

	fp = fopen(IdFileName,"wb");
	if(fp == NULL)
	{
		INFO_ERROR("fopen failed %s [%s]\n", IdFileName,strerror(errno));
		return E_JATIN_FAILURE;
	}

	ret = fwrite(&tv, 1, sizeof(struct timeval), fp);
	if(ret != sizeof(struct timeval))
	{
		INFO_ERROR("fwrite failed [%s]\n",strerror(errno));
	}

	fclose(fp);

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES DeleteStopwatch(int id)
{
	char IdFileName[ID_NAME_SIZE];
	int ret = -1;

	memset(IdFileName,0,sizeof(IdFileName));
	sprintf(IdFileName,"/tmp/%d",id);

	ret = isFileExist(IdFileName);
	if(ret != E_JATIN_SUCCESS)
	{
		INFO_ERROR("File not exist %s\n",IdFileName);
		return E_JATIN_FAILURE;
	}

	ret = unlink(IdFileName);
	if(ret != 0)
	{
		INFO_ERROR("ID file delete failed %s [%s]\n", IdFileName,strerror(errno));
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetElapsedTime(int id, struct timeval *tv)
{
	char IdFileName[ID_NAME_SIZE];
	struct timeval StartTv;
	int ret = -1;
	FILE *fp = NULL;

	memset(IdFileName,0,sizeof(IdFileName));
	memset(&StartTv,0,sizeof(struct timeval));

	sprintf(IdFileName,"/tmp/%d",id);

	ret = gettimeofday(tv, NULL);
	if(ret == -1)
	{
		INFO_ERROR("gettimeofday failed [%s]\n",strerror(errno));
		return E_JATIN_FAILURE;
	}

	fp = fopen(IdFileName,"rb");
	if(fp == NULL)
	{
		INFO_ERROR("fopen failed %s [%s]\n", IdFileName,strerror(errno));
		return E_JATIN_FAILURE;
	}

	ret = fread(&StartTv, 1, sizeof(struct timeval), fp);
	if(ret != sizeof(struct timeval))
	{
		INFO_ERROR("fread failed [%s]\n",strerror(errno));
	}

	fclose(fp);

	tv->tv_sec -= StartTv.tv_sec;
	tv->tv_usec -= StartTv.tv_usec;

	return E_JATIN_SUCCESS;
}
