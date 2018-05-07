/*
 * Environment.h
 *
 *  Created on: 2018. m�j. 6.
 *      Author: BT
 */

#ifndef ENVIRONMENT_H_
#define ENVIRONMENT_H_

//Serial konzol debug ON
//#define SERIAL_DEBUG


// Digit�lis (DS18B30) a h�m�r� szenzor? Ha LM335, akkor ki kell kommentezni
#define USE_DIGITAL_TEMPERATURE_SENSOR

//A 5V t�nyleges t�pfesz�lt�sg �rt�ke V-ban
//Az LM335-�s h�m�r�s m�r�s sor�n fontos a pontos �rt�ke
#ifndef USE_DIGITAL_TEMPERATURE_SENSOR
//#define REAL_5V_VCC_VALUE 			4.732 	/* Arduino nano pr�bapanelen, PC USB portr�l t�pl�lva */
#define REAL_5V_VCC_VALUE 			5.056	/* �sszerakott Spot Welder saj�t 7805 t�ppal */
#define SOFTWARE_TEMP_AJDUST		-5.00	/* Szoftveres LM335 adjust */
#endif

#endif /* ENVIRONMENT_H_ */
