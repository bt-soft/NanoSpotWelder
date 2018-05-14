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
 * NanoSpotWelder.cpp
 *
 *  Created on: 2018. ápr. 15.
 *      Author: bt-soft
 */
#include <Arduino.h>
#include <WString.h>

#include "NanoSpotWederPinouts.h"
#include "Config.h"
#include "LcdMenu.h"
#include "Buzzer.h"
#include "RotaryEncoderWrapper.h"
#include "MOTTemp.h"
#include "Environment.h"

//MOT hõmérsékletmérés
#define TEMP_DIFF_TO_DISPLAY  			0.5f	/* fél fokonkénti eltérésekkel foglalkozunk csak */
MOTTemp *pMOTTemp;

//--- Konfig support
Config *pConfig;

//--- Rotary Encoder
RotaryEncoderWrapper *pRotaryEncoder;

// --- Menü
//Másodpercenként nézünk rá a MOT hõmérsékletére
#define MAIN_DISPLAY_LOOP_TIME			1000

//Menü inaktivitási idõ másodpercben
#define MENU_INACTIVE_IN_MSEC			(30 /* Menü tétlenségi idõ limit mp-ben*/ * MAIN_DISPLAY_LOOP_TIME)
LcdMenu *lcdMenu;

// --- Buzzer
Buzzer *pBuzzer;

// -- Runtime adatok
long lastMiliSec = 0;				// hõmérséklet mmérésre, menü tétlenségi idõ detektálása*/

//A konfigban megadott ventilátor riasztási °C érték elõtt ennyivel bekapcsolunk, vagy ennyivel utána kikapcsolunk
#define VENTILATOR_TRIGGER_OFFSET_VALUE	10

/**
 * Ventilátor vezérlése
 */
void ventilatorController(float *currentMotTemp) {

	int triggerValue = pConfig->configVars.motTempAlarm - VENTILATOR_TRIGGER_OFFSET_VALUE;

	if (triggerValue > *currentMotTemp) {
		if (digitalRead(PIN_VENTILATOR)) {
			digitalWrite(PIN_VENTILATOR, LOW);
		}

	} else if (triggerValue <= *currentMotTemp) {
		if (!digitalRead(PIN_VENTILATOR)) {
			digitalWrite(PIN_VENTILATOR, HIGH);
		}
	}

#ifdef SERIAL_DEBUG
	Serial.print("PIN_VENTILATOR: ");
	Serial.print(digitalRead(PIN_VENTILATOR));

	Serial.print(", triggerValue: ");
	Serial.print(triggerValue);
	Serial.print(",  currentMotTemp: ");
	Serial.println(*currentMotTemp);
#endif
}

/**
 * Main display vezérlés
 * Ha MOT hõmérsékleti riasztás van, akkor true-val térünk vissza
 */
bool mainDisplayController(void) {

	bool highTempAlarm = false;

	float currentMotTemp = pMOTTemp->getTemperature();

	//Magas a hõmérséklet?
	if (currentMotTemp >= pConfig->configVars.motTempAlarm) {
		pMOTTemp->lastMotTemp = currentMotTemp;
		lcdMenu->drawWarningDisplay(&currentMotTemp);
		pBuzzer->buzzerAlarm();
		highTempAlarm = true;

	} else if (pMOTTemp->lastMotTemp == -1.0f || abs(pMOTTemp->lastMotTemp - currentMotTemp) > TEMP_DIFF_TO_DISPLAY) {
		//Csak az elsõ és a TEMP_DIFF_TO_DISPLAY-nál nagyobb eltérésekre reagálunk
		pMOTTemp->lastMotTemp = currentMotTemp;
		lcdMenu->drawMainDisplay(&currentMotTemp);
	}

	//Ventilátor vezérlés
	ventilatorController(&currentMotTemp);

#ifdef SERIAL_DEBUG
	Serial.print("MOT Temp: ");
	Serial.print(currentMotTemp);
	Serial.print(", highTempAlarm: ");
	Serial.println(highTempAlarm);
#endif

	return highTempAlarm;
}

/**
 * Elembeállító menü kontroller
 */
