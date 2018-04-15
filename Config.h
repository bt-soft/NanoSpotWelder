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

	//A boolean v�ltoz�ket �sszelap�toljuk egy byte-ba
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

	//Uniont h�zunk r�, hogy egy m�velettel tudjuk az EEprom-ban kezelni
	typedef union BoolBits {
		BitsT bits;
		unsigned char byte;
	};

	//Konfigur�ci�s t�pus deklar�ci�
	typedef struct config_t {
		unsigned char version[NSP_VERSION_SIZE];

		BoolBits boolBits;

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
	 * Inicializ�ci�
	 */
	void read() {
		EEPROM.get(0, configVars);

		//�rv�nyek konfig van az EEprom-ban?
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
	}

};

#endif /* CONFIG_H_ */
