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
 *  Created on: 2018. �pr. 15.
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

//MOT h�m�rs�kletm�r�s
#define TEMP_DIFF_TO_DISPLAY  			0.5f	/* f�l fokonk�nti elt�r�sekkel foglalkozunk csak */
MOTTemp *pMOTTemp;

//--- Konfig support
Config *pConfig;

//--- Rotary Encoder
RotaryEncoderWrapper *pRotaryEncoder;

// --- Men�
//M�sodpercenk�nt n�z�nk r� a MOT h�m�rs�klet�re
#define MAIN_DISPLAY_LOOP_TIME			1000

//Men� inaktivit�si id� m�sodpercben
#define MENU_INACTIVE_IN_MSEC			(30 /* Men� t�tlens�gi id� limit mp-ben*/ * MAIN_DISPLAY_LOOP_TIME)
LcdMenu *lcdMenu;

// --- Buzzer
Buzzer *pBuzzer;

// -- Runtime adatok
long lastMiliSec = 0;				// h�m�rs�klet mm�r�sre, men� t�tlens�gi id� detekt�l�sa*/

//A konfigban megadott ventil�tor riaszt�si �C �rt�k el�tt ennyivel bekapcsolunk, vagy ennyivel ut�na kikapcsolunk
#define VENTILATOR_TRIGGER_OFFSET_VALUE	10

/**
 * Ventil�tor vez�rl�se
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
 * Main display vez�rl�s
 * Ha MOT h�m�rs�kleti riaszt�s van, akkor true-val t�r�nk vissza
 */
bool mainDisplayController(void) {

	bool highTempAlarm = false;

	float currentMotTemp = pMOTTemp->getTemperature();

	//Magas a h�m�rs�klet?
	if (currentMotTemp >= pConfig->configVars.motTempAlarm) {
		pMOTTemp->lastMotTemp = currentMotTemp;
		lcdMenu->drawWarningDisplay(&currentMotTemp);
		pBuzzer->buzzerAlarm();
		highTempAlarm = true;

	} else if (pMOTTemp->lastMotTemp == -1.0f || abs(pMOTTemp->lastMotTemp - currentMotTemp) > TEMP_DIFF_TO_DISPLAY) {
		//Csak az els� �s a TEMP_DIFF_TO_DISPLAY-n�l nagyobb elt�r�sekre reag�lunk
		pMOTTemp->lastMotTemp = currentMotTemp;
		lcdMenu->drawMainDisplay(&currentMotTemp);
	}

	//Ventil�tor vez�rl�s
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
 * Elembe�ll�t� men� kontroller
 */
void itemMenuController(bool rotaryClicked, RotaryEncoderWrapper::Direction rotaryDirection) {

	if (lcdMenu->menuState != LcdMenu::ITEM_MENU) {
		return;
	}

	//Nem klikkeltek -> csak v�ltoztatj�k az elem �rt�k�t
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

				//Csak kirajzoltat�st k�rtek
			case RotaryEncoderWrapper::Direction::NONE:
				break;
		}

		//Men�elem be�l�t� k�perny� kirajzoltat�sa
		lcdMenu->drawMenuItemValue();

		//Ha van az almen�h�z hozz� callback, akkor azt is megh�vjuk
		lcdMenu->invokeMenuItemCallBackFunct();

		return;

	} //if (!rotaryClicked)

	//Klikkeltek az elem �rt�k�re -> kil�p�nk a men�elem-b�l a  f�men�be
	lcdMenu->menuState = LcdMenu::MAIN_MENU;
	lcdMenu->drawMainMenu();
}

/**
 * F�men� kontroller
 */
