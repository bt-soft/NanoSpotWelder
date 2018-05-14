/*
 *
 * Copyright 2018 - BT-Soft
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Buzzer.h
 *
 *  Created on: 2018. ápr. 26.
 *      Author: bt-soft
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
