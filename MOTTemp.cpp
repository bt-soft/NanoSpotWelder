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
	int adValue = analogRead(PIN_TEMP_SENSOR);
	float millivolts = MEASURED_MILIVOLT(adValue);
	currentMotTemp = KELVIN_TO_CELSIUS(millivolts / 10.0);
	currentMotTemp += SOFTWARE_TEML_AJDUST;

#ifdef SERIAL_DEBUG
	Serial.print("adValue: ");
	Serial.print(adValue);
	Serial.print(",  millivolts: ");
	Serial.println(millivolts);
#endif

#endif

	return currentMotTemp;
}
