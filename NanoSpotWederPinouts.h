/*
 * NanoSpotWederPinouts.h
 *
 *  Created on: 2018. ápr. 26.
 *      Author: bt-soft
 */

#ifndef NANOSPOTWEDERPINOUTS_H_
#define NANOSPOTWEDERPINOUTS_H_

#include <pins_arduino.h>

//------------------- Rotary Encoder
#define PIN_ENCODER_CLK        	3  		/* This pin must have a minimum 0.47 uF capacitor */
#define PIN_ENCODER_DT         	A0      /* data pin */
#define PIN_ENCODER_SW         	A1      /* switch pin (active LOW) */

//------------------- Hõmérés
#define PIN_TEMP_SENSOR			A2

//------------------- Ventilátor
#define PIN_VENTILATOR			A3

//------------------- LCD Display
#define PIN_LCD_BLACKLIGHT 		9
#define PIN_LCD_SCLK			8
#define PIN_LCD_DIN				7
#define PIN_LCD_DC				6
#define PIN_LCD_CS				5
#define PIN_LCD_RST				4

//------------------- Spot welding paraméterek
#define PIN_ZCD 				2		/* Zero Cross Detection PIN, megszakításban */
#define PIN_TRIAC 				12 		/* D12 Triac/MOC vezérlés PIN */
#define PIN_WELD_LED 			11		/* D11 Hegesztés LED visszajelzés */
#define PIN_WELD_BUTTON 		10 		/* D10 Hegesztés gomb */

//------------------- Beeper
#define PIN_BUZZER 				13

#endif /* NANOSPOTWEDERPINOUTS_H_ */