void mainMenuController(bool rotaryClicked, RotaryEncoderWrapper::Direction rotaryDirection) {

	if (lcdMenu->menuState != LcdMenu::MAIN_MENU) {
		return;
	}

	//Nem klikkeltek -> csak tall�zunk a men�ben
	if (!rotaryClicked) {

		switch (rotaryDirection) {

			//Felfel� tekertek
			case RotaryEncoderWrapper::Direction::UP:
				lcdMenu->stepDown();
				break;

				//Lefel� tekertek
			case RotaryEncoderWrapper::Direction::DOWN:
				lcdMenu->stepUp();
				break;

				//Nincs ir�ny
			default:
				return;
		}

		lcdMenu->drawMainMenu();
		return;

	} //if (!rotaryClicked)

	//
	// Klikkeltek a main men�ben egy men�elemre
	// - Kinyerj�k a kiv�lasztott men�elem pointer�t
	// - Kirajzoltatjuk az elem �rt�k�t
	// - �t�ll�tjuk az �llapotott az elembe�ll�t�sra
	//

	//T�pus szerint megy�nk tov�bb
	switch (lcdMenu->getSelectedItemPtr()->valueType) {

		//Ha ez egy �rt�kbe�ll�t� almen�
		//Csak egy f�ggv�nyt kell h�vni, az majd elint�z mindent
		case LcdMenu::FUNCT:
			lcdMenu->invokeMenuItemCallBackFunct();
			break;

		default:
			lcdMenu->menuState = LcdMenu::ITEM_MENU;
			itemMenuController(false, RotaryEncoderWrapper::Direction::NONE); //K�r�nk egy men�elem be�ll�t� k�perny� kirajzol�st
			break;
	}
}

/**
 * Menu kezel�se
 */
void menuController(bool rotaryClicked, RotaryEncoderWrapper::Direction rotaryDirection) {

	switch (lcdMenu->menuState) {

		// Nem l�tszik a f�men� -> Ha kikkeltek, akkor bel�p�nk a m�be
		case LcdMenu::OFF:
			if (rotaryClicked) {
				pBuzzer->buzzerMenu();
				lcdMenu->menuState = LcdMenu::MAIN_MENU;
				lcdMenu->drawMainMenu(); //Kirajzoltatjuk a f�men�t
			}
			break;

			//L�tszik a f�men�
		case LcdMenu::MAIN_MENU:
			pBuzzer->buzzerMenu();
			mainMenuController(rotaryClicked, rotaryDirection);
			break;

			//Elem v�ltoztat� men� l�tszik
		case LcdMenu::ITEM_MENU:
			pBuzzer->buzzerMenu();
			itemMenuController(rotaryClicked, rotaryDirection);
			break;
	}
}

/**
 * Men� inaktivit�s kezel�se
 */
void menuInactiveController(void) {

	switch (lcdMenu->menuState) {

		//Main Men�ben vagyunk
		case LcdMenu::MAIN_MENU:
			lcdMenu->resetMenu(); //Kil�p�nk a men�b�l...
			lcdMenu->menuState = LcdMenu::OFF;
			pMOTTemp->lastMotTemp = -1.0f; //kier�szakoljuk a main display �jrarajzoltat�s�t
			break;

			//Elembe�ll�t� men�ben vagyunk
		case LcdMenu::ITEM_MENU:
			lcdMenu->drawMainMenu(); //Kil�p�nk az almen�b�l a f�men�be
			lcdMenu->menuState = LcdMenu::MAIN_MENU;
	}
}

//--- Spot Welding ---------------------------------------------------------------------------------------------------------------------------------------
typedef enum weldState_t {
	PRE_WELD, 		//El�impulzus
	PAUSE_WELD,  	//Sz�net a k�t impulzus k�z�tt
	WELD, 			//hegeszt� impulzus
	WELD_END 		//Nincs hegeszt�s
} WeldState_T;

volatile WeldState_T weldCurrentState = WELD_END; //hegeszt�si �llapot jelz�
volatile uint16_t weldPeriodCnt = 0; //Peri�dus Sz�ml�l�, hegeszt�s alatt megszak�t�skor inkrement�l�dik

