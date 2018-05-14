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
 * Nokia5110DisplayWrapper.h
 *
 *  Created on: 2018. ápr. 15.
 *      Author: bt-soft
 */

#ifndef NOKIA5110DISPLAYWRAPPER_H_
#define NOKIA5110DISPLAYWRAPPER_H_

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "Config.h"

class Nokia5110DisplayWrapper: public Adafruit_PCD8544 {

private:
	uint8_t blackLightPin = -1;

public:

	// Software SPI with explicit CS pin.
	Nokia5110DisplayWrapper(int8_t SCLK, int8_t DIN, int8_t DC, int8_t CS, int8_t RST, uint8_t blackLightPin = -1) :
			Adafruit_PCD8544(SCLK, DIN, DC, CS, RST) {

		if (blackLightPin != -1) {
			this->blackLightPin = blackLightPin;
			pinMode(blackLightPin, OUTPUT);
		}

		//Felhúzzuk az LCD-t a konfigban megadott értékekkel
		begin(pConfig->configVars.contrast /* Constrast:0..127 */, pConfig->configVars.bias /* Bias:0...7 */);
	}

	virtual ~Nokia5110DisplayWrapper() {
	}

	/**
	 * Háttérvilágítás PIN beállítása
	 */
	void setBlackLightPin(int8_t blPin) {
		blackLightPin = blPin;
		if (blackLightPin != -1) {
			pinMode(blackLightPin, OUTPUT);
		}
	}

	/**
	 * LCD Kontraszt beállítása
	 * @Override
	 */
	void setContrast(byte contrast) {
		Adafruit_PCD8544::setContrast(contrast);
		display();
	}

	/**
	 * LCD Elõfeszítés beállítása
	 */
	void setBias(byte bias) {
		if (bias > 7) {
			bias = 7;
		}
		command(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION);
		command(PCD8544_SETBIAS | bias);
		command(PCD8544_FUNCTIONSET);
		display();

	}

	/**
	 * Háttérvilágítás állítása
	 */
	void setBlackLightState(bool state) {
		if (blackLightPin != -1) {
			digitalWrite(blackLightPin, !state ? HIGH : LOW);  //A Nokia 5110 LCD LOW-ra világít
		}
	}

};

#endif /* NOKIA5110DISPLAYWRAPPER_H_ */
