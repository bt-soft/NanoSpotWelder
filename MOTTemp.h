/*
 * MOTTemp.h
 *
 *  Created on: 2018. máj. 6.
 *      Author: BT
 */

#ifndef MOTTEMP_H_
#define MOTTEMP_H_

#include <Arduino.h>
#include "NanoSpotWederPinouts.h"


#ifdef USE_DIGITAL_TEMPERATURE_SENSOR
#include <OneWire.h>
#include <DallasTemperature.h>
#define MOT_TEMP_SENSOR_NDX 	0		/* Dallas DS18B20 hõmérõ szenzor indexe */
#else
#define MEASURED_MILIVOLT(x) (((x) / 1024.0) * REAL_5V_VCC_VALUE * 1000.0)
#define KELVIN_TO_CELSIUS(x) ((x) - 273.15)

#endif

class MOTTemp {

public:
	MOTTemp();
	float getTemperature(void);


public:
	float lastMotTemp = -1.0f;				//A MOT utolsó mért hõmérséklete

};

#endif /* MOTTEMP_H_ */
