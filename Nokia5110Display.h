/*
 * Nokia5110Display.h
 *
 *  Created on: 2018. ápr. 15.
 *      Author: BT
 */

#ifndef NOKIA5110DISPLAY_H_
#define NOKIA5110DISPLAY_H_

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define DEFAUL_CONTRAST 40

class Nokia5110Display: public Adafruit_PCD8544 {

public:
	uint8_t contrast;

private:
	uint8_t blackLightPin;

public:

	// Software SPI with explicit CS pin.
	Nokia5110Display(int8_t SCLK, int8_t DIN, int8_t DC, int8_t CS, int8_t RST) :
			Adafruit_PCD8544(SCLK, DIN, DC, CS, RST), contrast(DEFAUL_CONTRAST), blackLightPin(-1) {
	}

	// Software SPI with CS tied to ground.  Saves a pin but other pins can't be shared with other hardware.
	Nokia5110Display(int8_t SCLK, int8_t DIN, int8_t DC, int8_t RST) :
			Adafruit_PCD8544(SCLK, DIN, DC, RST), contrast(DEFAUL_CONTRAST), blackLightPin(-1) {
	}

	// Hardware SPI based on hardware controlled SCK (SCLK) and MOSI (DIN) pins. CS is still controlled by any IO pin.
	// NOTE: MISO and SS will be set as an input and output respectively, so be careful sharing those pins!
	Nokia5110Display(int8_t DC, int8_t CS, int8_t RST) :
			Adafruit_PCD8544(DC, CS, RST), contrast(DEFAUL_CONTRAST), blackLightPin(-1) {
	}

	virtual ~Nokia5110Display() {
	}

	/**
	 * Háttérvilágítás PIN beállítása
	 */
	void setBlackLightPin(int8_t blPin) {
		blackLightPin = blPin;
	}

	/**
	 * Kontraszt beállítása
	 */
	void setContrast(uint8_t _contrast) {
		contrast = _contrast;
		Adafruit_PCD8544::setContrast(contrast);
		Adafruit_PCD8544::display();
	}

	/**
	 * Háttérvilágítás toggle
	 */
	bool getBlackLightState(void) {
		return blackLightPin != -1 ? digitalRead(blackLightPin) == HIGH : false;
	}

	/**
	 * Háttérvilágítás állítása
	 */
	void setBlackLightState(bool state){
		digitalWrite(blackLightPin, state ? HIGH : LOW);
	}

};

#endif /* NOKIA5110DISPLAY_H_ */
