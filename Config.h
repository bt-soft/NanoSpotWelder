/*
 * Config.h
 *
 *  Created on: 2018. ápr. 15.
 *      Author: BT
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <Arduino.h>
#include <EEPROM.h>

#include "NanoSpotWelderVersion.h"
#include "NanoSpotWelderDefaults.h"

/**
 * Konfig osztály
 * Nem csinálunk a változóknak getter/setter metódusokat, nem növeljük a kód méretét feleslegesen
 */
class Config {

public:

	//A boolean változóket összelapátoljuk egy byte-ba
	typedef struct bits_t {
		unsigned char blackLightState :1;
		unsigned char beepState :1;
		unsigned char b2 :1;
		unsigned char b3 :1;
		unsigned char b4 :1;
		unsigned char b5 :1;
		unsigned char b6 :1;
		unsigned char b7 :1;
	} BitsT;

	//Uniont húzunk rá, hogy egy mûvelettel tudjuk az EEprom-ban kezelni
	typedef union BoolBits {
		BitsT bits;
		unsigned char byte;
	};

	//Konfigurációs típus deklaráció
	typedef struct config_t {
		unsigned char version[NSP_VERSION_SIZE];

		BoolBits boolBits;

		uint8_t contrast;
		uint8_t preWeldPulseCnt;
		uint8_t pausePulseCnt;
		uint8_t weldPulseCnt;

		uint8_t motTempAlarm;	//MOT hõmérséklet magas riasztás

	} ConfigT;

	//config
	ConfigT configVars;

	/**
	 * Default config létrehozása
	 */
	void createDefaultConfig(void) {
		//Mindent törlünk, ami eddig volt/jött a konfigban
		memset(&configVars, '\x0', sizeof(configVars));

		//Verzió info
		memcpy(&configVars.version, NSP_VERSION, NSP_VERSION_SIZE);

		//BitMap
		configVars.boolBits.bits.blackLightState = DEF_BACKLIGHT_STATE;
		configVars.boolBits.bits.beepState = DEF_BEEP_STATE;

		//LCD
		configVars.contrast = DEF_CONTRAST;

		//Weld
		configVars.preWeldPulseCnt = DEF_PREWELD_PULSE_CNT;
		configVars.pausePulseCnt = DEF_PAUSE_PULSE_CNT;
		configVars.weldPulseCnt = DEF_WELD_PULSE_CNT;

		//MOT Temp alarm
		configVars.motTempAlarm = DEF_MOT_TEMP_ALARM;
	}

public:

	/**
	 * Inicializáció
	 */
	void read() {
		EEPROM.get(0, configVars);

		//Érvények konfig van az EEprom-ban?
		if (memcmp(configVars.version, NSP_VERSION, NSP_VERSION_SIZE) != 0) {

			//nem -> legyártjuk a default konfigot
			createDefaultConfig();
			save();
		}
	}

	/**
	 * Konfig mentése
	 */
	void save(void) {
		//le is mentjük
		EEPROM.put(0, configVars);
	}

};

#endif /* CONFIG_H_ */
