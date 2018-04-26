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
#include "RotaryEncoderAdapter.h"
#include "LcdMenu.h"
#include "Buzzer.h"

//Serial konzol debug ON
//#define SERIAL_DEBUG

// A h�l�zat frekvenci�ja Hz-ben
#define SYSTEM_FREQUENCY		50.0	/* Hz-ben */

//Dallas DS18B20 h�m�r� szenzor
#define MOT_TEMP_SENSOR_NDX 	0
#define TEMP_DIFF_TO_DISPLAY  	0.5f	/* f�l fokonk�nti elt�r�sekkel foglalkozunk csak */
float lastMotTemp = -1.0f;				//A MOT utols� m�rt h�m�rs�klete
OneWire oneWire(PIN_DALLAS18B20);
DallasTemperature tempSensors(&oneWire);

//--- Konfig support
Config *pConfig;

//--- Rotary Encoder
RotaryEncoderAdapter *pRotaryEncoder;

// --- Men�
//Men� inaktivit�si id� m�sodpercben
#define MENU_INACTIVE_IN_MSEC			(30 * 1000)
LcdMenu *lcdMenu;

// --- Buzzer
Buzzer *pBuzzer;

// -- Runtime adatok
long lastMiliSec = -1;					// h�m�rs�klet mm�r�sre, men� t�tlens�gi id� detekt�l�sa*/

//A konfigban megadott ventil�tor �C �rt�k el�tt ennyivel bekapcsolunk, vagy ennyivel ut�na kikapcsolunk
#define VENTILATOR_TRIGGER_OFFSET_VALUE	10

/**
 * Ventil�tor vez�rl�se
 */
void ventilatorController(float *currentMotTemp) {

	int triggerValue =  pConfig->configVars.motTempAlarm - VENTILATOR_TRIGGER_OFFSET_VALUE;

	if (triggerValue > *currentMotTemp) {
		if (digitalRead(PIN_VENTILATOR) == HIGH) {
			digitalWrite(PIN_VENTILATOR, LOW);
		}

	} else if (triggerValue <= *currentMotTemp) {
		if (digitalRead(PIN_VENTILATOR) == LOW) {
			digitalWrite(PIN_VENTILATOR, HIGH);
		}
	}
}

/**
 * Main display vez�rl�s
 * Ha riaszt�s van, akkor true-val t�r�nk vissza
 */
bool mainDisplayController() {

	bool highTemp = false;

	if ((millis() - lastMiliSec) > 1000) {

		//H�m�rs�klet lek�r�se -> csak egy DS18B20 m�r�nk van -> 0 az index
		tempSensors.requestTemperaturesByIndex(MOT_TEMP_SENSOR_NDX);
		while (!tempSensors.isConversionComplete()) {
			delayMicroseconds(100);
		}
		float currentMotTemp = tempSensors.getTempCByIndex(MOT_TEMP_SENSOR_NDX);

		//Magas a h�m�rs�klet?
		if (currentMotTemp >= pConfig->configVars.motTempAlarm) {
			lastMotTemp = currentMotTemp;
			lcdMenu->drawWarningDisplay(&currentMotTemp);
			pBuzzer->buzzerAlarm();
			highTemp = true;

		} else if (lastMotTemp == -1.0f || abs(lastMotTemp - currentMotTemp) > TEMP_DIFF_TO_DISPLAY) {
			//Csak az els� �s a TEMP_DIFF_TO_DISPLAY-n�l nagyobb elt�r�sekre reag�lunk
			lastMotTemp = currentMotTemp;
			lcdMenu->drawMainDisplay(&currentMotTemp);
		}

		lastMiliSec = millis();

		//Ventil�tor vez�rl�s
		ventilatorController(&currentMotTemp);
	}

	return highTemp;
}

/**
 * Elembe�ll�t� men� kontroller
 */
void itemMenuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	if (lcdMenu->menuState != LcdMenu::ITEM_MENU) {
		return;
	}

	//Kinyerj�k a kiv�lasztott men�elem pointer�t
	LcdMenu::MenuItemT p = lcdMenu->menuItems[lcdMenu->menuViewport.selectedItem];

	//Nem klikkeltek -> csak v�ltoztatj�k az elem �rt�k�t
	if (!rotaryClicked) {

		switch (rotaryDirection) {

			case RotaryEncoderAdapter::Direction::UP:

				switch (p.valueType) {
					case LcdMenu::BYTE:
					case LcdMenu::PULSE:
					case LcdMenu::TEMP:
						if (*(byte *) p.valuePtr < p.maxValue) {
							(*(byte *) p.valuePtr)++;
						}
						break;

					case LcdMenu::BOOL:
						if (!*(bool *) p.valuePtr) { //ha most false, akkor true-t csin�lunk bel�le
							*(bool *) p.valuePtr = true;
						}
						break;
				}
				break;

			case RotaryEncoderAdapter::Direction::DOWN:
				switch (p.valueType) {
					case LcdMenu::BYTE:
					case LcdMenu::PULSE:
					case LcdMenu::TEMP:
						if (*(byte *) p.valuePtr > p.minValue) {
							(*(byte *) p.valuePtr)--;
						}
						break;
					case LcdMenu::BOOL:
						if (*(bool *) p.valuePtr) { //ha most true, akkor false-t csin�lunk bel�le
							*(bool *) p.valuePtr = false;
						}
						break;
				}
				break;

			case RotaryEncoderAdapter::Direction::NONE:
				//Csak kirajzoltat�st k�rtek
				break;
		}

		//Menuelem be�l�t� k�perny� kirajzoltat�sa
		lcdMenu->drawMenuItemValue();

		//Ha van az almen�h�z hozz� callback, akkor azt is megh�vjuk
		if (p.callbackFunct != NULL) {
			(lcdMenu->*(p.callbackFunct))();
		}

		return;

	} //if (!rotaryClicked)

	//Klikkeltek az elem �rt�k�re -> kil�p�nk a men�elem-b�l a  f�men�be
	lcdMenu->menuState = LcdMenu::MAIN_MENU;
	lcdMenu->drawMainMenu();
}

/**
 * F�men� kontroller
 */
void mainMenuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	if (lcdMenu->menuState != LcdMenu::MAIN_MENU) {
		return;
	}

	//Nem klikkeltek -> csak tall�zunk a men�ben
	if (!rotaryClicked) {

		switch (rotaryDirection) {
			case RotaryEncoderAdapter::Direction::UP:

				//Az utols� elem a kiv�lasztott? Ha igen, akkor nem megy�nk tov�bb
				if (lcdMenu->menuViewport.selectedItem == LAST_MENUITEM_NDX) {
					return;
				}

				//A k�vetkez� men�elem lesz a kiv�lasztott
				lcdMenu->menuViewport.selectedItem++;

				//A viewport alj�n�l t�ljutottunk? Ha igen, akkor scrollozunk egyet lefel�
				if (lcdMenu->menuViewport.selectedItem > lcdMenu->menuViewport.lastItem) {
					lcdMenu->menuViewport.firstItem++;
					lcdMenu->menuViewport.lastItem++;
				}
				break;

			case RotaryEncoderAdapter::Direction::DOWN:

				//Az els� elem a kiv�lasztott? Ha igen, akkor nem megy�nk tov�bb
				if (lcdMenu->menuViewport.selectedItem == 0) {
					return;
				}

				//Az el�z� men�elem lesz a kiv�lasztott
				lcdMenu->menuViewport.selectedItem--;

				//A viewport alj�n�l t�ljutottunk? Ha igen, akkor scrollozunk egyet lefel�
				if (lcdMenu->menuViewport.selectedItem < lcdMenu->menuViewport.firstItem) {
					lcdMenu->menuViewport.firstItem--;
					lcdMenu->menuViewport.lastItem--;
				}
				break;

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
	LcdMenu::MenuItemT p = lcdMenu->menuItems[lcdMenu->menuViewport.selectedItem];

	//T�pus szerint megy�nk tov�bb
	switch (p.valueType) {
		//Ha ez egy �rt�kbe�ll�t� almen�
		case LcdMenu::BOOL:
		case LcdMenu::BYTE:
		case LcdMenu::PULSE:
		case LcdMenu::TEMP:
			lcdMenu->menuState = LcdMenu::ITEM_MENU;
			itemMenuController(false, RotaryEncoderAdapter::Direction::NONE); //K�r�nk egy men�elem be�ll�t� k�perny� kirajzol�st
			break;

			//Csak egy f�ggv�nyt kell h�vni, az majd elint�z mindent
		case LcdMenu::FUNCT:
			(lcdMenu->*(p.callbackFunct))();
			break;
	}
}

/**
 * Menu kezel�se
 */
void menuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	pBuzzer->buzzerMenu();
	switch (lcdMenu->menuState) {
		case LcdMenu::OFF: 	// Nem l�tszik a f�men� -> Ha kikkeltek, akkor bel�p�nk a m�be
			if (rotaryClicked) {
				lcdMenu->menuState = LcdMenu::MAIN_MENU;
				lcdMenu->drawMainMenu(); //Kirajzoltatjuk a f�men�t
			}
			break;

		case LcdMenu::MAIN_MENU: //L�tszik a f�men�
			mainMenuController(rotaryClicked, rotaryDirection);
			break;

		case LcdMenu::ITEM_MENU: //Elem v�ltoztat� men� l�tszik
			itemMenuController(rotaryClicked, rotaryDirection);
			break;
	}
}

