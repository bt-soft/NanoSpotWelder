/*
 * NanoSpotWelder.cpp
 *
 *  Created on: 2018. ápr. 15.
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

// A hálózat frekvenciája Hz-ben
#define SYSTEM_FREQUENCY		50.0	/* Hz-ben */

//Dallas DS18B20 hõmérõ szenzor
#define MOT_TEMP_SENSOR_NDX 	0
#define TEMP_DIFF_TO_DISPLAY  	0.5f	/* fél fokonkénti eltérésekkel foglalkozunk csak */
float lastMotTemp = -1.0f;				//A MOT utolsó mért hõmérséklete
OneWire oneWire(PIN_DALLAS18B20);
DallasTemperature tempSensors(&oneWire);

//--- Konfig support
Config *pConfig;

//--- Rotary Encoder
RotaryEncoderAdapter *pRotaryEncoder;

// --- Menü
//Menü inaktivitási idõ másodpercben
#define MENU_INACTIVE_IN_MSEC			(30 * 1000)
LcdMenu *lcdMenu;

// --- Buzzer
Buzzer *pBuzzer;

// -- Runtime adatok
long lastMiliSec = -1;					// hõmérséklet mmérésre, menü tétlenségi idõ detektálása*/

//A konfigban megadott ventilátor °C érték elõtt ennyivel bekapcsolunk, vagy ennyivel utána kikapcsolunk
#define VENTILATOR_TRIGGER_OFFSET_VALUE	10

/**
 * Ventilátor vezérlése
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
 * Main display vezérlés
 * Ha riasztás van, akkor true-val térünk vissza
 */
bool mainDisplayController() {

	bool highTemp = false;

	if ((millis() - lastMiliSec) > 1000) {

		//Hõmérséklet lekérése -> csak egy DS18B20 mérõnk van -> 0 az index
		tempSensors.requestTemperaturesByIndex(MOT_TEMP_SENSOR_NDX);
		while (!tempSensors.isConversionComplete()) {
			delayMicroseconds(100);
		}
		float currentMotTemp = tempSensors.getTempCByIndex(MOT_TEMP_SENSOR_NDX);

		//Magas a hõmérséklet?
		if (currentMotTemp >= pConfig->configVars.motTempAlarm) {
			lastMotTemp = currentMotTemp;
			lcdMenu->drawWarningDisplay(&currentMotTemp);
			pBuzzer->buzzerAlarm();
			highTemp = true;

		} else if (lastMotTemp == -1.0f || abs(lastMotTemp - currentMotTemp) > TEMP_DIFF_TO_DISPLAY) {
			//Csak az elsõ és a TEMP_DIFF_TO_DISPLAY-nál nagyobb eltérésekre reagálunk
			lastMotTemp = currentMotTemp;
			lcdMenu->drawMainDisplay(&currentMotTemp);
		}

		lastMiliSec = millis();

		//Ventilátor vezérlés
		ventilatorController(&currentMotTemp);
	}

	return highTemp;
}

/**
 * Elembeállító menü kontroller
 */
void itemMenuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	if (lcdMenu->menuState != LcdMenu::ITEM_MENU) {
		return;
	}

	//Kinyerjük a kiválasztott menüelem pointerét
	LcdMenu::MenuItemT p = lcdMenu->menuItems[lcdMenu->menuViewport.selectedItem];

	//Nem klikkeltek -> csak változtatják az elem értékét
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
						if (!*(bool *) p.valuePtr) { //ha most false, akkor true-t csinálunk belõle
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
						if (*(bool *) p.valuePtr) { //ha most true, akkor false-t csinálunk belõle
							*(bool *) p.valuePtr = false;
						}
						break;
				}
				break;

			case RotaryEncoderAdapter::Direction::NONE:
				//Csak kirajzoltatást kértek
				break;
		}

		//Menuelem beálító képernyõ kirajzoltatása
		lcdMenu->drawMenuItemValue();

		//Ha van az almenühöz hozzá callback, akkor azt is meghívjuk
		if (p.callbackFunct != NULL) {
			(lcdMenu->*(p.callbackFunct))();
		}

		return;

	} //if (!rotaryClicked)

	//Klikkeltek az elem értékére -> kilépünk a menüelem-bõl a  fõmenübe
	lcdMenu->menuState = LcdMenu::MAIN_MENU;
	lcdMenu->drawMainMenu();
}

/**
 * Fõmenü kontroller
 */
void mainMenuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	if (lcdMenu->menuState != LcdMenu::MAIN_MENU) {
		return;
	}

	//Nem klikkeltek -> csak tallózunk a menüben
	if (!rotaryClicked) {

		switch (rotaryDirection) {
			case RotaryEncoderAdapter::Direction::UP:

				//Az utolsó elem a kiválasztott? Ha igen, akkor nem megyünk tovább
				if (lcdMenu->menuViewport.selectedItem == LAST_MENUITEM_NDX) {
					return;
				}

				//A következõ menüelem lesz a kiválasztott
				lcdMenu->menuViewport.selectedItem++;

				//A viewport aljánál túljutottunk? Ha igen, akkor scrollozunk egyet lefelé
				if (lcdMenu->menuViewport.selectedItem > lcdMenu->menuViewport.lastItem) {
					lcdMenu->menuViewport.firstItem++;
					lcdMenu->menuViewport.lastItem++;
				}
				break;

			case RotaryEncoderAdapter::Direction::DOWN:

				//Az elsõ elem a kiválasztott? Ha igen, akkor nem megyünk tovább
				if (lcdMenu->menuViewport.selectedItem == 0) {
					return;
				}

				//Az elõzõ menüelem lesz a kiválasztott
				lcdMenu->menuViewport.selectedItem--;

				//A viewport aljánál túljutottunk? Ha igen, akkor scrollozunk egyet lefelé
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
	// Klikkeltek a main menüben egy menüelemre
	// - Kinyerjük a kiválasztott menüelem pointerét
	// - Kirajzoltatjuk az elem értékét
	// - Átállítjuk az állapotott az elembeállításra
	//
	LcdMenu::MenuItemT p = lcdMenu->menuItems[lcdMenu->menuViewport.selectedItem];

	//Típus szerint megyünk tovább
	switch (p.valueType) {
		//Ha ez egy értékbeállító almenü
		case LcdMenu::BOOL:
		case LcdMenu::BYTE:
		case LcdMenu::PULSE:
		case LcdMenu::TEMP:
			lcdMenu->menuState = LcdMenu::ITEM_MENU;
			itemMenuController(false, RotaryEncoderAdapter::Direction::NONE); //Kérünk egy menüelem beállító képernyõ kirajzolást
			break;

			//Csak egy függvényt kell hívni, az majd elintéz mindent
		case LcdMenu::FUNCT:
			(lcdMenu->*(p.callbackFunct))();
			break;
	}
}

/**
 * Menu kezelése
 */
void menuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	pBuzzer->buzzerMenu();
	switch (lcdMenu->menuState) {
		case LcdMenu::OFF: 	// Nem látszik a fõmenü -> Ha kikkeltek, akkor belépünk a mübe
			if (rotaryClicked) {
				lcdMenu->menuState = LcdMenu::MAIN_MENU;
				lcdMenu->drawMainMenu(); //Kirajzoltatjuk a fõmenüt
			}
			break;

		case LcdMenu::MAIN_MENU: //Látszik a fõmenü
			mainMenuController(rotaryClicked, rotaryDirection);
			break;

		case LcdMenu::ITEM_MENU: //Elem változtató menü látszik
			itemMenuController(rotaryClicked, rotaryDirection);
			break;
	}
}

/**
 * Menü inaktivitás kezelése
 */
void menuInactiveController() {
	switch (lcdMenu->menuState) {
		case LcdMenu::MAIN_MENU:
			lcdMenu->resetMenu(); //Kilépünk a menübõl
			lcdMenu->menuState = LcdMenu::OFF;
			break;

		case LcdMenu::ITEM_MENU:
			lcdMenu->drawMainMenu(); //Kilépünk az almenübõl
			lcdMenu->menuState = LcdMenu::MAIN_MENU;
	}
}
//--- Spot Welding ---------------------------------------------------------------------------------------------------------------------------------------
typedef enum weldState_t {
	PRE_WELD, PAUSE_WELD, WELD, WELD_END
} WeldState_T;
volatile WeldState_T weldCurrentState = WELD_END; //hegesztési állapot jelzõ
volatile uint16_t weldPeriodCnt = 0;	//Periódus Számláló, hegesztés alatt megszakításkor inkrementálódik

/**
 * ZCD interrupt
 */
