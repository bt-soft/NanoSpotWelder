/*
 * Environment.h
 *
 *  Created on: 2018. máj. 6.
 *      Author: BT
 */

#ifndef ENVIRONMENT_H_
#define ENVIRONMENT_H_

//Serial konzol debug ON
//#define SERIAL_DEBUG


// Digitális (DS18B30) a hõmérõ szenzor? Ha LM335, akkor ki kell kommentezni
#define USE_DIGITAL_TEMPERATURE_SENSOR

//A 5V tényleges tápfeszültésg értéke V-ban
//Az LM335-ös hõmérés mérés során fontos a pontos értéke
#ifndef USE_DIGITAL_TEMPERATURE_SENSOR
//#define REAL_5V_VCC_VALUE 			4.732 	/* Arduino nano próbapanelen, PC USB portról táplálva */
#define REAL_5V_VCC_VALUE 			5.056	/* Összerakott Spot Welder saját 7805 táppal */
#define SOFTWARE_TEMP_AJDUST		-5.00	/* Szoftveres LM335 adjust */
#endif

#endif /* ENVIRONMENT_H_ */
