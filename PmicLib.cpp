#include <stdio.h>
#include "LibInterface.h"

char PMICREGISTER[] = {
	0x22,
	0x32,
	0x42,
	0x51,
	0x55,
	0x61,
	0x65
};

E_JATIN_ERROR_CODES SetRegulatorStatus(E_PMIC_REG regulator, E_PMIC_REG_STATE status)
{
	int value = 0;

	if(regulator < E_PMIC_REG1 || regulator > E_PMIC_REG7)
	{
		INFO_ERROR("Invalid regulator number\n");
		return E_JATIN_PARAM_ERR;
	}

	if(GetI2CDevRegisterValue(E_PERI_PMIC, PMICREGISTER[regulator], &value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read I2c register\n");
		return E_JATIN_FAILURE;
	}

	if(status == E_PMIC_REG_ENABLE) {
		value |= (1 << 7); /* set 7th bit */
	} else if (status == E_PMIC_REG_DISABLE) {
		value &= ~(1 << 7); /* clear 7th bit */
	} else {
		INFO_ERROR("Invalid status to set\n");
		return E_JATIN_PARAM_ERR;
	}

	if(SetI2CDevRegisterValue(E_PERI_PMIC, PMICREGISTER[regulator], value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to set I2c register\n");
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetPmicShutdownVoltage(E_SYSLEV_THRESHOLD voltage)
{
	int value = 0;

	if(voltage < E_SYSLEV_THR_2300 || voltage > E_SYSLEV_THR_3800)
	{
		INFO_ERROR("Invalid mode to set\n");
		return E_JATIN_PARAM_ERR;
	}

	if(GetI2CDevRegisterValue(E_PERI_PMIC, 0x00, &value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read I2c register\n");
		return E_JATIN_FAILURE;
	}

	value &= 0xF0;
	value |= voltage;

	if(SetI2CDevRegisterValue(E_PERI_PMIC, 0x00, value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to set I2c register\n");
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetPmicSysmode(E_PMIC_SYSMODE mode)
{
	int value = 0;

	if(mode < E_SYSMODE_INTERRUPT || mode > E_SYSMODE_SHUTDOWN)
	{
		INFO_ERROR("Invalid mode to set\n");
		return E_JATIN_PARAM_ERR;
	}

	if(GetI2CDevRegisterValue(E_PERI_PMIC, 0x00, &value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read I2c register\n");
		return E_JATIN_FAILURE;
	}

	if(mode == E_SYSMODE_INTERRUPT)	{
		value |= (1 << 6); /* set 6th bit */
	} else if (mode == E_SYSMODE_SHUTDOWN) {
		value &= ~(1 << 6); /* clear 6th bit */
	}

	if(SetI2CDevRegisterValue(E_PERI_PMIC, 0x00, value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to set I2c register\n");
		return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetRegulatorPowerOkStatus(E_PMIC_REG regulator, E_REG_POWEROK *status)
{
	int value = 0;

	if(regulator < E_PMIC_REG1 || regulator > E_PMIC_REG7)
	{
		INFO_ERROR("Invalid regulator number\n");
		return E_JATIN_PARAM_ERR;
	}

	if(GetI2CDevRegisterValue(E_PERI_PMIC, PMICREGISTER[regulator], &value) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Failed to read I2c register\n");
		return E_JATIN_FAILURE;
	}

	if(value & 1) {
		*status = E_POWER_OK;
	} else {
		*status = E_POWER_NOT_OK;
	}

	return E_JATIN_SUCCESS;
}