/**
 * ZCD interrupt rutin
 */
void zeroCrossDetect(void) {

	//Ha nincs pre (ez esetben m�r a pause sem �rdekel), akkor mehet egyb�l a weld
	if (weldCurrentState == PRE_WELD && pConfig->configVars.preWeldPulseCnt == 0) {
		weldPeriodCnt = 0;
		weldCurrentState = WELD;
	}

	switch (weldCurrentState) {

		//A pre-ben vagyunk
		case PRE_WELD:
			//A Triakot csak akkor kapcsoljuk be, ha van el�inpulzus sz�m a konfigban, �s nincs m�g bekapcsolva
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

			//A f� hegeszt�sben vagyunk
		case WELD:
			//bekapcsoljuk a triakot, ha m�g nincs bekapcsolva
			if (!digitalRead(PIN_TRIAC)) {
				weldPeriodCnt = 0;
				digitalWrite(PIN_TRIAC, HIGH); //TRIAC BE
				return;
			}

			//Ha el�rt�k a pulzussz�mot, akkor kikapcsolunk
			if (++weldPeriodCnt >= pConfig->configVars.weldPulseCnt) {
				digitalWrite(PIN_TRIAC, LOW); //TRIAC KI
				weldCurrentState = WELD_END;
			}
			break;

			//Hegeszt�s v�ge
		case WELD_END:
			//Igazib�l itt m�r nem csin�lunk semmit sem
			digitalWrite(PIN_TRIAC, LOW);	//triak ki, csak a biztons�g kedv��rt
			break;
	}
}

/**
 * Hegeszt�si protokoll
 */
