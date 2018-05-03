/*
 * Nokia5110DisplayWrapper.h
 *
 *  Created on: 2018. ápr. 15.
 *      Author: BT
 */

#ifndef NOKIA5110DISPLAYWRAPPER_H_
#define NOKIA5110DISPLAYWRAPPER_H_

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define INVERSE_5110_BACKLIGHT_STATE	true 	/* Egyes Nokia 5110 LCD-k háttérvilágítása földre aktívak, ekkor true legyen */

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
		//begin(80 /* Constrast:0..127 */, 0x07 /* Bias:0...7 */);
		begin();
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
	 * Kontraszt beállítása
	 * @Override
	 */
	void setContrast(byte contrast) {
		Adafruit_PCD8544::setContrast(contrast);
		display();
	}

	/**
	 * Háttérvilágítás állítása
	 */
	void setBlackLightState(bool state) {
		if (blackLightPin != -1) {
			digitalWrite(blackLightPin,
#ifdef INVERSE_5110_BACKLIGHT_STATE && if(INVERSE_5110_BACKLIGHT_STATE == true)	//Egyes Nokia 5110 LCD-k háttérvilágítása földre aktívak -> negáljuk a 'state'-t
					!
#endif
					state ? HIGH : LOW);  //A Nokia 5110 LCD földre világít!!
		}
	}

};

#endif /* NOKIA5110DISPLAYWRAPPER_H_ */
