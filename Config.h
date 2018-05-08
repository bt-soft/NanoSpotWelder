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

#include "Defaults.h"
#include "Version.h"

/**
 * Konfig osztály
 * Nem csinálunk a változóknak getter/setter metódusokat, nem növeljük a kód méretét feleslegesen
 */
class Config {

public:

	//Számított hálózati frekvencia
	float spotWelderSystemPeriodTime = 0.0;

	/**
	 * Kell menteni a konfig-et?
	 */
	bool wantSaveConfig = false;

	//Konfigurációs típus deklaráció
	typedef struct config_t {
		unsigned char version[NSP_VERSION_SIZE];

		//A logikai változókat nem lehet bitfield-be szervezni, mert a menüben nem lehet rájuk pointert húzni
		bool pulseCountWeldMode;	//pulzusszámlálás (true) vagy kézi(false) hegesztés
		bool blackLightState;		//háttévilágítás
		bool beepState;				//beep

		byte contrast;				//LCD kontraszt érték
		byte bias;					//LCD elõfeszítés érték
		uint8_t preWeldPulseCnt;	//elõhegesztés impulzus szám
		uint8_t pausePulseCnt;		//szünet impulzus szám
		uint8_t weldPulseCnt;		//hegesztés impulzus szám

		byte motTempAlarm;			//MOT hõmérséklet magas riasztás

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

		//logikai változók
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
extern Config *pConfig;

#endif /* CONFIG_H_ */