void weldButtonPushed(void) {
#define SLEEP_TIME_MICROSEC 100

	//LED-be
	digitalWrite(PIN_WELD_LED, HIGH);

	//Ha pulzussz�ml�l�s van
	if (pConfig->configVars.pulseCountWeldMode) {

		//R�k�lt�z�nk a ZCD interrupt-ra
		attachInterrupt(digitalPinToInterrupt(PIN_ZCD), zeroCrossDetect, FALLING);

		//Be�ll�tjuk, hogy a PRE_WELD �llapotb�l induljunk
		weldCurrentState = PRE_WELD;

		//Megv�rjuk a hegeszt�si folyamat v�g�t
		while (weldCurrentState != WELD_END) {
			delayMicroseconds(SLEEP_TIME_MICROSEC);
		}

		//Lesz�llunk a ZCD interrupt-r�l
		detachInterrupt(digitalPinToInterrupt(PIN_ZCD));

	} else { //K�zi hegeszt�s vez�rl�s van

		digitalWrite(PIN_TRIAC, HIGH); //TRIAC BE

		//Addig am�g a gomb le van nyomva, addig nem mozdulunk innen
		while (digitalRead(PIN_WELD_BUTTON)) {
			delayMicroseconds(SLEEP_TIME_MICROSEC * 20);
		}

		digitalWrite(PIN_TRIAC, LOW); //TRIAC KI
	}

	{ //Csak a biztons�g kedv��rt...
		digitalWrite(PIN_TRIAC, LOW);	//TRIAC ki
		weldCurrentState = WELD_END;    //Nincs hegeszt�s �llapot
	}

	digitalWrite(PIN_WELD_LED, LOW); //LED ki
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------

/**
 * Boot be�ll�t�sok
 */
void setup(void) {
#ifdef SERIAL_DEBUG
	Serial.begin(SERIAL_BAUD_RATE);
	while (!Serial)
	;
	Serial.println("Debug active");
#endif

	//Konfig felolvas�sa
	pConfig = new Config();
	pConfig->read();

	//Rotary encoder init
	pRotaryEncoder = new RotaryEncoderWrapper(PIN_ENCODER_CLK, PIN_ENCODER_DT, PIN_ENCODER_SW);
	pRotaryEncoder->init();
	pRotaryEncoder->SetChanged();   // kier�szakoljuk az �llapot kiolvashat�s�g�t
	pRotaryEncoder->readRotaryEncoder();   //els� kiolvas�st eldobjuk

	//Buzzer init
	pBuzzer = new Buzzer();

	//Software SPI (slower updates, more flexible pin options):
	lcdMenu = new LcdMenu();
	lcdMenu->drawSplashScreen();

	pMOTTemp = new MOTTemp();

	//Weld button PIN
	pinMode(PIN_WELD_BUTTON, INPUT);

	//--- ZCD input felh�z�sa
	pinMode(PIN_ZCD, INPUT_PULLUP);

	//--- Triac PIN
	pinMode(PIN_TRIAC, OUTPUT);
	digitalWrite(PIN_TRIAC, LOW);

	//--- Weld LED PIN
	pinMode(PIN_WELD_LED, OUTPUT);
	digitalWrite(PIN_WELD_LED, LOW);

	//--- Ventil�tor
	pinMode(PIN_VENTILATOR, OUTPUT);
	digitalWrite(PIN_VENTILATOR, LOW);

	//id�m�r�s indul
	lastMiliSec = millis();
}

/**
 * Main loop
 */
void loop(void) {

	static byte weldButtonPrevState = HIGH;   //A hegeszt�s gomb el�z� �llapota

	//
	// --- Hegeszt�s kezel�se -------------------------------------------------------------------
	//

	//Kiolvassuk a weld button �llapot�t
	byte weldButtonCurrentState = digitalRead(PIN_WELD_BUTTON);

	//Ha v�ltozott az �llapot LOW -> HIGH ir�nyban
	if (weldButtonCurrentState != weldButtonPrevState && weldButtonCurrentState == HIGH && weldButtonPrevState == LOW) {
		weldButtonPushed();
		lcdMenu->menuState = LcdMenu::OFF; //kil�p�nk majd a men�b�l, ha �pp benne voltunk
		delay(100); //weld button debounce -> r�vid impulzus csomagok eset�n j�l j�het
	}
	//Eltessz�k az aktu�lis button �llapotot
	weldButtonPrevState = weldButtonCurrentState;

	//
	//--- MainDisplay kezel�se --------------------------------------------------------------------
	//
	if (lcdMenu->menuState == LcdMenu::FORCE_MAIN_DISPLAY) {
		lcdMenu->menuState = LcdMenu::MAIN_MENU;
		menuInactiveController();
	} else if (lcdMenu->menuState == LcdMenu::OFF) {
		//Men� t�tlens�g figyel�se
		if (millis() - lastMiliSec >= MAIN_DISPLAY_LOOP_TIME) {
			mainDisplayController();
			lastMiliSec = millis();
		}
	} else { //Valamilyen men�ben vagyunk
		//Men�t�tlens�g figyel�se
		if ((millis() - lastMiliSec) >= MENU_INACTIVE_IN_MSEC) {
			menuInactiveController();
			lastMiliSec = millis();
		}
	}

	//Rotary Encoder olvas�sa
	RotaryEncoderWrapper::RotaryEncoderResult rotaryEncoderResult = pRotaryEncoder->readRotaryEncoder();

	//Ha klikkeltek VAGY van ir�ny, akkor a men�t piszk�ljuk
	if (rotaryEncoderResult.clicked || rotaryEncoderResult.direction != RotaryEncoderWrapper::Direction::NONE) {
		menuController(rotaryEncoderResult.clicked, rotaryEncoderResult.direction);
		lastMiliSec = millis(); //volt aktivit�s, a timer mutat�j�t ut�nah�zzuk
	}

	//Ha m�r nem vagyunk a men�ben, �s kell menteni a konfigot, akkor azt most tessz�k meg
	if (lcdMenu->menuState == LcdMenu::OFF && pConfig->wantSaveConfig) {
		pConfig->save();
	}
}