/**
 * Men� inaktivit�s kezel�se
 */
void menuInactiveController() {
	switch (lcdMenu->menuState) {
		case LcdMenu::MAIN_MENU:
			lcdMenu->resetMenu(); //Kil�p�nk a men�b�l
			lcdMenu->menuState = LcdMenu::OFF;
			break;

		case LcdMenu::ITEM_MENU:
			lcdMenu->drawMainMenu(); //Kil�p�nk az almen�b�l
			lcdMenu->menuState = LcdMenu::MAIN_MENU;
	}
}
//--- Spot Welding ---------------------------------------------------------------------------------------------------------------------------------------
typedef enum weldState_t {
	PRE_WELD, PAUSE_WELD, WELD, WELD_END
} WeldState_T;
volatile WeldState_T weldCurrentState = WELD_END; //hegeszt�si �llapot jelz�
volatile uint16_t weldPeriodCnt = 0;	//Peri�dus Sz�ml�l�, hegeszt�s alatt megszak�t�skor inkrement�l�dik

/**
 * ZCD interrupt
 */
void zeroCrossDetect(void) {

	//Ha nincs pre (ez esetben m�r a pause sem �rdekel), akkor mehet egyb�l a weld
	if (weldCurrentState == PRE_WELD && pConfig->configVars.preWeldPulseCnt == 0) {
		weldPeriodCnt = 0;
		weldCurrentState = WELD;
	}

	switch (weldCurrentState) {

		case PRE_WELD: //A pre-ben vagyunk
			//A Triakot csak akkor kapcsoljuk be, ha van el�inpulzus sz�m a konfigban, �s nincs m�g bekapcsolva
			if (digitalRead(PIN_TRIAC) == LOW && pConfig->configVars.preWeldPulseCnt > 0) {
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

		case PAUSE_WELD: //A pause-ban vagyunk
			if (++weldPeriodCnt >= pConfig->configVars.pausePulseCnt) {
				weldPeriodCnt = 0;
				weldCurrentState = WELD;
			}
			break;

		case WELD:
			//bekapcsoljuk a triakot, ha m�g nincs bekapcsolva
			if (digitalRead(PIN_TRIAC) == LOW) {
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

	//interrupt ON
	weldCurrentState = PRE_WELD;
	sei();
	//Megv�rjuk a hegeszt�si folyamat v�g�t
	while (weldCurrentState != WELD_END) {
		delay(SLEEP_TIME_MSEC);
	}
	//interrupt OFF
	cli();

	digitalWrite(PIN_TRIAC, LOW);	//triak ki, csak a biztons�g kedv��rt
	digitalWrite(PIN_WELD_LED, LOW); //LED ki
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------

/**
 * Boot be�ll�t�sok
 */
void setup() {
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
	pRotaryEncoder = new RotaryEncoderAdapter(PIN_ENCODER_CLK, PIN_ENCODER_DT, PIN_ENCODER_SW);
	pRotaryEncoder->init();

	//Buzzer init
	pBuzzer = new Buzzer();

	//Software SPI (slower updates, more flexible pin options):
	lcdMenu = new LcdMenu();
	lcdMenu->drawSplashScreen();

	//Weld button PIN
	pinMode(PIN_WELD_BUTTON, INPUT);

	//--- ZCD Interrupt felh�z�sa
	pinMode(PIN_ZCD, INPUT);
	attachInterrupt(0, zeroCrossDetect, FALLING);

	//--- Triac PIN
	pinMode(PIN_TRIAC, OUTPUT);
	digitalWrite(PIN_TRIAC, LOW);

	//Weld LED PIN
	pinMode(PIN_WELD_LED, OUTPUT);
	digitalWrite(PIN_WELD_LED, LOW);

	//Ventil�tor
	pinMode(PIN_VENTILATOR, OUTPUT);
	digitalWrite(PIN_VENTILATOR, LOW);

	//--- H�m�r�s
	tempSensors.begin();

	//id�m�r�s indul
	lastMiliSec = millis();

	//kisz�m�tjuk a peri�dus id�t
	pConfig->spotWelderSystemPeriodTime = 1 / (float) SYSTEM_FREQUENCY;
}

/**
 * Main loop
 */
void loop() {

	static bool firstTime = true;	//els� fut�s jelz�se
	static byte weldButtonPrevState = LOW;   //A hegeszt�s gomb el�z� �llapota

	//--- Hegeszt�s kezel�se -------------------------------------------------------------------
	//Kiolvassuk a weld button �llapot�t
	byte weldButtonCurrentState = digitalRead(PIN_WELD_BUTTON);
	if (weldButtonCurrentState != weldButtonPrevState && weldButtonCurrentState == HIGH && weldButtonPrevState == LOW) {
		weldButtonPushed();
		lcdMenu->menuState = LcdMenu::OFF; //kil�p�nk majd a men�b�l, ha �pp benne voltunk
	}
	weldButtonPrevState = weldButtonCurrentState;

	//--- MainDisplay kezel�se ---
	if (lcdMenu->menuState == LcdMenu::OFF) {
		lastMotTemp = -1.0f; //kir�szakoljuk a mainScren ki�rat�s�t
		mainDisplayController();
	}

	//--- Men� kezel�se ---
	if (firstTime) {
		firstTime = false;
		pRotaryEncoder->SetChanged();   // force an update on active rotary
		pRotaryEncoder->readRotaryEncoder();   //els� kiolvas�st eldobjuk
	}

	RotaryEncoderAdapter::RotaryEncoderResult rotaryEncoderResult = pRotaryEncoder->readRotaryEncoder();
	//Ha klikkeltek VAGY van ir�ny, akkor a men�t piszk�ljuk
	if (rotaryEncoderResult.clicked || rotaryEncoderResult.direction != RotaryEncoderAdapter::Direction::NONE) {
		menuController(rotaryEncoderResult.clicked, rotaryEncoderResult.direction);
		//men�t�tlens�g 'reset'
		lastMiliSec = millis();
	}

	//Men� t�tlens�g figyel�se
	if (lcdMenu->menuState != LcdMenu::OFF && ((millis() - lastMiliSec) > MENU_INACTIVE_IN_MSEC)) {
		menuInactiveController();
		lastMiliSec = millis();
	}

	//Ha m�r nem vagyunk a men�ben, �s kell menteni valamit a konfigban, akkor azt most tessz�k meg
	if (lcdMenu->menuState == LcdMenu::OFF && pConfig->wantSaveConfig) {
		pConfig->save();
	}
}