void itemMenuController(bool rotaryClicked, RotaryEncoderWrapper::Direction rotaryDirection) {

	if (lcdMenu->menuState != LcdMenu::ITEM_MENU) {
		return;
	}

	//Nem klikkeltek -> csak változtatják az elem értékét
	if (!rotaryClicked) {

		switch (rotaryDirection) {

			case RotaryEncoderWrapper::Direction::UP:
				lcdMenu->incSelectedValue();
				pConfig->wantSaveConfig = true;
				break;

			case RotaryEncoderWrapper::Direction::DOWN:
				lcdMenu->decSelectedValue();
				pConfig->wantSaveConfig = true;
				break;

				//Csak kirajzoltatást kértek
			case RotaryEncoderWrapper::Direction::NONE:
				break;
		}

		//Menüelem beálító képernyõ kirajzoltatása
		lcdMenu->drawMenuItemValue();

		//Ha van az almenühöz hozzá callback, akkor azt is meghívjuk
		lcdMenu->invokeMenuItemCallBackFunct();

		return;

	} //if (!rotaryClicked)

	//Klikkeltek az elem értékére -> kilépünk a menüelem-bõl a  fõmenübe
	lcdMenu->menuState = LcdMenu::MAIN_MENU;
	lcdMenu->drawMainMenu();
}

/**
 * Fõmenü kontroller
 */
void mainMenuController(bool rotaryClicked, RotaryEncoderWrapper::Direction rotaryDirection) {

	if (lcdMenu->menuState != LcdMenu::MAIN_MENU) {
		return;
	}

	//Nem klikkeltek -> csak tallózunk a menüben
	if (!rotaryClicked) {

		switch (rotaryDirection) {

			//Felfelé tekertek
			case RotaryEncoderWrapper::Direction::UP:
				lcdMenu->stepDown();
				break;

				//Lefelé tekertek
			case RotaryEncoderWrapper::Direction::DOWN:
				lcdMenu->stepUp();
				break;

				//Nincs irány
			default:
				return;
		}

		lcdMenu->drawMainMenu();
		return;

	} //if (!rotaryClicked)

	//
	// Klikkeltek a main menüben egy menüelemre
	// - Kinyerjük a kiválasztott menüelem pointerét
	// - Kirajzoltatjuk az elem értékét
	// - Átállítjuk az állapotott az elembeállításra
	//

	//Típus szerint megyünk tovább
	switch (lcdMenu->getSelectedItemPtr()->valueType) {

		//Ha ez egy értékbeállító almenü
		//Csak egy függvényt kell hívni, az majd elintéz mindent
		case LcdMenu::FUNCT:
			lcdMenu->invokeMenuItemCallBackFunct();
			break;

		default:
			lcdMenu->menuState = LcdMenu::ITEM_MENU;
			itemMenuController(false, RotaryEncoderWrapper::Direction::NONE); //Kérünk egy menüelem beállító képernyõ kirajzolást
			break;
	}
}

/**
 * Menu kezelése
 */
void menuController(bool rotaryClicked, RotaryEncoderWrapper::Direction rotaryDirection) {

	switch (lcdMenu->menuState) {

		// Nem látszik a fõmenü -> Ha kikkeltek, akkor belépünk a mübe
		case LcdMenu::OFF:
			if (rotaryClicked) {
				pBuzzer->buzzerMenu();
				lcdMenu->menuState = LcdMenu::MAIN_MENU;
				lcdMenu->drawMainMenu(); //Kirajzoltatjuk a fõmenüt
			}
			break;

			//Látszik a fõmenü
		case LcdMenu::MAIN_MENU:
			pBuzzer->buzzerMenu();
			mainMenuController(rotaryClicked, rotaryDirection);
			break;

			//Elem változtató menü látszik
		case LcdMenu::ITEM_MENU:
			pBuzzer->buzzerMenu();
			itemMenuController(rotaryClicked, rotaryDirection);
			break;
	}
}

/**
 * Menü inaktivitás kezelése
 */
void menuInactiveController(void) {

	switch (lcdMenu->menuState) {

		//Main Menüben vagyunk
		case LcdMenu::MAIN_MENU:
			lcdMenu->resetMenu(); //Kilépünk a menübõl...
			lcdMenu->menuState = LcdMenu::OFF;
			pMOTTemp->lastMotTemp = -1.0f; //kierõszakoljuk a main display újrarajzoltatását
			break;

			//Elembeállító menüben vagyunk
		case LcdMenu::ITEM_MENU:
			lcdMenu->drawMainMenu(); //Kilépünk az almenübõl a fõmenübe
			lcdMenu->menuState = LcdMenu::MAIN_MENU;
	}
}

