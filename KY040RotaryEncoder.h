/*
 * KY040RotaryEncoder.h
 *
 *  Created on: 2018. ápr. 15.
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

//A ClickEncoder service metódusát meghívó timer interrupt idõzítése -> 1 msec
#define ROTARY_ENCODER_SERVICE_INTERVAL_IN_USEC 1000

/**
 * KY-040 Rotary Encoder osztály
 */
class KY040RotaryEncoder: public ClickEncoder {

public:
	/**
	 * Irány enum
	 */
	typedef enum Direction_t {
		UP, DOWN, NONE
	} Direction;

private:
	int16_t lastRotaryValue;
	int16_t rotaryValue;
	Direction direction;

public:

	/**
	 * konstruktor
	 */
	KY040RotaryEncoder(uint8_t A, uint8_t B, uint8_t BTN = -1, uint8_t stepsPerNotch = 1, bool active = LOW) :	ClickEncoder(A, B, BTN, stepsPerNotch, active) {
		rotaryValue = 0;
		lastRotaryValue = getValue();
		direction = NONE;
	}


	/**
	 * A rotary encoder kezdõértékének kiolvasása
	 */
	void init(void) {

	}

	/**
	 * Irány elkérése
	 * Kiolvasás után az irányt töröjük
	 */
	const Direction getDirection(void) {
		Direction d = direction;
		direction = NONE;
		return d;
	}

	/**
	 * Rotary encoder olvasása, az irány meghatározása
	 */
	void readRotaryEncoder(void) {

		rotaryValue += getValue();

		if (rotaryValue / 2 > lastRotaryValue) {
			lastRotaryValue = rotaryValue / 2;
			direction = DOWN;
			delay(150);
		} else if (rotaryValue / 2 < lastRotaryValue) {
			lastRotaryValue = rotaryValue / 2;
			direction = UP;
			delay(150);
		} else {
			//direction = NONE;
		}

	}

	/**
	 * Klikkeltek?
	 */
	bool isClicked(void) {
		ClickEncoder::Button button = ClickEncoder::getButton();
		if(button != ClickEncoder::Open && button == ClickEncoder::Clicked){
			return true;
		}

		return false;
	}


};

#endif /* KY040ROTARYENCODER_H_ */
