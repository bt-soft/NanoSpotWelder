/*
 * Environment.h
 *
 *  Created on: 2018. m�j. 6.
 *      Author: bt-soft
 */

#ifndef ENVIRONMENT_H_
#define ENVIRONMENT_H_

//Serial konzol debug ON
//#define SERIAL_DEBUG
#ifdef SERIAL_DEBUG
#define SERIAL_BAUD_RATE 9600
#endif

// Digit�lis (DS18B30) a h�m�r� szenzor? Ha LM335, akkor ki kell kommentezni
//#define USE_DIGITAL_TEMPERATURE_SENSOR

#endif /* ENVIRONMENT_H_ */
