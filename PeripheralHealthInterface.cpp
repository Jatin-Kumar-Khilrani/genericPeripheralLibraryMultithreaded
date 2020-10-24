#include "LibInterface.h"
#include <math.h>

#define STATUS_LEN 20

#define FILE_GAUGE_CAPACITY "/sys/class/power_supply/bms/capacity"
#define FILE_CHARGER_STATUS "/sys/class/power_supply/bq25601-battery/status"
#define BATTERY_CURRENT_PATH "/sys/class/power_supply/bms/current_now"

#define GPIO_EXT_ADC 88
#define GPIO_AUDIO_CODEC 62
#define GPIO_WIFI_SUPPLY 13
#define GPIO_WIFI_REGULATOR 12
#define GPIO_BT_REGULATOR 11
#define GPIO_BATT_CHARGER 79

#define BIT(nr)                 (1UL << (nr))
#define BQ25601D_REG_STATUS_CHARGING	(BIT(4) | BIT(3))

/* Get the value of bitfield */
#define BF_GET(_y, _mask) (((_y) & _mask) >> (__builtin_ffs((int) _mask) - 1))

E_JATIN_ERROR_CODES DetectDevice (char *data)
{
	char DeviceDetect[STATUS_LEN];

	memset(DeviceDetect, 0, sizeof(DeviceDetect));

	//Detect battery charger over i2c
	if(ExecuteSytemCommand(data, DeviceDetect) == E_JATIN_SUCCESS){
		if(strncmp(DeviceDetect, "UU",2)){
			INFO_ERROR("Unable to detect device over i2c\n");
			return E_JATIN_FAILURE;
		}
	} else {
			return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetPMICHealthStatus (E_PERI_HEALTH *health)
{
	*health = E_HEALTH_GOOD;
	float voltage_out1 = 0, voltage_out2 = 0, voltage_out3 = 0, voltage_out5 = 0;

	//Detect device over i2c
	if(DetectDevice("i2cdetect -y 0 | tail -n 3 | head -n 1 | awk '{print$13}'") != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Enable external ADC
	if(SetGpioValue(GPIO_EXT_ADC, 1) != E_JATIN_SUCCESS)
	{
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	// Read out1 voltage
	if(GetInternalAdcData(E_INTERNAL_ADC_CHANNEL_1V35_SENSE, &voltage_out1) != E_JATIN_SUCCESS)
	{
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	//typical voltage is 1350 mV
	if((voltage_out1 < 1275.00) || (voltage_out1 > 1400.00)){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Read out2 voltage
	if(GetExternalAdcData(E_EXTERNAL_ADC_VOLTAGE_CHANNEL_1V2, &voltage_out2) != E_JATIN_SUCCESS)
	{
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	//typical voltage is 1222 mV
	if((voltage_out2 < 1150) || (voltage_out2 > 1250)){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Read out3 voltage
	if(GetExternalAdcData(E_EXTERNAL_ADC_VOLTAGE_CHANNEL_1V8_3V3, &voltage_out3) != E_JATIN_SUCCESS)
	{
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	if((voltage_out3 < 2000) || (voltage_out3 > 2600))
	{
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Read out5 voltage
	if(GetExternalAdcData(E_EXTERNAL_ADC_VOLTAGE_CHANNEL_3V3, &voltage_out5) != E_JATIN_SUCCESS)
	{
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	if((voltage_out5 < 2700) || (voltage_out5 > 3300))
	{
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetBatteryChargerHealthStatus (E_PERI_HEALTH *health)
{
	*health = E_HEALTH_GOOD;
	int current = 0;
	E_BATTERY_CONNECT_STATUS batt_connect = E_BAT_CONN_NOTCONNECTED;
	E_CHARGER_SOURCE charge_source = POWER_SUPPLY_TYPE_UNKNOWN;
	int reg_value, bit_val;

	//Detect device over i2c
	if(DetectDevice("i2cdetect -y 4 | tail -n 2 | head -n 1 | awk '{print$13}'") != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//make sure battery is connected.
	if(GetBatteryConnectStatus(&batt_connect) != E_JATIN_SUCCESS){
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	if(GetBatteryChargerSource(&charge_source) != E_JATIN_SUCCESS)
	{
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	if(charge_source == POWER_SUPPLY_TYPE_NO_INPUT){
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	//Enable charging.
	if(SetGpioValue(GPIO_BATT_CHARGER, 0) != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	sleep(3);

	//Read battery charging status
	if(GetI2CDevRegisterValue(E_PERI_BATTERY_CHARGER, 0x08, &reg_value) != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}
	bit_val = BF_GET(reg_value, BQ25601D_REG_STATUS_CHARGING);

	if(GetBatteryCurrent(&current) != E_JATIN_SUCCESS){
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	//If charging.
	if(bit_val == 2){
		if(current <= 0)
			*health = E_HEALTH_BAD;
	//If already full.
	} else if(bit_val == 3){
		if(current != 0)
			*health =E_HEALTH_BAD;
	} else {
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	sleep(0.5);
	//Disable charging
	if(SetGpioValue(GPIO_BATT_CHARGER, 1) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Unable to disable charger\n");
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetFuelGaugeHealthStatus (E_PERI_HEALTH *health)
{
	*health = E_HEALTH_GOOD;
	int capacity = 0;
	E_BATTERY_CONNECT_STATUS batt_connect = E_BAT_CONN_NOTCONNECTED;

	//Detect device over i2c
	if(DetectDevice("i2cdetect -y 4 | tail -n 3 | head -n 1 | awk ' {print $7} '") != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//make sure battery is connected.
	if(GetBatteryConnectStatus(&batt_connect) != E_JATIN_SUCCESS){
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	//Read battery capacity
	if(GetBatteryCapacity(&capacity) != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	if(capacity < 0 || capacity > 100)
		*health = E_HEALTH_BAD;

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetAudioCodecHealthStatus (E_PERI_HEALTH *health)
{
	int reg_value = 0;
	float voltage = 0;
	*health = E_HEALTH_GOOD;

	if(GetExternalAdcData(E_EXTERNAL_ADC_VOLTAGE_CHANNEL_1V8_3V3, &voltage) != E_JATIN_SUCCESS)
	{
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	// typical voltage will be 2300mV
	if((voltage < 2000) || (voltage > 2600))
	{
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Detect device over i2c
	if(DetectDevice("i2cdetect -y 2 | head -n 3 | tail -n 1 | awk '{print $10}'") != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Read codec value over I2C
	if(GetI2CDevRegisterValue(E_PERI_AUDIO_CODEC, 0X41, &reg_value) != E_JATIN_SUCCESS)
	{
		*health = E_HEALTH_BAD;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES Get9AxisSensorHealthStatus (E_PERI_HEALTH *health)
{
	int reg_value;
	*health = E_HEALTH_GOOD;

	//Detect device over i2c
	if(DetectDevice("i2cdetect -y 1 | tail -n 2 | head -n 1 | awk ' {print $10} '") != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Read data over i2c
	if(GetI2CDevRegisterValue(E_PERI_9AXIS_SENSOR, 0x04, &reg_value) != E_JATIN_SUCCESS)
		*health = E_HEALTH_BAD;

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetInternalAdcHealthStatus (E_PERI_HEALTH *health)
{
	float voltage = 0;
	float adc_volt_scale = 0, typical_scale_voltage = 0.7324;
	*health = E_HEALTH_GOOD;

	//Read out5 voltages
	if(GetExternalAdcData(E_EXTERNAL_ADC_VOLTAGE_CHANNEL_3V3, &voltage) != E_JATIN_SUCCESS)
	{
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	if((voltage < 2900) || (voltage > 3100))
	{
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Read Internal ADC voltage scale (typical value is 0.732422)
	if(GetAdcVoltageScale(&adc_volt_scale)){
			*health = E_HEALTH_UNKNOWN;
			return E_JATIN_FAILURE;
	}

	adc_volt_scale = floor(adc_volt_scale*10000)/10000;
	if(adc_volt_scale != typical_scale_voltage) {
		*health = E_HEALTH_BAD;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetLEDHealthStatus(E_PERI_HEALTH *health)
{
	float voltage_value = 0.0;
	*health = E_HEALTH_GOOD;

	//Set-Blue-Led
	if(SetLedColor(E_LED_COLOR_BLUE, 255) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to set Led Color Blue\n");
		*health = E_HEALTH_BAD;
		return E_JATIN_FAILURE;
	}
	/* Here delay is required before reading ADC channel because after turning on LED,
	   voltage will take few milliseconds of time to decrease from its original value
	   i.e 3.0V. So we have kept 5ms delay
	*/
	usleep(5000);
	//Read ADC_LED_FAULT channel from ADC
	if(GetInternalAdcData(E_INTERNAL_ADC_CHANNEL_LED_FAULT, &voltage_value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read Internal ADC data for channel %d\n", E_INTERNAL_ADC_CHANNEL_LED_FAULT);
		return E_JATIN_FAILURE;
	}
	if(voltage_value > 2980.0)
	{
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Set-Green-Led
	if(SetLedColor(E_LED_COLOR_GREEN, 255) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to set Led Color Green\n");
		*health = E_HEALTH_BAD;
		return E_JATIN_FAILURE;
	}
	usleep(5000);
	//Read ADC_LED_FAULT channel from ADC
	if(GetInternalAdcData(E_INTERNAL_ADC_CHANNEL_LED_FAULT, &voltage_value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read Internal ADC data for channel %d\n", E_INTERNAL_ADC_CHANNEL_LED_FAULT);
		return E_JATIN_FAILURE;
	}
	if(voltage_value > 2980.0)
	{
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Set-Red-Led
	if(SetLedColor(E_LED_COLOR_RED, 255) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to set Led Color Red\n");
		*health = E_HEALTH_BAD;
		return E_JATIN_FAILURE;
	}
	usleep(5000);
	//Read ADC_LED_FAULT channel from ADC
	if(GetInternalAdcData(E_INTERNAL_ADC_CHANNEL_LED_FAULT, &voltage_value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read Internal ADC data for channel %d\n", E_INTERNAL_ADC_CHANNEL_LED_FAULT);
		return E_JATIN_FAILURE;
	}
	if(voltage_value > 2980.0)
	{
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Turn off LED
	TurnOffLed();

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetLEDControllerHealthStatus (E_PERI_HEALTH *health)
{
	*health = E_HEALTH_GOOD;

	//Detect device over i2c
	if(DetectDevice("i2cdetect -y 1 | head -n 5 | tail -n 1 | awk ' {print$2} '") != E_JATIN_SUCCESS)
		*health = E_HEALTH_BAD;

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetExternalAdcHealthStatus (E_PERI_HEALTH *health)
{
	*health = E_HEALTH_GOOD;

	if(SetGpioValue(GPIO_EXT_ADC, 1) != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Detect device over i2c
	if(DetectDevice("i2cdetect -y 4 | tail -n 7 | head -n 1 | awk ' {print $2} '") != E_JATIN_SUCCESS)
		*health = E_HEALTH_BAD;

	return E_JATIN_SUCCESS;

}

E_JATIN_ERROR_CODES GetExternalRTCHealthStatus (E_PERI_HEALTH *health)
{
	*health = E_HEALTH_GOOD;

	//Detect device over i2c
	if(DetectDevice("i2cdetect -y 1 | tail -n 3 | head -n 1 | awk ' {print$3} '") != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	//Check if rtc1 exists
	if(isFileExist("/dev/rtc1") != E_JATIN_SUCCESS)
		*health = E_HEALTH_BAD;

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetEEPROMHealthStatus (E_PERI_HEALTH *health)
{
	char writeData[STATUS_LEN], readData[STATUS_LEN];
	*health = E_HEALTH_GOOD;

	//Detect device over i2c
	if(DetectDevice("i2cdetect -y 3 | tail -n 3 | head -n 1 | awk ' {print$2} '") != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	memset(writeData, 0, sizeof(writeData));

	//Disable write protect
	SetWriteProtect(E_EEPROM_WP_DISABLE);
	sprintf(writeData,"12345678");

	if(WriteEeprom(writeData, 8, sizeof(writeData)) != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}
	memset(readData, 0, sizeof(readData));
	if(ReadEeprom(readData, 8, sizeof(readData)) != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	if(strcmp(writeData, readData)) {
		*health = E_HEALTH_BAD;
	}

	//Enable write protect
	SetWriteProtect(E_EEPROM_WP_ENABLE);

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetWIFIHealthStatus(E_PERI_HEALTH *health)
{
	float voltage = 0;
	bool insert_modules = false, is_wifi_up = false;
	char val = 0, retVal = 0;
	*health = E_HEALTH_GOOD;

	//Cannot be accessed, as it is being used internally.
	//check if supply is enabled
	/*if(!GetGpioValue(GPIO_WIFI_SUPPLY, "out", &gpio_value)){
		if(!gpio_value){
			if(SetGpioValue(GPIO_WIFI_SUPPLY, 1) == -1){
				*health = E_HEALTH_BAD;
				return 1;
			}
		}
	} else {
		*health = E_HEALTH_BAD;
		return 1;
	}

	//check if regulator is on
	if(!GetGpioValue(GPIO_WIFI_REGULATOR, "out", &gpio_value)){
		if(!gpio_value){
			if(SetGpioValue(GPIO_WIFI_REGULATOR, 1) == -1){
				*health = E_HEALTH_BAD;
				return 1;
			}
		}
	} else {
		*health = E_HEALTH_BAD;
		return 1;
	}*/

	//check if driver is inserted
	ExecuteSytemCommand("lsmod | grep compat | wc -l", &retVal);
	if(retVal == 48)
		insert_modules = true;
	ExecuteSytemCommand("lsmod | grep cfg80211 | wc -l", &retVal);
	if(retVal == 48)
		insert_modules = true;
	ExecuteSytemCommand("lsmod | grep brcmutil | wc -l", &retVal);
	if(retVal == 48)
		insert_modules = true;
	ExecuteSytemCommand("lsmod | grep brcmfmac | wc -l", &retVal);
	if(retVal == 48)
		insert_modules = true;

	/* Check if wifi is up or not */
	ExecuteSytemCommand("ifconfig | grep wlan0 | wc -l",  &retVal);
	if(retVal == 49)
		is_wifi_up = true;

	/* If drivers are not already loaded, then load the modules */
	if(insert_modules)
	{
		ExecuteSytemCommand("insmod /etc/broadcom/wifi/compat.ko", NULL);
		ExecuteSytemCommand("insmod /etc/broadcom/wifi/cfg80211.ko", NULL);
		ExecuteSytemCommand("insmod /etc/broadcom/wifi/brcmutil.ko", NULL);
		ExecuteSytemCommand("insmod /etc/broadcom/wifi/brcmfmac.ko", NULL);
	}
	sleep(1);
	//Check wifi voltages
	if(GetExternalAdcData(E_EXTERNAL_ADC_VOLTAGE_CHANNEL_3V3_WIFI, &voltage) != E_JATIN_SUCCESS) {
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	//validate voltage range
	if ((voltage < 3250) || (voltage > 3400)) {
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	sleep(2);

	if(!is_wifi_up)
		ExecuteSytemCommand("ifconfig wlan0 up", NULL);

	ExecuteSytemCommand("ifconfig | grep wlan0 | wc -l",  &val);

	if (val != 49) {
		*health = E_HEALTH_BAD;
	}

	sleep(0.2);
	//Disable
	if(!is_wifi_up)
		ExecuteSytemCommand("ifconfig wlan0 down", NULL);

	/* If wifi modules were loaded during execution of this function,
	 * then remove the modules, else leave as it is
	 */
	if(insert_modules)
	{
		ExecuteSytemCommand("rmmod brcmfmac", NULL);
		ExecuteSytemCommand("rmmod brcmutil", NULL);
		ExecuteSytemCommand("rmmod cfg80211", NULL);
		ExecuteSytemCommand("rmmod compat", NULL);
		insert_modules = false;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetBTHealthStatus(E_PERI_HEALTH *health) {

	float voltage = 0;
	*health = E_HEALTH_GOOD;
	char val = 0;

	//Cannot be accessed, as it is being used internally.
	//Supply enable
	/*if(!GetGpioValue(GPIO_WIFI_SUPPLY, "out", &gpio_value)){
		if(!gpio_value){
			if(SetGpioValue(GPIO_WIFI_SUPPLY, 1) == -1){
				*health = E_HEALTH_BAD;
				return 1;
			}
		}
	} else {
		*health = E_HEALTH_BAD;
		return 1;
	}*/

	if(SetGpioValue(GPIO_BT_REGULATOR, 1) != E_JATIN_SUCCESS){
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	ExecuteSytemCommand("hciattach /dev/ttyS1 bcm43xx 3000000 flow -t 20", NULL);

	//Check wifi voltages
	if(GetExternalAdcData(E_EXTERNAL_ADC_VOLTAGE_CHANNEL_3V3_WIFI, &voltage) != E_JATIN_SUCCESS) {
		*health = E_HEALTH_UNKNOWN;
		return E_JATIN_FAILURE;
	}

	if ((voltage < 3250) || (voltage > 3400)) {
		*health = E_HEALTH_BAD;
		return E_JATIN_SUCCESS;
	}

	ExecuteSytemCommand("hciconfig | grep hci0 | wc -l", &val);
	if (val <= 0) {
		*health = E_HEALTH_BAD;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetPeripheralHealthStatus (E_PERI_HEALTH *health, E_PERIPHERAL peripheral)
{
	E_JATIN_ERROR_CODES ret = E_JATIN_FAILURE;

	switch(peripheral)
	{
		case E_PERI_PMIC:
			ret = GetPMICHealthStatus(health);
		break;
		case E_PERI_BATTERY_CHARGER:
			ret = GetBatteryChargerHealthStatus(health);
		break;
		case E_PERI_BATTERY_GAUGE:
			ret = GetFuelGaugeHealthStatus(health);
		break;
		case E_PERI_AUDIO_CODEC:
			ret = GetAudioCodecHealthStatus(health);
		break;
		case E_PERI_9AXIS_SENSOR:
			ret = Get9AxisSensorHealthStatus(health);
		break;
		case E_PERI_INTERNAL_ADC:
			ret = GetInternalAdcHealthStatus(health);
		break;
		case E_PERI_LED:
			ret = GetLEDHealthStatus(health);
		break;
		case E_PERI_LED_CONTROLLER:
			ret = GetLEDControllerHealthStatus(health);
		break;
		case E_PERI_EXTERNAL_ADC:
			ret = GetExternalAdcHealthStatus(health);
		break;
		case E_PERI_EXTERNAL_RTC:
			ret = GetExternalRTCHealthStatus(health);
		break;
		case E_PERI_SERIAL_EEPROM:
			ret = GetEEPROMHealthStatus(health);
		break;
		case E_PERI_WIFI:
			ret = GetWIFIHealthStatus(health);
		break;
		case E_PERI_BT:
			ret = GetBTHealthStatus(health);
		break;
		case E_PERI_ALL:
			return E_JATIN_SUCCESS;
		break;
		case E_PERI_BUTTON:
		case E_PERI_EXTERNAL_WATCHDOG:
			INFO_ERROR("Unable to perform button peripheral health check\n");
			*health = E_HEALTH_UNKNOWN;
			ret = E_JATIN_FAILURE;
		break;
		default:
			INFO_ERROR("Request for unknown peripheral\n");
			return E_JATIN_FAILURE;
	}
 	return ret;
}
