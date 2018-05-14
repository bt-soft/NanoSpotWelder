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
 * Config.h
 *
 *  Created on: 2018. �pr. 15.
 *      Author: bt-soft
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <Arduino.h>
#include <EEPROM.h>

#include "Defaults.h"
#include "Version.h"

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

	//Konfigur�ci�s t�pus deklar�ci�
	typedef struct config_t {
		unsigned char version[NSP_VERSION_SIZE];

		//A logikai v�ltoz�kat nem lehet bitfield-be szervezni, mert a men�ben nem lehet r�juk pointert h�zni
		bool pulseCountWeldMode;	//pulzussz�ml�l�s (true) vagy k�zi(false) hegeszt�s
		bool blackLightState;		//h�tt�vil�g�t�s
		bool beepState;				//beep

		byte contrast;				//LCD kontraszt �rt�k
		byte bias;					//LCD el�fesz�t�s �rt�k
		uint8_t preWeldPulseCnt;	//el�hegeszt�s impulzus sz�m
		uint8_t pausePulseCnt;		//sz�net impulzus sz�m
		uint8_t weldPulseCnt;		//hegeszt�s impulzus sz�m

		byte motTempAlarm;			//MOT h�m�rs�klet magas riaszt�s

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

		//logikai v�ltoz�k
		configVars.pulseCountWeldMode = DEF_PULSE_COUNT_WELD_MODE;
		configVars.blackLightState = DEF_BACKLIGHT_STATE;
		configVars.beepState = DEF_BEEP_STATE;

		//LCD
		configVars.contrast = DEF_CONTRAST;
		configVars.bias = DEF_BIAS;

		//Weld
		configVars.preWeldPulseCnt = DEF_PREWELD_PULSE_CNT;
		configVars.pausePulseCnt = DEF_PAUSE_PULSE_CNT;
		configVars.weldPulseCnt = DEF_WELD_PULSE_CNT;

		//MOT Temp alarm
		configVars.motTempAlarm = DEF_MOT_TEMP_ALARM;
	}

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
extern Config *pConfig;

#endif /* CONFIG_H_ */
