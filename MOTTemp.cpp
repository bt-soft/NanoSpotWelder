/*
 * MOTTemp.cpp
 *
 *  Created on: 2018. máj. 6.
 *      Author: BT
 */

#include "MOTTemp.h"

#ifdef USE_DIGITAL_TEMPERATURE_SENSOR
OneWire oneWire(PIN_TEMP_SENSOR);
DallasTemperature tempSensors(&oneWire);
#else
LM335A Temp(PIN_TEMP_SENSOR);
#endif

/**
 * Konstruktor
 */
MOTTemp::MOTTemp() {
	//Hõmérés init
#ifdef USE_DIGITAL_TEMPERATURE_SENSOR
	tempSensors.begin();
#endif

}

/**
 * Hõmérés
 */
float MOTTemp::getTemperature(void) {

	float currentMotTemp = -1.0;

#ifdef USE_DIGITAL_TEMPERATURE_SENSOR
	//MOT Hõmérséklet lekérése -> csak egy DS18B20 mérõnk van -> 0 az index
	tempSensors.requestTemperaturesByIndex(MOT_TEMP_SENSOR_NDX);
	//MOT Hõmérséklet kiolvasása
	currentMotTemp = tempSensors.getTempCByIndex(MOT_TEMP_SENSOR_NDX);
#else
	Temp.Read();
	currentMotTemp = Temp.dC / 100.0f;
#endif

	return currentMotTemp;
}
