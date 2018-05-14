/*
 * RotaryEncoderWrapper.h
 *
 *  Created on: 2018. ápr. 17.
 *      Author: bt-soft
 */

#define SMALLEST_CODESIZE
#include "ky-040.h"

#ifndef ROTARYENCODERADAPTER_H_
#define ROTARYENCODERWRAPPER_H_

//Rotary Encoder
// https://github.com/Billwilliams1952/KY-040-Encoder-Library---Arduino

/*
 * KY-040- Rotary Encoder
 *
 *
 * https://www.google.hu/imgres?imgurl=https%3A%2F%2Fi0.wp.com%2Fhenrysbench.capnfatz.com%2Fwp-content%2Fuploads%2F2015%2F05%2FKeyes-KY-040-Rotary-Encoder-Pin-Outs.png&imgrefurl=http%3A%2F%2Fhenrysbench.capnfatz.com%2Fhenrys-bench%2Farduino-sensors-and-input%2Fkeyes-ky-040-arduino-rotary-encoder-user-manual%2F&docid=_c4mGtu-_4T-7M&tbnid=C4A0RLRlbinkiM%3A&vet=10ahUKEwjKlcq8lqvaAhWNLlAKHSK5Bs4QMwh8KDgwOA..i&w=351&h=109&bih=959&biw=1920&q=ky-040%20rotary%20encoder%20simulation%20proteus&ved=0ahUKEwjKlcq8lqvaAhWNLlAKHSK5Bs4QMwh8KDgwOA&iact=mrc&uact=8
 *  +--------------+
 *  |          CLK |-- Encoder pin A
 *  |           DT |-- Encoder pin B
 *  |           SW |-- PushButton SW
 *  |         +VCC |-- +5V
 *  |          GND |-- Encoder pin C
 *  +--------------+
 *
 * https://github.com/0xPIT/encoder/issues/7
 */
/**
 * KY-040 Rotary encoder adapter
 *  - lenyomás detektálása
 *  - irány detektálása
 */
class RotaryEncoderWrapper: public ky040 {

#define ROTARY_ID1			1		/* Rotary encoder ID */

private:
	int16_t lastRotaryValue = 0;

public:
	/**
	 * Irány enum
	 */
	typedef enum Direction_t {
		UP, DOWN, NONE
	} Direction;

	/**
	 * Visszatérési érték
	 */
	typedef struct t {
		Direction_t direction;bool clicked;
	} RotaryEncoderResult;

public:
	/**
	 * Konstruktor
	 */
	RotaryEncoderWrapper(uint8_t interruptClkPin, uint8_t dataPin, uint8_t switchPin, uint8_t maxRotarys = 1) :
			ky040(interruptClkPin, dataPin, switchPin, maxRotarys) {
	}

	/**
	 * Init
	 */
	void init() {
		//--- Rotary Encoder felhúzása
		// Kezdeti érték: 0
		// Intervallum: -255...+255
		// Lépés: 1
		// Túlhajtás: igen
		ky040::AddRotaryCounter(ROTARY_ID1, 0, -255, 255, 1, true /*rollOver*/);
		ky040::SetRotary(ROTARY_ID1);
		lastRotaryValue = ky040::GetRotaryValue(ROTARY_ID1);
	}

	/**
	 * Rotary encoder olvasása, az irány meghatározása
	 */
	RotaryEncoderResult readRotaryEncoder() {

		RotaryEncoderResult result;
		result.clicked = ky040::SwitchPressed();
		result.direction = NONE;

		if (ky040::HasRotaryValueChanged(ROTARY_ID1)) {
			int16_t currentValue = ky040::GetRotaryValue(ROTARY_ID1);
			if (!result.clicked) { //Egyszerre klikkelni és tekerni nem tud
				result.direction = currentValue > lastRotaryValue ? UP : DOWN;
			}
			lastRotaryValue = currentValue;
		}

		return result;
	}
};

#endif /* ROTARYENCODERADAPTER_H_ */