//--- Spot Welding ---------------------------------------------------------------------------------------------------------------------------------------
typedef enum weldState_t {
	PRE_WELD, 		//Elõimpulzus
	PAUSE_WELD,  	//Szünet a két impulzus között
	WELD, 			//hegesztõ impulzus
	WELD_END 		//Nincs hegesztés
} WeldState_T;

volatile WeldState_T weldCurrentState = WELD_END; //hegesztési állapot jelzõ
volatile uint16_t weldPeriodCnt = 0; //Periódus Számláló, hegesztés alatt megszakításkor inkrementálódik

/**
 * ZCD interrupt rutin
 */
void zeroCrossDetect(void) {

	//Ha nincs pre (ez esetben már a pause sem érdekel), akkor mehet egybõl a weld
	if (weldCurrentState == PRE_WELD && pConfig->configVars.preWeldPulseCnt == 0) {
		weldPeriodCnt = 0;
		weldCurrentState = WELD;
	}

	switch (weldCurrentState) {

		//A pre-ben vagyunk
		case PRE_WELD:
			//A Triakot csak akkor kapcsoljuk be, ha van elõinpulzus szám a konfigban, és nincs még bekapcsolva
			if (!digitalRead(PIN_TRIAC) && pConfig->configVars.preWeldPulseCnt > 0) {
				weldPeriodCnt = 0;
				digitalWrite(PIN_TRIAC, HIGH); //TRIAC BE
				return;
			}

			if (++weldPeriodCnt >= pConfig->configVars.preWeldPulseCnt) {
				digitalWrite(PIN_TRIAC, LOW); //TRIAC KI
				weldPeriodCnt = 0;
				weldCurrentState = PAUSE_WELD;
			}
			break;

			//A pause-ban vagyunk
		case PAUSE_WELD:
			if (++weldPeriodCnt >= pConfig->configVars.pausePulseCnt) {
				weldPeriodCnt = 0;
				weldCurrentState = WELD;
			}
			break;

			//A fõ hegesztésben vagyunk
		case WELD:
			//bekapcsoljuk a triakot, ha még nincs bekapcsolva
			if (!digitalRead(PIN_TRIAC)) {
				weldPeriodCnt = 0;
				digitalWrite(PIN_TRIAC, HIGH); //TRIAC BE
				return;
			}

			//Ha elértük a pulzusszámot, akkor kikapcsolunk
			if (++weldPeriodCnt >= pConfig->configVars.weldPulseCnt) {
				digitalWrite(PIN_TRIAC, LOW); //TRIAC KI
				weldCurrentState = WELD_END;
			}
			break;

			//Hegesztés vége
		case WELD_END:
			//Igaziból itt már nem csinálunk semmit sem
			digitalWrite(PIN_TRIAC, LOW);	//triak ki, csak a biztonság kedvéért
			break;
	}
}

/**
 * Hegesztési protokoll
 */
