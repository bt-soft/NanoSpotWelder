/*
 * Nokia5110DisplayWrapper.h
 *
 *  Created on: 2018. �pr. 15.
 *      Author: BT
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
		begin(pConfig->configVars.contrast /* Constrast:0..127 */, pConfig->configVars.bias /* Bias:0...7 */);
	}

	virtual ~Nokia5110DisplayWrapper() {
	}

	/**
	 * H�tt�rvil�g�t�s PIN be�ll�t�sa
	 */
	void setBlackLightPin(int8_t blPin) {
		blackLightPin = blPin;
		if (blackLightPin != -1) {
			pinMode(blackLightPin, OUTPUT);
		}
	}

	/**
	 * LCD Kontraszt be�ll�t�sa
	 * @Override
	 */
	void setContrast(byte contrast) {
		Adafruit_PCD8544::setContrast(contrast);
		display();
	}

	/**
	 * LCD El�fesz�t�s be�ll�t�sa
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
	 * H�tt�rvil�g�t�s �ll�t�sa
	 */
	void setBlackLightState(bool state) {
		if (blackLightPin != -1) {
			digitalWrite(blackLightPin, !state ? HIGH : LOW);  //A Nokia 5110 LCD LOW-ra vil�g�t
		}
	}

};

#endif /* NOKIA5110DISPLAYWRAPPER_H_ */