void zeroCrossDetect(void) {

	//Ha nincs pre (ez esetben már a pause sem érdekel), akkor mehet egybõl a weld
	if (weldCurrentState == PRE_WELD && pConfig->configVars.preWeldPulseCnt == 0) {
		weldPeriodCnt = 0;
		weldCurrentState = WELD;
	}

	switch (weldCurrentState) {

		case PRE_WELD: //A pre-ben vagyunk
			//A Triakot csak akkor kapcsoljuk be, ha van elõinpulzus szám a konfigban, és nincs még bekapcsolva
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
			//bekapcsoljuk a triakot, ha még nincs bekapcsolva
			if (digitalRead(PIN_TRIAC) == LOW) {
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
#define SLEEP_TIME_MSEC 50

	//LED-be
	digitalWrite(PIN_WELD_LED, HIGH);

	//interrupt ON
	weldCurrentState = PRE_WELD;
	sei();
	//Megvárjuk a hegesztési folyamat végét
	while (weldCurrentState != WELD_END) {
		delay(SLEEP_TIME_MSEC);
	}
	//interrupt OFF
	cli();

	digitalWrite(PIN_TRIAC, LOW);	//triak ki, csak a biztonság kedvéért
	digitalWrite(PIN_WELD_LED, LOW); //LED ki
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------

/**
 * Boot beállítások
 */
void setup() {
#ifdef SERIAL_DEBUG
	Serial.begin(9600);
	while (!Serial)
	;
	Serial.println("Debug active");
#endif

	//Konfig felolvasása
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

	//--- ZCD Interrupt felhúzása
	pinMode(PIN_ZCD, INPUT);
	attachInterrupt(0, zeroCrossDetect, FALLING);

	//--- Triac PIN
	pinMode(PIN_TRIAC, OUTPUT);
	digitalWrite(PIN_TRIAC, LOW);

	//Weld LED PIN
	pinMode(PIN_WELD_LED, OUTPUT);
	digitalWrite(PIN_WELD_LED, LOW);

	//Ventilátor
	pinMode(PIN_VENTILATOR, OUTPUT);
	digitalWrite(PIN_VENTILATOR, LOW);

	//--- Hõmérés
	tempSensors.begin();

	//idõmérés indul
	lastMiliSec = millis();

	//kiszámítjuk a periódus idõt
	pConfig->spotWelderSystemPeriodTime = 1 / (float) SYSTEM_FREQUENCY;
}

/**
 * Main loop
 */
void loop() {

	static bool firstTime = true;	//elsõ futás jelzése
	static byte weldButtonPrevState = LOW;   //A hegesztés gomb elõzõ állapota

	//--- Hegesztés kezelése -------------------------------------------------------------------
	//Kiolvassuk a weld button állapotát
	byte weldButtonCurrentState = digitalRead(PIN_WELD_BUTTON);
	if (weldButtonCurrentState != weldButtonPrevState && weldButtonCurrentState == HIGH && weldButtonPrevState == LOW) {
		weldButtonPushed();
		lcdMenu->menuState = LcdMenu::OFF; //kilépünk majd a menübõl, ha épp benne voltunk
	}
	weldButtonPrevState = weldButtonCurrentState;

	//--- MainDisplay kezelése ---
	if (lcdMenu->menuState == LcdMenu::OFF) {
		lastMotTemp = -1.0f; //kirõszakoljuk a mainScren kiíratását
		mainDisplayController();
	}

	//--- Menü kezelése ---
	if (firstTime) {
		firstTime = false;
		pRotaryEncoder->SetChanged();   // force an update on active rotary
		pRotaryEncoder->readRotaryEncoder();   //elsõ kiolvasást eldobjuk
	}

	RotaryEncoderAdapter::RotaryEncoderResult rotaryEncoderResult = pRotaryEncoder->readRotaryEncoder();
	//Ha klikkeltek VAGY van irány, akkor a menüt piszkáljuk
	if (rotaryEncoderResult.clicked || rotaryEncoderResult.direction != RotaryEncoderAdapter::Direction::NONE) {
		menuController(rotaryEncoderResult.clicked, rotaryEncoderResult.direction);
		//menütétlenség 'reset'
		lastMiliSec = millis();
	}

	//Menü tétlenség figyelése
	if (lcdMenu->menuState != LcdMenu::OFF && ((millis() - lastMiliSec) > MENU_INACTIVE_IN_MSEC)) {
		menuInactiveController();
		lastMiliSec = millis();
	}

	//Ha már nem vagyunk a menüben, és kell menteni valamit a konfigban, akkor azt most tesszük meg
	if (lcdMenu->menuState == LcdMenu::OFF && pConfig->wantSaveConfig) {
		pConfig->save();
	}
}

