/*
 * MOTTemp.h
 *
 *  Created on: 2018. m�j. 6.
 *      Author: BT
 */

#ifndef MOTTEMP_H_
#define MOTTEMP_H_

#include <Arduino.h>
#include "NanoSpotWederPinouts.h"
#include "Environment.h"

#ifdef USE_DIGITAL_TEMPERATURE_SENSOR
#include <OneWire.h>
#include <DallasTemperature.h>
#define MOT_TEMP_SENSOR_NDX 	0		/* Dallas DS18B20 h�m�r� szenzor indexe */
#else
#include <MD_LM335A.h>
#endif

class MOTTemp {

	public:
		MOTTemp();
		float getTemperature(void);

	public:
		float lastMotTemp = -1.0f;				//A MOT utols� m�rt h�m�rs�klete

};

#endif /* MOTTEMP_H_ */
