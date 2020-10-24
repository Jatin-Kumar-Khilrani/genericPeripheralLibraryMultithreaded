#include "LibInterface.h"
#include <time.h>

#define PATH "/opt/bmi160/"
/************************************
*		Files Related to Scale		*
*************************************/
#define FILE_ACCEL_SCALE PATH"in_accel_scale"
#define FILE_GYRO_SCALE PATH"in_anglvel_scale"
#define FILE_MAGN_SCALE PATH"in_magn_scale"

/************************************
*		Files Related to ODR		*
*************************************/
#define FILE_ACCEL_ODR PATH"in_accel_sampling_frequency"
#define FILE_GYRO_ODR PATH"in_anglvel_sampling_frequency"
#define FILE_MAGN_ODR PATH"in_magn_sampling_frequency"

/************************************
*		Files Related to AXIS		*
*************************************/
#define FILE_ACCEL_X_AXIS PATH"in_accel_x_raw"
#define FILE_ACCEL_Y_AXIS PATH"in_accel_y_raw"
#define FILE_ACCEL_Z_AXIS PATH"in_accel_z_raw"

#define FILE_GYRO_X_AXIS PATH"in_anglvel_x_raw"
#define FILE_GYRO_Y_AXIS PATH"in_anglvel_y_raw"
#define FILE_GYRO_Z_AXIS PATH"in_anglvel_z_raw"

#define FILE_MAGN_X_AXIS PATH"in_magn_x_raw"
#define FILE_MAGN_Y_AXIS PATH"in_magn_y_raw"
#define FILE_MAGN_Z_AXIS PATH"in_magn_z_raw"

/*************************************
*		ACCEL + GYRO ODR in Hz		 *
**************************************/
#define E_BMI160_ODR_12P5 12.5
#define E_BMI160_ODR_25	25
#define E_BMI160_ODR_50 50
#define E_BMI160_ODR_100 100
#define E_BMI160_ODR_200 200
#define E_BMI160_ODR_400 400
#define E_BMI160_ODR_800 800
#define E_BMI160_ODR_1600 1600
#define E_BMI160_ODR_3200 3200

E_JATIN_ERROR_CODES ReadAllAxesData (sSensorData *data, E_9AXIS_SNR_AXIS sensor)
{
	float accel_scale, gyro_scale;
	float x_accel, y_accel, z_accel;
	float x_gyro, y_gyro, z_gyro;
	//float x_magn, y_magn, z_magn;

	memset(data, 0, sizeof(sSensorData));

	/* Validate Sensor type*/
	if(sensor < E_9AXIS_SNR_ACCELEROMETER || sensor > E_9AXIS_SNR_ALL_AXES)
	{
		INFO_ERROR("Invalid sensor type\n");
		return E_JATIN_NOT_SUPPORTED;
	}

	/* Read ACCELEROMETER Axis Data*/
	if(sensor == E_9AXIS_SNR_ACCELEROMETER || sensor == E_9AXIS_SNR_ALL_AXES)
	{
		/* Read scale Value */
		if(ReadRegFloat(FILE_ACCEL_SCALE, &accel_scale) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Unable to read ACCELEROMETER Scale Value\n");
			return E_JATIN_FAILURE;
		}

		/* Read ACCELEROMETER X-Axis Data*/
		if(ReadRegFloat(FILE_ACCEL_X_AXIS, &x_accel) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Unable to read ACCELEROMETER X-axis data\n");
			return E_JATIN_FAILURE;
		}

		/* Read ACCELEROMETER Y-Axis Data*/
		if(ReadRegFloat(FILE_ACCEL_Y_AXIS, &y_accel) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Unable to read ACCELEROMETER Y-axis data\n");
			return E_JATIN_FAILURE;
		}

		/* Read ACCELEROMETER Z-Axis Data*/
		if(ReadRegFloat(FILE_ACCEL_Z_AXIS, &z_accel) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Unable to read ACCELEROMETER Z-axis data\n");
			return E_JATIN_FAILURE;
		}

		/* Once All axis data is read, multiply them with respective scale
		 * and then assign the value to structure members
		 */

		data->X_accel = x_accel * accel_scale;
		data->Y_accel = y_accel * accel_scale;
		data->Z_accel = z_accel * accel_scale;
	}

	/* Read GYROSCOPE Axis Data*/
	if(sensor == E_9AXIS_SNR_GYROSCOPE || sensor == E_9AXIS_SNR_ALL_AXES)
	{
		/* Read scale Value */
		if(ReadRegFloat(FILE_GYRO_SCALE, &gyro_scale) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Unable to read GYROSCOPE Scale Value\n");
			return E_JATIN_FAILURE;
		}

		/* Read GYROSCOPE X-Axis Data*/
		if(ReadRegFloat(FILE_GYRO_X_AXIS, &x_gyro) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Unable to read GYROSCOPE X-axis data\n");
			return E_JATIN_FAILURE;
		}

		/* Read GYROSCOPE Y-Axis Data*/
		if(ReadRegFloat(FILE_GYRO_Y_AXIS, &y_gyro) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Unable to read GYROSCOPE Y-axis data\n");
			return E_JATIN_FAILURE;
		}

		/* Read GYROSCOPE Z-Axis Data*/
		if(ReadRegFloat(FILE_GYRO_Z_AXIS, &z_gyro) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Unable to read GYROSCOPE Z-axis data");
			return E_JATIN_FAILURE;
		}

		/* Once All axis data is read, multiply them with respective scale
		 * and then assign the value to structure members
		 */

		data->X_gyro = x_gyro * gyro_scale;
		data->Y_gyro = y_gyro * gyro_scale;
		data->Z_gyro = z_gyro * gyro_scale;
	}

	/* Read MAGNETOMETER Axis Data*/
	if(sensor == E_9AXIS_SNR_MAGNETOMETER || sensor == E_9AXIS_SNR_ALL_AXES)
	{
		#if 0
		/* Read MAGNETOMETER X-Axis Data*/
		if(ReadRegFloat(FILE_MAGN_X_AXIS, &x_magn))
		{
			printf("Unable to read MAGNETOMETER X-axis data\n");
			return 0;
		}

		/* Read MAGNETOMETER Y-Axis Data*/
		if(ReadRegFloat(FILE_MAGN_Y_AXIS, &y_magn))
		{
			printf("Unable to read MAGNETOMETER Y-axis data\n");
			return 0;
		}

		/* Read MAGNETOMETER Z-Axis Data*/
		if(ReadRegFloat(FILE_MAGN_Z_AXIS, &z_magn))
		{
			printf("Unable to read MAGNETOMETER Z-axis data\n");
			return 0;
		}

		/* Once All axis data is read, multiply them with respective scale
		 * and then assign the value to structure members
		 */

			data->X_magn = x_magn * magn_scale;
			data->Y_magn = y_magn * magn_scale;
			data->Z_magn = z_magn * magn_scale;
		#endif
		if(ReadRegFloat(FILE_MAGN_X_AXIS, &data->X_magn) != E_JATIN_SUCCESS)
        {
            INFO_ERROR("Unable to read MAGNETOMETER X-axis data\n");
            return E_JATIN_FAILURE;
        }

        /* Read MAGNETOMETER Y-Axis Data*/
        if(ReadRegFloat(FILE_MAGN_Y_AXIS, &data->Y_magn)!= E_JATIN_SUCCESS)
        {
            INFO_ERROR("Unable to read MAGNETOMETER Y-axis data\n");
            return E_JATIN_FAILURE;
        }

        /* Read MAGNETOMETER Z-Axis Data*/
        if(ReadRegFloat(FILE_MAGN_Z_AXIS, &data->Z_magn) != E_JATIN_SUCCESS)
        {
            INFO_ERROR("Unable to read MAGNETOMETER Z-axis data\n");
            return E_JATIN_FAILURE;
        }

	}

	data->timestamp = time(NULL);
	return E_JATIN_SUCCESS;
}

