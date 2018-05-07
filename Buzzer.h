/*
 * Buzzer.h
 *
 *  Created on: 2018. ápr. 26.
 *      Author: u626374
 */

#ifndef BUZZER_H_
#define BUZZER_H_

#include <Arduino.h>

#include "NanoSpotWederPinouts.h"
#include "Config.h"

class Buzzer {

public:
	/**
	 * Sipolás - Ha engedélyezve van
	 */
	void buzzer(void) {

		if (!pConfig->configVars.beepState) {
			return;
		}

		tone(PIN_BUZZER, 1000);
		delay(500);
		tone(PIN_BUZZER, 800);
		delay(500);
		noTone(PIN_BUZZER);
	}

	/**
	 * Riasztási hang
	 */
	void buzzerAlarm(void) {
		tone(PIN_BUZZER, 1000);
		delay(300);
		tone(PIN_BUZZER, 1000);
		delay(300);
		tone(PIN_BUZZER, 1000);
		delay(300);
		noTone(PIN_BUZZER);
	}

	/**
	 * menu hang
	 */
	void buzzerMenu() {
		if (!pConfig->configVars.beepState) {
			return;
		}

		tone(PIN_BUZZER, 500);
		delay(15);
		noTone(PIN_BUZZER);
	}

};

extern Buzzer *pBuzzer;

#endif /* BUZZER_H_ */
