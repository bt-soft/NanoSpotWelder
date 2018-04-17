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

	/**
	 * Kell menteni a konfig-et?
	 */
	bool wantSaveConfig = false;

//	//A boolean változókat összelapátoljuk egy byte-ba
//	typedef struct bits_t {
//		bool blackLightState :1;bool beepState :1;bool b2 :1;bool b3 :1;bool b4 :1;bool b5 :1;bool b6 :1;bool b7 :1;
//	} BitsT;

	//Konfigurációs típus deklaráció
	typedef struct config_t {
		unsigned char version[NSP_VERSION_SIZE];

		//BoolBits boolBits;
		//BitsT bits;

		bool blackLightState;
		bool beepState;

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
		configVars.blackLightState = DEF_BACKLIGHT_STATE;
		configVars.beepState = DEF_BEEP_STATE;

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

		//Érvényes konfig van az EEprom-ban?
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
		wantSaveConfig = false;
	}

};

#endif /* CONFIG_H_ */