E_JATIN_ERROR_CODES setODR(float odr, E_9AXIS_SNR_AXIS sensor)
{
	if(sensor == E_9AXIS_SNR_ACCELEROMETER)
	{
		/* Check if odr_to_set is valid or not. If Valid then set, otherwise return */
		if((odr != E_BMI160_ODR_12P5)	&&
			(odr != E_BMI160_ODR_25)	&&
			(odr != E_BMI160_ODR_50)	&&
			(odr != E_BMI160_ODR_100)	&&
			(odr != E_BMI160_ODR_200)	&&
			(odr != E_BMI160_ODR_400)	&&
			(odr != E_BMI160_ODR_800)	&&
			(odr != E_BMI160_ODR_1600))
		{
			INFO_ERROR("Invalid ODR Value For Accelerometer\n");
			return E_JATIN_NOT_SUPPORTED;
		}

		if(WriteRegFloat(FILE_ACCEL_ODR, odr) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Unable to Set Accelerometer ODR\n");
			return E_JATIN_FAILURE;
		}
		return E_JATIN_SUCCESS;
	}

	else if(sensor == E_9AXIS_SNR_GYROSCOPE)
	{
		if((odr != E_BMI160_ODR_25)		&&
			(odr != E_BMI160_ODR_50)	&&
			(odr != E_BMI160_ODR_100)	&&
			(odr != E_BMI160_ODR_200)	&&
			(odr != E_BMI160_ODR_400)	&&
			(odr != E_BMI160_ODR_800)	&&
			(odr != E_BMI160_ODR_1600)	&&
			(odr != E_BMI160_ODR_3200))
		{
			INFO_ERROR("Invalid ODR Value For Gyroscope\n");
			return E_JATIN_NOT_SUPPORTED;
		}

		if(WriteRegFloat(FILE_GYRO_ODR, odr) != E_JATIN_SUCCESS)
		{
			INFO_ERROR("Unable to Set Gyroscope ODR\n");
			return E_JATIN_FAILURE;
		}
		return E_JATIN_SUCCESS;
	}
	#if 0
	else if(sensor == E_9AXIS_SNR_MAGNETOMETER)
	{
		/* Check if odr_to_set is valid or not. If Valid then set, otherwise return */
		if(odr_to_set == 0.78125 || odr_to_set == 1.5625 || odr_to_set == 3.125 ||
			odr_to_set == 6.25 || odr_to_set == 12.5 || odr_to_set == 25 ||
			odr_to_set == 50 || odr_to_set == 100 || odr_to_set == 200 ||
			odr_to_set == 400 || odr_to_set == 800 || odr_to_set == 1600)
		{
			if(WriteRegFloat(FILE_MAGN_ODR, odr_to_set))
			{
				printf("Unable to Set MAgnetometer ODR\n");
				return 0;
			}
			return 1;
		}

		else
		{
			printf("Invalid ODR Value\n");
			return 0;
		}
	}
	#endif
	else
	{
		INFO_ERROR("Invalid Sensor type\n");
		return E_JATIN_NOT_SUPPORTED;
	}
}
