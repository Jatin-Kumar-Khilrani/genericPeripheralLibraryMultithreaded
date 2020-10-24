#include <stdlib.h>
#include "LibInterface.h"

#define RED_PATH	"/sys/class/leds/red/brightness"
#define GREEN_PATH	"/sys/class/leds/green/brightness"
#define BLUE_PATH	"/sys/class/leds/blue/brightness"

#define ENGINE_SELECT "/sys/bus/i2c/devices/1-0030/select_engine"
#define FIRMWARE_LOADING	"/sys/class/firmware/lp5562/loading"
#define FIRMWARE_DATA	"/sys/class/firmware/lp5562/data"
#define RUN_ENGINE	"/sys/bus/i2c/devices/1-0030/run_engine"

//int ReadRegInt(char *pVarName, int *piNum);
//int WriteRegInt(char *pVarName, int pValue);
//int WriteRegString(char *pVarName, char *pValue);

#ifdef USE_ENGINE_MUX
E_JATIN_ERROR_CODES SetEngineMux()
{
	ExecuteSytemCommand("echo RGB > /sys/bus/i2c/devices/1-0030/engine_mux", NULL);
	return E_JATIN_SUCCESS;
}
#else

#define SetEngineMux()

#endif

E_JATIN_ERROR_CODES TurnOffLed()
{
	if(WriteRegInt(RED_PATH, 0) != E_JATIN_SUCCESS) {
		INFO_ERROR("Unable to turn off Red led\n");
		return E_JATIN_FAILURE;
	}
	if(WriteRegInt(GREEN_PATH, 0) != E_JATIN_SUCCESS){
		INFO_ERROR("Unable to turn off Green led\n");
		return E_JATIN_FAILURE;
	}
	if(WriteRegInt(BLUE_PATH, 0) != E_JATIN_SUCCESS){
		INFO_ERROR("Unable to turn off Blue led\n");
		return E_JATIN_FAILURE;
	}
	if(WriteRegInt(RUN_ENGINE, 0) != E_JATIN_SUCCESS){
		INFO_ERROR("Unable to stop the led engine\n");
			return E_JATIN_FAILURE;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetLedColor (E_LED_COLORS color, int brightness)
{
	if(color < E_LED_OFF || color > E_LED_COLOR_WHITE)
	{
		INFO_ERROR("Invalid Led Color Entered\n");
		return E_JATIN_PARAM_ERR;
	}

	//Need to turn off previous leds, before switching to new led color.
	if(TurnOffLed() != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Unable to turn off leds\n");
		return E_JATIN_FAILURE;
	}

	if(SetLedBrightness(color, brightness) != E_JATIN_SUCCESS)
	{
		INFO_ERROR("Unable to set Led color\n");
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES GetLedColor (E_LED_COLORS *color)
{
	int red;
	int green;
	int blue;

	//If unable to read current color values, simply return.
	if(ReadRegInt(RED_PATH, &red) != E_JATIN_SUCCESS){
		INFO_ERROR("unable to read Red led status\n");
		return E_JATIN_FAILURE;
	}

	if(ReadRegInt(GREEN_PATH, &green) != E_JATIN_SUCCESS){
		INFO_ERROR("Unable to read Green led status\n");
		return E_JATIN_FAILURE;
	}

	if(ReadRegInt(BLUE_PATH, &blue) != E_JATIN_SUCCESS){
		INFO_ERROR("Unable to read Blue led status\n");
		return E_JATIN_FAILURE;
	}

	if(red > 0 && green > 0 && blue > 0)
		*color =  E_LED_COLOR_WHITE;
	else if(blue > 0 && red > 0)
		*color = E_LED_COLOR_MAGENTA;
	else if(green > 0 && red > 0)
		*color = E_LED_COLOR_YELLOW;
	else if(green > 0 && blue > 0)
		*color = E_LED_COLOR_CYAN;
	else if(red > 0)
		*color = E_LED_COLOR_RED;
	else if(green > 0)
		*color = E_LED_COLOR_GREEN;
	else if(blue > 0)
		*color = E_LED_COLOR_BLUE;

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetLedBrightness (E_LED_COLORS color, int brightness)
{
	if(brightness < 0 || brightness > 255)
	{
		INFO_ERROR("Invalid brightness value\n");
		return E_JATIN_FAILURE;
	}

	SetEngineMux();
	switch(color)
	{
		case E_LED_COLOR_RED:
			if(WriteRegInt(RED_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to set brightness of red led\n");
				return E_JATIN_FAILURE;
			}
			break;
		case E_LED_COLOR_GREEN:
			if(WriteRegInt(GREEN_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to set brightness of green led\n");
				return E_JATIN_FAILURE;
			}
			break;
		case E_LED_COLOR_BLUE:
			if(WriteRegInt(BLUE_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to set brightness of blue led\n");
				return E_JATIN_FAILURE;
			}
			break;
		case E_LED_COLOR_CYAN:
			if(WriteRegInt(GREEN_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to turn on Green led\n");
				return E_JATIN_FAILURE;
			}
			if(WriteRegInt(BLUE_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to turn on Blue led\n");
				return E_JATIN_FAILURE;
			}
			break;
		case E_LED_COLOR_YELLOW:
			if(WriteRegInt(RED_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to turn on Red led\n");
				return E_JATIN_FAILURE;
			}
			if(WriteRegInt(GREEN_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to turn on Green led\n");
				return E_JATIN_FAILURE;
			}
			break;
		case E_LED_COLOR_MAGENTA:
			if(WriteRegInt(BLUE_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to turn on Blue led\n");
				return E_JATIN_FAILURE;
			}
			if(WriteRegInt(RED_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to turn on Red led\n");
				return E_JATIN_FAILURE;
			}
			break;
		case E_LED_COLOR_WHITE:
			if(WriteRegInt(GREEN_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to turn on Green led\n");
				return E_JATIN_FAILURE;
			}
			if(WriteRegInt(BLUE_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to turn on Blue led\n");
				return E_JATIN_FAILURE;
			}
			if(WriteRegInt(RED_PATH, brightness) != E_JATIN_SUCCESS){
				INFO_ERROR("Unable to turn on Red led\n");
				return E_JATIN_FAILURE;
			}
			break;
		case E_LED_OFF:
			// Turn off led is called from SetLedColor
			break;
		default:
			INFO_ERROR("Wrong set-color brightness request\n");
			return E_JATIN_NOT_SUPPORTED;
	}
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES FormBlinkData(int duration_on, int duration_off, char* data)
{
	int interval_on = (int)(duration_on / 998);
	int step_size_on = duration_on / ( 15.6 * (interval_on +1));
	unsigned short data_on = 0x4000 | (step_size_on << 8 ) | (interval_on);
	sprintf(data,"%s%x",data,0x4000);

	int interval_off = (int)(duration_off / 998);
	int step_size_off = duration_off / ( 15.6 * (interval_off +1));
	unsigned short data_off = 0x4000 | (step_size_off << 8 ) | (interval_off);
	sprintf(data,"%s%x",data,data_off);

	sprintf(data,"%s%x",data,0x40FF);
	sprintf(data, "%s%x",data,data_on);
	sprintf(data, "%s0000%x",data,0xD800);

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES BlinkLed(E_LED_COLORS color, int duration_on, int duration_off)
{
	char data[25];
	memset(data, 0, sizeof(data));

	if(color < E_LED_COLOR_RED || color > E_LED_COLOR_BLUE)
	{
		INFO_ERROR("Invalid color. Please enter Red, Green or Blue\n");
		return E_JATIN_PARAM_ERR;
	}

	SetEngineMux();
	sleep(0.5);
	if(WriteRegInt(ENGINE_SELECT, color) != E_JATIN_SUCCESS){
		INFO_ERROR("Unable to select engine\n");
		return E_JATIN_FAILURE;
	}
	sleep(1);
	if(WriteRegInt(FIRMWARE_LOADING, 1) != E_JATIN_SUCCESS){
		INFO_ERROR("Unable to load the firmware\n");
		return E_JATIN_FAILURE;
	}

	FormBlinkData(duration_on, duration_off, data);

	if(WriteRegString(FIRMWARE_DATA, data) != E_JATIN_SUCCESS){
		INFO_ERROR("Unable to set firmware data\n");
		return E_JATIN_FAILURE;
	}
	sleep(0.05);
	if(WriteRegInt(FIRMWARE_LOADING, 0) != E_JATIN_SUCCESS){
		INFO_ERROR("Unable to load the firmware\n");
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES SetLedBlink (E_LED_COLORS color, int duration_on, int duration_off)
{
	//First turn off the current Leds.
	if(TurnOffLed() != E_JATIN_SUCCESS){
		INFO_ERROR("Unable to turn off leds\n");
		return E_JATIN_FAILURE;
	}

	switch(color){

		case E_LED_COLOR_RED:
		case E_LED_COLOR_GREEN:
		case E_LED_COLOR_BLUE:
			if(BlinkLed(color, duration_on, duration_off) != E_JATIN_SUCCESS)
				return E_JATIN_FAILURE;
			break;

		case E_LED_COLOR_CYAN:
			if(BlinkLed(E_LED_COLOR_GREEN, duration_on, duration_off) != E_JATIN_SUCCESS)
				return E_JATIN_FAILURE;
			if(BlinkLed(E_LED_COLOR_BLUE, duration_on, duration_off) != E_JATIN_SUCCESS)
				return E_JATIN_FAILURE;
			break;

		case E_LED_COLOR_YELLOW:
			if(BlinkLed(E_LED_COLOR_RED, duration_on, duration_off) != E_JATIN_SUCCESS)
				return E_JATIN_FAILURE;
			if(BlinkLed(E_LED_COLOR_GREEN, duration_on, duration_off) != E_JATIN_SUCCESS)
				return E_JATIN_FAILURE;
			break;

		case E_LED_COLOR_MAGENTA:
			if(BlinkLed(E_LED_COLOR_RED, duration_on, duration_off) != E_JATIN_SUCCESS)
				return E_JATIN_FAILURE;
			if(BlinkLed(E_LED_COLOR_BLUE, duration_on, duration_off) != E_JATIN_SUCCESS)
				return E_JATIN_FAILURE;
			break;

		case E_LED_COLOR_WHITE:
			if(BlinkLed(E_LED_COLOR_RED, duration_on, duration_off) != E_JATIN_SUCCESS)
				return E_JATIN_FAILURE;
			if(BlinkLed(E_LED_COLOR_GREEN, duration_on, duration_off) != E_JATIN_SUCCESS)
				return E_JATIN_FAILURE;
			if(BlinkLed(E_LED_COLOR_BLUE, duration_on, duration_off) != E_JATIN_SUCCESS)
				return E_JATIN_FAILURE;
			break;

		default:
			INFO_ERROR("Wrong request, Unable to blink led!\n");
			return E_JATIN_NOT_SUPPORTED;
	}
	//Run the engine.
	sleep(1);
	if(WriteRegInt(RUN_ENGINE, 1) != E_JATIN_SUCCESS){
		INFO_ERROR("Unable to run the led engine\n");
		return E_JATIN_FAILURE;
	}

	return E_JATIN_SUCCESS;
}
