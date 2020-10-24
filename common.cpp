#include "LibInterface.h"
#include "i2c-dev.h"

#define MAX_PATH_LEN	(255)
#define FILE_GPIO_EXPORT "/sys/class/gpio/export"

char *EVENT2STR [] ={
	/* Gpio keys related Events */
	"/dev/input/by-path/gpio-keys-event^1^257",		//EVENT_BUTTON_COMMAND_BUTTON,
	"/dev/input/by-path/gpio-keys-event^1^258",		//EVENT_BUTTON_VOLUME_PLUS,,
	"/dev/input/by-path/gpio-keys-event^1^259",		//EVENT_BUTTON_VOLUME_MINUS,
	"/dev/input/by-path/gpio-keys-event^1^260",		//EVENT_BUTTON_PTT,
	"/dev/input/by-path/gpio-keys-event^1^261",		//EVENT_BUTTON_AUDIO_HEADSET,

	/* Audio Jack Detect Event */
	"/dev/input/by-path/audio-event^5^2",			//EVENT_AUDIO_JACK_DETECT,

	/* Battery Charger related Events */
	"/dev/input/by-path/charger-event^4^3",			//EVENT_CHARGER_SRC_CONNECT,
	"/dev/input/by-path/charger-event^4^2",			//EVENT_CHARGE_STATUS,
	"/dev/input/by-path/charger-event^4^4",			//EVENT_CHARGE_POWERGOOD,
	"/dev/input/by-path/charger-event^4^1",			//EVENT_CHARGE_FAULT,
	"/dev/input/by-path/charger-event^4^0",			//EVENT_BATTERY_FAULT,
	"/dev/input/by-path/charger-event^4^5",			//EVENT_NTC_FAULT,

	"",//EVENT_GAUGE_LOW_SOC,						//dummy

	/* Fuel Gauge Related Events */
	"/dev/input/by-path/gauge-event^4^0",			//EVENT_BATTERY_CONNECT,
	"/dev/input/by-path/gauge-event^4^2",			//EVENT_GAUGE_HIGH_VOLTAGE,
	"/dev/input/by-path/gauge-event^4^3",			//EVENT_GAUGE_LOW_VOLTAGE,
	"/dev/input/by-path/gauge-event^4^4",			//EVENT_GAUGE_SOC_DELTA,
	"/dev/input/by-path/gauge-event^4^5",                   //EVENT_GAUGE_BATTERY_HEALTH,


	"",//EVENT_9AXIS_SNR_ANY_MOTION_INT,			//dummy

	/* External RTC Event */
	"/dev/input/by-path/rtc-event^4^5",				//EVENT_RTC_INT,

	/* PAC1934 Event */
	"/dev/input/by-path/pac-event^4^4",				//EVENT_PAC_THRESHOLD,

	/* Internal ADC Event */
	"/dev/input/by-path/adc-event^4^3",				//EVENT_INT_ADC_THRESHOLD,
};

E_JATIN_ERROR_CODES isFileExist(char *fname)
{
	E_JATIN_ERROR_CODES status = E_JATIN_FAILURE;
	if( access( fname, F_OK ) != -1 ) {
		status = E_JATIN_SUCCESS;
	}

	return status;
}

E_JATIN_ERROR_CODES ReadRegString(char *pVarName, char *pValue, int len) {
    FILE *fp;
    char sTempVar[MAX_PATH_LEN];
    memset(sTempVar, 0x0, MAX_PATH_LEN);
    fp = fopen(pVarName, "rb");
    if (fp) {

        fscanf(fp, "%256[^\n]", sTempVar);
		snprintf(pValue, len, "%s", sTempVar);
        fclose(fp);
    } else {
		printf("Error in reading %s [%s]", pVarName, strerror(errno));
		return E_JATIN_FAILURE;
	}

    return E_JATIN_SUCCESS;
}

void ReadRegStringWithSpace(char *pVarName, char *pValue)
{
	FILE *fp;
	memset(pValue, 0, MAX_PATH_LEN);
	fp = fopen(pVarName, "rb");
	if (fp)
	{
		fscanf(fp, "%255[^\n]", pValue);
		fclose(fp);
	}

	return;
}


