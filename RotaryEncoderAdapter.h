/*
 * RotaryEncoderAdapter.h
 *
 *  Created on: 2018. ápr. 17.
 *      Author: BT
 */

#define SMALLEST_CODESIZE
#include "ky-040.h"

#ifndef ROTARYENCODERADAPTER_H_
#define ROTARYENCODERADAPTER_H_

/**
 * KY-040 Rotary encoder adapter
 *  - lenyomás detektálása
 *  - irány detektálása
 */
class RotaryEncoderAdapter: public ky040 {

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
		RotaryEncoderAdapter(uint8_t interruptClkPin, uint8_t dataPin, uint8_t switchPin, uint8_t maxRotarys = 1) :
				ky040(interruptClkPin, dataPin, switchPin, maxRotarys) {
		}

		/**
		 * Init
		 */
		void init() {
			//--- Rotary Encoder felhúzása
			// Intervallum: -50...+50
			// Lépés: 2
			// Kezdeti érték: 0
			// Túlhajtás: nem
			ky040::AddRotaryCounter(ROTARY_ID1, 0, -50, 50, 2, true /*rollOver*/);
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
