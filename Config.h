/*
 * Config.h
 *
 *  Created on: 2018. �pr. 15.
 *      Author: BT
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <Arduino.h>
#include <EEPROM.h>

#include "NanoSpotWelderVersion.h"
#include "NanoSpotWelderDefaults.h"

/**
 * Konfig oszt�ly
 * Nem csin�lunk a v�ltoz�knak getter/setter met�dusokat, nem n�velj�k a k�d m�ret�t feleslegesen
 */
class Config {

public:

	/**
	 * Kell menteni a konfig-et?
	 */
	bool wantSaveConfig = false;

//	//A boolean v�ltoz�kat �sszelap�toljuk egy byte-ba
//	typedef struct bits_t {
//		bool blackLightState :1;bool beepState :1;bool b2 :1;bool b3 :1;bool b4 :1;bool b5 :1;bool b6 :1;bool b7 :1;
//	} BitsT;

	//Konfigur�ci�s t�pus deklar�ci�
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

		uint8_t motTempAlarm;	//MOT h�m�rs�klet magas riaszt�s

	} ConfigT;

	//config
	ConfigT configVars;

	/**
	 * Default config l�trehoz�sa
	 */
	void createDefaultConfig(void) {
		//Mindent t�rl�nk, ami eddig volt/j�tt a konfigban
		memset(&configVars, '\x0', sizeof(configVars));

		//Verzi� info
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
	 * Inicializ�ci�
	 */
	void read() {
		EEPROM.get(0, configVars);

		//�rv�nyes konfig van az EEprom-ban?
		if (memcmp(configVars.version, NSP_VERSION, NSP_VERSION_SIZE) != 0) {

			//nem -> legy�rtjuk a default konfigot
			createDefaultConfig();
			save();
		}
	}

	/**
	 * Konfig ment�se
	 */
	void save(void) {
		//le is mentj�k
		EEPROM.put(0, configVars);
		wantSaveConfig = false;
	}

};

#endif /* CONFIG_H_ */
