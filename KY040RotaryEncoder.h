/*
 * KY040RotaryEncoder.h
 *
 *  Created on: 2018. �pr. 15.
 *      Author: BT
 */

#ifndef KY040ROTARYENCODER_H_
#define KY040ROTARYENCODER_H_

#include "ClickEncoder.h"

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
 * With a 5 (3 wire encoder, 2 wire button) wire encoder you should wire as follows:
 *   Encoder A = A1
 *   Encoder B = A0
 *   Encoder Common = GND
 *   Encoder Button_1 = A2
 *   Encoder Button_2 = GND
 *
 * encoder = new ClickEncoder(A1, A0, A2);
 * encoder->setAccelerationEnabled(false);
 */

//A ClickEncoder service met�dus�t megh�v� timer interrupt id�z�t�se -> 1 msec
#define ROTARY_ENCODER_SERVICE_INTERVAL_IN_USEC 1000

#define ROTARY_ENCODER_DIVIDER	2

/**
 * KY-040 Rotary Encoder oszt�ly
 */
class KY040RotaryEncoder: public ClickEncoder {

public:

	/**
	 * Ir�ny enum
	 */
	typedef enum Direction_t {
		UP, DOWN, NONE
	} Direction;

	/**
	 * Visszat�r�si �rt�k
	 */
	typedef struct KY040RotaryEncoderResult_T {
		Direction_t direction;bool clicked;
	} KY040RotaryEncoderResult;

private:
	int16_t lastRotaryValue = 0;
	int16_t rotaryValue = 0;

public:

	/**
	 * konstruktor
	 */
	KY040RotaryEncoder(uint8_t A, uint8_t B, uint8_t BTN = -1, uint8_t stepsPerNotch = 1, bool active = LOW) :
			ClickEncoder(A, B, BTN, stepsPerNotch, active) {
	}

	/**
	 * A rotary encoder kezd��rt�k�nek kiolvas�sa
	 */
	void init(void) {
		lastRotaryValue = ClickEncoder::getValue();
	}

	/**
	 * Rotary encoder olvas�sa, az ir�ny meghat�roz�sa
	 */
	KY040RotaryEncoderResult readRotaryEncoder(void) {

		KY040RotaryEncoderResult result;

		//Gomb lenyom�s �llapot�nak kiolvas�sa
		ClickEncoder::Button button = ClickEncoder::getButton();
		result.clicked = button != ClickEncoder::Open && button == ClickEncoder::Clicked;

		//Ir�ny meg�llap�t�sa
		rotaryValue += ClickEncoder::getValue();

		int16_t divVal = rotaryValue / ROTARY_ENCODER_DIVIDER;

		if (divVal > lastRotaryValue) {
			result.direction = UP;
			lastRotaryValue = divVal;
			delay(150);

		} else if (divVal < lastRotaryValue) {
			result.direction = DOWN;
			lastRotaryValue = divVal;
			delay(150);

		} else {
			result.direction = NONE;
		}

		return result;
	}

};

#endif /* KY040ROTARYENCODER_H_ */