E_JATIN_ERROR_CODES ReadRegFloat(char *pVarName, float *pfNum)
{
    FILE *fp;
	float value = 0;

    fp = fopen(pVarName, "rb");
    if (fp) {
        fscanf(fp, "%f", &value);
		*pfNum = value;
        //printf("Float value: %f, %s\n", *pfNum, pVarName);
        fclose(fp);
    } else {
		printf("Error in reading %s [%s].", pVarName, strerror(errno));
		return E_JATIN_FAILURE;
	}

    return E_JATIN_SUCCESS;
}


/**
** Function that reads an intergral value from a registry file
**
** @param[in] pVarName Name of registry file
** @param[out] piNum value
** @return nothing
**/

E_JATIN_ERROR_CODES ReadRegInt(char *pVarName, int *piNum)
{
    FILE *fp;
	int value = 0;

    fp = fopen(pVarName, "rb");
    if (fp) {
        fscanf(fp, "%d", &value);
		*piNum = value;
        //printf("Integer value: %d, file : %s\n", *piNum, pVarName);
        fclose(fp);
    } else {
		printf("Error in reading %s [%s].", pVarName, strerror(errno));
		return E_JATIN_FAILURE;
	}

    return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES ReadRegHex(char *pVarName, int *piNum)
{
    FILE *fp;
    E_JATIN_ERROR_CODES iStatus = E_JATIN_SUCCESS;
    fp = fopen(pVarName, "rb");
    if (fp)
    {
        fscanf(fp, "%x", piNum);
        printf("Hex value: %d\n", *piNum);
        fclose(fp);
    }
    else
    {
		printf("Error in reading %s [%s].", pVarName, strerror(errno));
        iStatus = E_JATIN_FAILURE;
    }

    return iStatus;
}

E_JATIN_ERROR_CODES WriteRegInt(char *pVarName, int pValue)
{
	FILE *fp;
	fp = fopen(pVarName, "wb");
	if (fp) {
		if(fprintf(fp, "%d", pValue)) {
			//printf("written %d to %s\n", pValue, pVarName);
		} else {
			printf("%s: Unable to write [%s]\n", __func__, strerror(errno));
		}

		fclose(fp);
	} else {
		printf("Unable to open %s [%s]\n", pVarName, strerror(errno));
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES WriteRegString(char *pVarName, char *pValue)
{
	FILE *fp;

	if(!pValue || !pVarName) {
		if(pValue)
			//printf("%s: pVarName Null argument. pValue:%s\n", __func__, pValue);
		if(pVarName)
			//printf("%s: pValue Null argument. pVarName:%s\n", __func__, pVarName);
		return E_JATIN_FAILURE;
	}

	fp = fopen(pVarName, "wb");
	if (fp) {
		if (fprintf(fp, "%s", pValue)) {
			//printf("Written %s to %s\n",pValue, pVarName);
		} else {
			printf("%s: Unable to write [%s]\n", __func__, strerror(errno));
		}
		fclose(fp);
	} else {
		printf("Unable to open %s [%s]\n", pVarName, strerror(errno));
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES WriteRegFloat(char *pVarName, float pValue)
{
	FILE *fp;
	fp = fopen(pVarName, "wb");
	if (fp) {
		fprintf(fp, "%f", pValue);
		fclose(fp);
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES CreateDirectory(char *path)
{
	E_JATIN_ERROR_CODES iRetVal = E_JATIN_SUCCESS;
	int iStatus                 = 0;
	char strPath[MAX_PATH_LEN]  = {0};
	char strRegVar[MAX_PATH_LEN]= {0};
	char *pStr                  = NULL;
	char *saveptr               = NULL;
	struct stat stat_buf;

	if (path == NULL) {
		printf("Invalid arguments\n");
		return iRetVal;
	}

	iStatus = stat(path, &stat_buf);
	if (iStatus == 0) {
		printf("Directory <%s> already present", path);
		return E_JATIN_SUCCESS;
	}

	snprintf(strPath, MAX_PATH_LEN, "%s", path);
	memset(strRegVar, 0x0, sizeof(strRegVar));

	pStr = strtok_r(strPath, "/", &saveptr);
	while (pStr != NULL) {
		strcat(strRegVar, "/");
		strcat(strRegVar, pStr);
		printf("Creating directory <%s>", strRegVar);
		/* Check if file/directory already exists */
		iStatus = stat(strRegVar, &stat_buf);
		if (iStatus == 0) {
			printf("Directory <%s> already present", strRegVar);
			pStr = strtok_r(NULL, "/", &saveptr);
			continue;
		}

		iStatus = mkdir(strRegVar, 0755);
		if (iStatus != 0 && errno != EEXIST) {
			printf("Failed to create directory <%s :: %s>, %s", path, strRegVar, strerror(errno));
			iRetVal = E_JATIN_FAILURE;
			break;
		}

		pStr = strtok_r(NULL, "/", &saveptr);
	}

	if (iRetVal != E_JATIN_FAILURE) {
		if (pStr == NULL) {
			iRetVal = E_JATIN_SUCCESS;
		}
	}

	return iRetVal;
}

E_JATIN_ERROR_CODES GetGpioValue(int gpioNo, char *gpioDirection, int *value)
{
	int port = 0, number = 0, ret = -1;
	char gpioStr[5];
	char FileStr[40];
	char direction[10];

	memset(direction, 0, sizeof(direction));
	memset(gpioStr, 0, sizeof(gpioStr));
	memset(FileStr, 0, sizeof(FileStr));

	port = gpioNo/32;
	number = gpioNo%32;

	switch(port) {
		case 0:
			sprintf(gpioStr, "PA%d",number);
		break;
		case 1:
			sprintf(gpioStr, "PB%d",number);
		break;
		case 2:
			sprintf(gpioStr, "PC%d",number);
		break;
		case 3:
			sprintf(gpioStr, "PD%d",number);
		break;
		case 4:
			sprintf(gpioStr, "PE%d",number);
		break;
		default:
			printf("Invalid GPIO number = %d\n",gpioNo);
			return E_JATIN_NOT_SUPPORTED;
	}

	sprintf(FileStr,"/sys/class/gpio/%s/direction",gpioStr);

	ret = isFileExist(FileStr);
	if(ret != E_JATIN_SUCCESS)
	{
		//printf("%s file doesn't exist exporting gpio %d\n", FileStr, gpioNo);
		/* export gpio */
		WriteRegInt(FILE_GPIO_EXPORT, gpioNo);
	}

	ReadRegString(FileStr, direction, sizeof(direction));
	if(strcmp(direction, gpioDirection) )
	{
	//	printf("Setting direction out\n");
		WriteRegString(FileStr, gpioDirection);
	}

	memset(FileStr, 0, sizeof(FileStr));
	sprintf(FileStr,"/sys/class/gpio/%s/value",gpioStr);
	ret = ReadRegInt(FileStr, value);
	if(ret != E_JATIN_SUCCESS)
	{
		printf("WriteRegInt failed for %s\n", FileStr);
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;

}

E_JATIN_ERROR_CODES SetGpioValue(int gpioNo, int value)
{
	int port = 0, number = 0;
	E_JATIN_ERROR_CODES ret = E_JATIN_FAILURE;
	char gpioStr[5];
	char FileStr[40];
	char direction[10];

	memset(direction, 0, sizeof(direction));
	memset(gpioStr, 0, sizeof(gpioStr));
	memset(FileStr, 0, sizeof(FileStr));

	port = gpioNo/32;
	number = gpioNo%32;

	switch(port) {
		case 0:
			sprintf(gpioStr, "PA%d",number);
		break;
		case 1:
			sprintf(gpioStr, "PB%d",number);
		break;
		case 2:
			sprintf(gpioStr, "PC%d",number);
		break;
		case 3:
			sprintf(gpioStr, "PD%d",number);
		break;
		case 4:
			sprintf(gpioStr, "PE%d",number);
		break;
		default:
			printf("Invalid GPIO number = %d\n",gpioNo);
			return E_JATIN_NOT_SUPPORTED;
	}

	sprintf(FileStr,"/sys/class/gpio/%s/direction",gpioStr);

	ret = isFileExist(FileStr);
	if(ret != E_JATIN_SUCCESS)
	{
		//printf("%s file doesn't exist exporting gpio %d\n", FileStr, gpioNo);
		/* export gpio */
		WriteRegInt(FILE_GPIO_EXPORT, gpioNo);
	}

	ReadRegString(FileStr, direction, sizeof(direction));
	if(strcmp(direction, "out"))
	{
	//	printf("Setting direction out\n");
		WriteRegString(FileStr, "out");
	}

	memset(FileStr, 0, sizeof(FileStr));
	sprintf(FileStr,"/sys/class/gpio/%s/value",gpioStr);
	ret = WriteRegInt(FileStr, value);
	if(ret != E_JATIN_SUCCESS)
	{
		printf("WriteRegInt failed for %s\n", FileStr);
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

int I2C_Read_Byte(char *I2cFileName, int SlaveAddr, char reg)
{
	int I2C_fd = -1, ret = -1;

	I2C_fd = open(I2cFileName, O_RDWR);
	if(I2C_fd < 0)
	{
		printf("%s cannot open file [%s]\n",__func__,strerror(errno));
		return ret;
	}
//	printf("%s opened successfully\n", I2cFileName);

	ioctl(I2C_fd, I2C_SLAVE_FORCE, SlaveAddr);
//	printf("slave address set @ 0x%x\n", SlaveAddr);

	ret = i2c_smbus_read_byte_data(I2C_fd, reg);

	close(I2C_fd);
	return ret;
}

int I2C_Write_Refresh(char *I2cFileName, int SlaveAddr, char value)
{
	int I2C_fd = -1, ret = -1;

	I2C_fd = open(I2cFileName, O_RDWR);
	if(I2C_fd < 0)
	{
		printf("%s cannot open file [%s]\n",__func__,strerror(errno));
		return ret;
	}
//	printf("%s opened successfully\n", I2cFileName);

	ioctl(I2C_fd, I2C_SLAVE_FORCE, SlaveAddr);
//	printf("slave address set @ 0x%x\n", SlaveAddr);

	ret = i2c_smbus_write_byte(I2C_fd, value);

	close(I2C_fd);
	return ret;
}

int I2C_Write_Byte(char *I2cFileName, int SlaveAddr, char reg ,char value)
{
	int I2C_fd = -1, ret = -1;

	I2C_fd = open(I2cFileName, O_RDWR);
	if(I2C_fd < 0)
	{
		printf("%s cannot open file [%s]\n",__func__,strerror(errno));
		return ret;
	}
//	printf("%s opened successfully\n", I2cFileName);

	ioctl(I2C_fd, I2C_SLAVE_FORCE, SlaveAddr);
//	printf("slave address set @ 0x%x\n", SlaveAddr);

	ret = i2c_smbus_write_byte_data(I2C_fd, reg, value);

	close(I2C_fd);
	return ret;
}

int I2C_Read_Word(char *I2cFileName, int SlaveAddr, char command){

	int I2C_fd = -1, ret = -1;

	I2C_fd = open(I2cFileName, O_RDWR);
	if(I2C_fd < 0)
	{
		printf("%s cannot open file [%s]\n",__func__,strerror(errno));
		return ret;
	}
//	printf("%s opened successfully\n", I2cFileName);

	ioctl(I2C_fd, I2C_SLAVE_FORCE, SlaveAddr);
//	printf("slave address set @ 0x%x\n", SlaveAddr);

	ret = i2c_smbus_read_word_data(I2C_fd, command);

	close(I2C_fd);
	return ret;
}
#if 0
int ExecuteSytemCommand1(char *data){

	FILE *fp;
	char value[20];
	memset(value, 0, sizeof(value));

	fp = popen(data, "r");
	if (fp == NULL) {
		printf("Failed to run command\n");
		return E_JATIN_FAILURE;
	}

	/* Read the output a line at a time - output it. */
	if (fgets(value, sizeof(value)-1, fp) == NULL)
	{
		pclose(fp);
		return E_JATIN_FAILURE;
	}

	/* close */
	pclose(fp);
	return E_JATIN_SUCCESS;
}
#endif
E_JATIN_ERROR_CODES ExecuteSytemCommand(char *data, char *ret_value){

	FILE *fp;
	char value[20];

	fp = popen(data, "r");
	if (fp == NULL) {
		printf("%s Failed to run command [%s] \n",__func__, strerror(errno));
		return E_JATIN_FAILURE;
	}

	/* Read the output a line at a time - output it. */
	if (fgets(value, sizeof(value)-1, fp) == NULL)
	{
		pclose(fp);
		return E_JATIN_FAILURE;
	}

	if (ret_value) {
		strcpy(ret_value, value);
	}

	/* close */
	pclose(fp);
	return E_JATIN_SUCCESS;
}
