/*
 * NanoSpotWelder.cpp
 *
 *  Created on: 2018. �pr. 15.
 *      Author: BT
 */
#include <Arduino.h>
#include <WString.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "NanoSpotWederPinouts.h"
#include "Config.h"
#include "LcdMenu.h"
#include "Buzzer.h"
#include "RotaryEncoderWrapper.h"

//Serial konzol debug ON
//#define SERIAL_DEBUG

//Dallas DS18B20 h�m�r� szenzor
#define MOT_TEMP_SENSOR_NDX 	0
#define TEMP_DIFF_TO_DISPLAY  	0.5f	/* f�l fokonk�nti elt�r�sekkel foglalkozunk csak */
float lastMotTemp = -1.0f;				//A MOT utols� m�rt h�m�rs�klete
OneWire oneWire(PIN_DALLAS18B20);
DallasTemperature tempSensors(&oneWire);

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
}

/**
 * Main display vez�rl�s
 * Ha MOT h�m�rs�kleti riaszt�s van, akkor true-val t�r�nk vissza
 */
bool mainDisplayController(void) {

	bool highTempAlarm = false;

	//H�m�rs�klet lek�r�se -> csak egy DS18B20 m�r�nk van -> 0 az index
	tempSensors.requestTemperaturesByIndex(MOT_TEMP_SENSOR_NDX);
//		while (!tempSensors.isConversionComplete()) {
//			delayMicroseconds(100);
//		}
//
	float currentMotTemp = tempSensors.getTempCByIndex(MOT_TEMP_SENSOR_NDX);

#ifdef SERIAL_DEBUG
	Serial.print("MOT Temp: ");
	Serial.println(currentMotTemp);
#endif

	//Magas a h�m�rs�klet?
	if (currentMotTemp >= pConfig->configVars.motTempAlarm) {
		lastMotTemp = currentMotTemp;
		lcdMenu->drawWarningDisplay(&currentMotTemp);
		pBuzzer->buzzerAlarm();
		highTempAlarm = true;

	} else if (lastMotTemp == -1.0f || abs(lastMotTemp - currentMotTemp) > TEMP_DIFF_TO_DISPLAY) {
		//Csak az els� �s a TEMP_DIFF_TO_DISPLAY-n�l nagyobb elt�r�sekre reag�lunk
		lastMotTemp = currentMotTemp;
		lcdMenu->drawMainDisplay(&currentMotTemp);
	}

	//Ventil�tor vez�rl�s
	ventilatorController(&currentMotTemp);

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

	//Csippantunk
	pBuzzer->buzzerMenu();

	switch (lcdMenu->menuState) {

		// Nem l�tszik a f�men� -> Ha kikkeltek, akkor bel�p�nk a m�be
		case LcdMenu::OFF:
			if (rotaryClicked) {
				lcdMenu->menuState = LcdMenu::MAIN_MENU;
				lcdMenu->drawMainMenu(); //Kirajzoltatjuk a f�men�t
			}
			break;

			//L�tszik a f�men�
		case LcdMenu::MAIN_MENU:
			mainMenuController(rotaryClicked, rotaryDirection);
			break;

			//Elem v�ltoztat� men� l�tszik
		case LcdMenu::ITEM_MENU:
			itemMenuController(rotaryClicked, rotaryDirection);
			break;
	}
}

/**
 * Men� inaktivit�s kezel�se
 */
void menuInactiveController(void) {

#ifdef SERIAL_DEBUG
	Serial.println("menuInactiveController()");
#endif

	switch (lcdMenu->menuState) {

		//Main Men�ben vagyunk
		case LcdMenu::MAIN_MENU:
			lcdMenu->resetMenu(); //Kil�p�nk a men�b�l...
			lcdMenu->menuState = LcdMenu::OFF;
			lastMotTemp = -1.0f; //kier�szakoljuk a main display �jrarajzoltat�s�t
			//lastMiliSec = -1;
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
#define SLEEP_TIME_MSEC 50

	//LED-be
	digitalWrite(PIN_WELD_LED, HIGH);

	//Ha pulzussz�ml�l�s van
	if (pConfig->configVars.pulseCountWeldMode) {

		//R�k�lt�z�nk a ZCD interrupt-ra
		attachInterrupt(digitalPinToInterrupt(PIN_ZCD), zeroCrossDetect,
		FALLING);

		//Be�ll�tjuk, hogy a PRE_WELD �llapotb�l induljunk
		weldCurrentState = PRE_WELD;

		//Megv�rjuk a hegeszt�si folyamat v�g�t
		while (weldCurrentState != WELD_END) {
			delay(SLEEP_TIME_MSEC);
		}

		//Lesz�llunk a ZCD interrupt-r�l
		detachInterrupt(digitalPinToInterrupt(PIN_ZCD));

	} else { //K�zi hegeszt�s vez�rl�s van

		digitalWrite(PIN_TRIAC, HIGH); //TRIAC BE

		//Addig am�g a gomb le van nyomva, addig nem mozdulunk innen
		while (digitalRead(PIN_WELD_BUTTON)) {
			delay(SLEEP_TIME_MSEC * 2);
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
	Serial.begin(9600);
	while (!Serial)
		;
	Serial.println("Debug active");
#endif

	//Konfig felolvas�sa
	pConfig = new Config();
	pConfig->read();

	//Rotary encoder init
	pRotaryEncoder = new RotaryEncoderWrapper(PIN_ENCODER_CLK, PIN_ENCODER_DT,
	PIN_ENCODER_SW);
	pRotaryEncoder->init();

	//Buzzer init
	pBuzzer = new Buzzer();

	//Software SPI (slower updates, more flexible pin options):
	lcdMenu = new LcdMenu();
	lcdMenu->drawSplashScreen();

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

	//H�m�r�s init
	tempSensors.begin();

	//id�m�r�s indul
	lastMiliSec = millis();

	//kisz�m�tjuk a peri�dus id�t
	pConfig->spotWelderSystemPeriodTime = 1 / (float) SYSTEM_FREQUENCY;
}

/**
 * Main loop
 */
void loop(void) {

	static bool firstTime = true;	//els� fut�s jelz�se
	static byte weldButtonPrevState = LOW;   //A hegeszt�s gomb el�z� �llapota

	//--- Indul�si be�ll�t�sok
	if (firstTime) {
		firstTime = false;
		pRotaryEncoder->SetChanged();   // force an update on active rotary
		pRotaryEncoder->readRotaryEncoder();   //els� kiolvas�st eldobjuk
	}

	//
	// --- Hegeszt�s kezel�se -------------------------------------------------------------------
	//

	//Kiolvassuk a weld button �llapot�t
	byte weldButtonCurrentState = digitalRead(PIN_WELD_BUTTON);

	//Ha v�ltozott az �llapot LOW -> HIGH ir�nyban
	if (weldButtonCurrentState != weldButtonPrevState && weldButtonCurrentState == HIGH && weldButtonPrevState == LOW) {
		weldButtonPushed();
		lcdMenu->menuState = LcdMenu::OFF; //kil�p�nk majd a men�b�l, ha �pp benne voltunk
	}
	//Eltessz�k az aktu�lis button �llapotot
	weldButtonPrevState = weldButtonCurrentState;

	//
	//--- MainDisplay kezel�se --------------------------------------------------------------------
	//
	if(lcdMenu->menuState == LcdMenu::FORCE_MAIN_DISPLAY){
		lcdMenu->menuState =LcdMenu::MAIN_MENU;
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

