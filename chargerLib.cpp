#include "LibInterface.h"

#define FILE_CHARGER_STATUS "/sys/class/power_supply/bq25601-battery/status"
#define CHARGE_STATUS_LEN 20
#define FILE_CHARGE_CURRENT "/sys/class/power_supply/bq25601-charger/constant_charge_current"
#define FILE_CHARGE_TYPE "/sys/class/power_supply/bq25601-charger/type"
#define FILE_CHARGE_STATE "/sys/class/power_supply/bq25601-battery/toggle_charging"

E_JATIN_ERROR_CODES GetBatteryChargeStatus (E_CHARGING_STATE *state)
{
	char chgStatus[CHARGE_STATUS_LEN];

	memset(chgStatus, 0, sizeof(chgStatus));

	if(ReadRegString(FILE_CHARGER_STATUS, chgStatus, sizeof(chgStatus)) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s", FILE_CHARGER_STATUS);
		return E_JATIN_FAILURE;
	}

	if(!strcmp(chgStatus, "Unknown"))
	{
		*state = E_CHARGING_STATE_UNKNOWN;
	} else if (!strcmp(chgStatus, "Charging")) {
		*state = E_CHARGING_STATE_CHARGING;
	} else if (!strcmp(chgStatus, "Discharging")) {
		*state = E_CHARGING_STATE_DISCHARGING;
	} else if (!strcmp(chgStatus, "Not charging")) {
		*state = E_CHARGING_STATE_NOTCHARGING;
	} else if (!strcmp(chgStatus, "Full")) {
		*state = E_CHARGING_STATE_FULLYCHARGED;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetBatteryChargingCurrent(int *current)
{
	if(ReadRegInt(FILE_CHARGE_CURRENT, current) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s\n", FILE_CHARGE_CURRENT);
		*current = 0;
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetBatteryChargingCurrent(int current)
{
	if(WriteRegInt(FILE_CHARGE_CURRENT, current) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s", FILE_CHARGE_CURRENT);
		return E_JATIN_FAILURE;
	}
	INFO_DEBUG("Battery charging current is set to %d\n", current);
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetBatteryChargerState (E_CHARGER_STATE enable)
{
	E_BATTERY_TEMPERATURE_REGION battHealth;

	if((enable != E_CHARGER_ENABLE) && (enable != E_CHARGER_DISABLE))
	{
		INFO_ERROR("Invalid value of enable %d\n", enable);
		return E_JATIN_NOT_SUPPORTED;
	}

	//Check if battery health is read properly
	if((enable == E_CHARGER_ENABLE) &&  (GetBatteryHealth(&battHealth) == E_JATIN_SUCCESS))
	{
		//Check if battery is in Overheat/Cold condition
		if(battHealth == E_BATTERY_OVERHEAT)
		{
			INFO_ERROR("Unable to start charging, battery in thermal regulation mode \n");
			return E_JATIN_BATTERY_THERMAL_REGULATION;
		}
	}

	if(WriteRegInt(FILE_CHARGE_STATE, enable) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s, enable=%d\n", FILE_CHARGE_STATE, enable);
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetBatteryChargerSource (E_CHARGER_SOURCE *src)
{
	char chgType[CHARGE_STATUS_LEN];

	memset(chgType, 0, sizeof(chgType));

	if(ReadRegString(FILE_CHARGE_TYPE, chgType, sizeof(chgType)) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read %s", FILE_CHARGE_TYPE);
		return E_JATIN_FAILURE;
	}

	if(!strcmp(chgType, "Unknown"))
	{
		*src = POWER_SUPPLY_TYPE_UNKNOWN;
	} else if (!strcmp(chgType, "USB")) {
		*src = POWER_SUPPLY_TYPE_USB;
	} else if (!strcmp(chgType, "USB_DCP")) {
		*src = POWER_SUPPLY_TYPE_USB_DCP;
	} else if (!strcmp(chgType, "USB_CDP")) {
		*src = POWER_SUPPLY_TYPE_USB_CDP;
	} else if (!strcmp(chgType, "No_Input")) {
		*src = POWER_SUPPLY_TYPE_NO_INPUT;
	} else if (!strcmp(chgType, "Non_Standard")) {
		*src = POWER_SUPPLY_TYPE_NON_STANDARD;
	} else if (!strcmp(chgType, "OTG")) {
		*src = POWER_SUPPLY_TYPE_OTG;
	} else if (!strcmp(chgType, "Mains")) {
		*src = POWER_SUPPLY_TYPE_MAINS;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetBatteryConnectStatus (E_BATTERY_CONNECT_STATUS *connect)
{
	if(I2C_Read_Word("/dev/i2c-4",  0x55, 0x2C) < 0 )
	{
		INFO_ERROR("Failed in reading fuel gauge register\n");
		*connect = E_BAT_CONN_NOTCONNECTED;
		return E_JATIN_FAILURE;
	}

	*connect = E_BAT_CONN_CONNECTED;
	return E_JATIN_SUCCESS;
}