void weldButtonPushed(void) {
#define SLEEP_TIME_MICROSEC 100

	//LED-be
	digitalWrite(PIN_WELD_LED, HIGH);

	//Ha pulzusszámlálás van
	if (pConfig->configVars.pulseCountWeldMode) {

		//Ráköltözünk a ZCD interrupt-ra
		attachInterrupt(digitalPinToInterrupt(PIN_ZCD), zeroCrossDetect, FALLING);

		//Beállítjuk, hogy a PRE_WELD állapotból induljunk
		weldCurrentState = PRE_WELD;

		//Megvárjuk a hegesztési folyamat végét
		while (weldCurrentState != WELD_END) {
			delayMicroseconds(SLEEP_TIME_MICROSEC);
		}

		//Leszállunk a ZCD interrupt-ról
		detachInterrupt(digitalPinToInterrupt(PIN_ZCD));

	} else { //Kézi hegesztés vezérlés van

		digitalWrite(PIN_TRIAC, HIGH); //TRIAC BE

		//Addig amíg a gomb le van nyomva, addig nem mozdulunk innen
		while (digitalRead(PIN_WELD_BUTTON)) {
			delayMicroseconds(SLEEP_TIME_MICROSEC * 20);
		}

		digitalWrite(PIN_TRIAC, LOW); //TRIAC KI
	}

	{ //Csak a biztonság kedvéért...
		digitalWrite(PIN_TRIAC, LOW);	//TRIAC ki
		weldCurrentState = WELD_END;    //Nincs hegesztés állapot
	}

	digitalWrite(PIN_WELD_LED, LOW); //LED ki
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------

/**
 * Boot beállítások
 */
void setup(void) {
#ifdef SERIAL_DEBUG
	Serial.begin(SERIAL_BAUD_RATE);
	while (!Serial)
	;
	Serial.println("Debug active");
#endif

	//Konfig felolvasása
	pConfig = new Config();
	pConfig->read();

	//Rotary encoder init
	pRotaryEncoder = new RotaryEncoderWrapper(PIN_ENCODER_CLK, PIN_ENCODER_DT, PIN_ENCODER_SW);
	pRotaryEncoder->init();
	pRotaryEncoder->SetChanged();   // kierõszakoljuk az állapot kiolvashatóságát
	pRotaryEncoder->readRotaryEncoder();   //elsõ kiolvasást eldobjuk

	//Buzzer init
	pBuzzer = new Buzzer();

	//Software SPI (slower updates, more flexible pin options):
	lcdMenu = new LcdMenu();
	lcdMenu->drawSplashScreen();

	pMOTTemp = new MOTTemp();

	//Weld button PIN
	pinMode(PIN_WELD_BUTTON, INPUT);

	//--- ZCD input felhúzása
	pinMode(PIN_ZCD, INPUT_PULLUP);

	//--- Triac PIN
	pinMode(PIN_TRIAC, OUTPUT);
	digitalWrite(PIN_TRIAC, LOW);

	//--- Weld LED PIN
	pinMode(PIN_WELD_LED, OUTPUT);
	digitalWrite(PIN_WELD_LED, LOW);

	//--- Ventilátor
	pinMode(PIN_VENTILATOR, OUTPUT);
	digitalWrite(PIN_VENTILATOR, LOW);

	//idõmérés indul
	lastMiliSec = millis();
}

/**
 * Main loop
 */
void loop(void) {

	static byte weldButtonPrevState = HIGH;   //A hegesztés gomb elõzõ állapota

	//
	// --- Hegesztés kezelése -------------------------------------------------------------------
	//

	//Kiolvassuk a weld button állapotát
	byte weldButtonCurrentState = digitalRead(PIN_WELD_BUTTON);

	//Ha változott az állapot LOW -> HIGH irányban
	if (weldButtonCurrentState != weldButtonPrevState && weldButtonCurrentState == HIGH && weldButtonPrevState == LOW) {
		weldButtonPushed();
		lcdMenu->menuState = LcdMenu::OFF; //kilépünk majd a menübõl, ha épp benne voltunk
		delay(100); //weld button debounce -> rövid impulzus csomagok esetén jól jöhet
	}
	//Eltesszük az aktuális button állapotot
	weldButtonPrevState = weldButtonCurrentState;

	//
	//--- MainDisplay kezelése --------------------------------------------------------------------
	//
	if (lcdMenu->menuState == LcdMenu::FORCE_MAIN_DISPLAY) {
		lcdMenu->menuState = LcdMenu::MAIN_MENU;
		menuInactiveController();
	} else if (lcdMenu->menuState == LcdMenu::OFF) {
		//Menü tétlenség figyelése
		if (millis() - lastMiliSec >= MAIN_DISPLAY_LOOP_TIME) {
			mainDisplayController();
			lastMiliSec = millis();
		}
	} else { //Valamilyen menüben vagyunk
		//Menütétlenség figyelése
		if ((millis() - lastMiliSec) >= MENU_INACTIVE_IN_MSEC) {
			menuInactiveController();
			lastMiliSec = millis();
		}
	}

	//Rotary Encoder olvasása
	RotaryEncoderWrapper::RotaryEncoderResult rotaryEncoderResult = pRotaryEncoder->readRotaryEncoder();

	//Ha klikkeltek VAGY van irány, akkor a menüt piszkáljuk
	if (rotaryEncoderResult.clicked || rotaryEncoderResult.direction != RotaryEncoderWrapper::Direction::NONE) {
		menuController(rotaryEncoderResult.clicked, rotaryEncoderResult.direction);
		lastMiliSec = millis(); //volt aktivitás, a timer mutatóját utánahúzzuk
	}

	//Ha már nem vagyunk a menüben, és kell menteni a konfigot, akkor azt most tesszük meg
	if (lcdMenu->menuState == LcdMenu::OFF && pConfig->wantSaveConfig) {
		pConfig->save();
	}
}

