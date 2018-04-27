/*
 * NanoSpotWederPinouts.h
 *
 *  Created on: 2018. �pr. 26.
 *      Author: u626374
 */

#ifndef NANOSPOTWEDERPINOUTS_H_
#define NANOSPOTWEDERPINOUTS_H_

#include <pins_arduino.h>

//------------------- Rotary Encoder
#define PIN_ENCODER_CLK        	3  		/* This pin must have a minimum 0.47 uF capacitor */
#define PIN_ENCODER_DT         	A0      /* data pin */
#define PIN_ENCODER_SW         	A1      /* switch pin (active LOW) */


//------------------- H�m�r�s
#define PIN_DALLAS18B20			A2

//------------------- Ventil�tor
#define PIN_VENTILATOR			A3


//------------------- Beeper
#define PIN_BUZZER 				13


//------------------- LCD Display
#define PIN_LCD_BLACKLIGHT 		9
#define PIN_LCD_SCLK			8
#define PIN_LCD_DIN				7
#define PIN_LCD_DC				6
#define PIN_LCD_CS				5
#define PIN_LCD_RST				4

//------------------- Spot welding param�terek
#define PIN_ZCD 				2		/* Zero Cross Detection PIN, megszak�t�sban */
#define PIN_TRIAC 				12 		/* D12 Triac/MOC vez�rl�s PIN */
#define PIN_WELD_LED 			11		/* D11 Hegeszt�s LED visszajelz�s */
#define PIN_WELD_BUTTON 		10 		/* D10 Hegeszt�s gomb */




#endif /* NANOSPOTWEDERPINOUTS_H_ */