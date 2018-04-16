#include <Arduino.h>
#include <WString.h>

#include <TimerOne.h>
#include "KY040RotaryEncoder.h"
#include "Nokia5110Display.h"
#include "Config.h"

#include <OneWire.h>
#include <DallasTemperature.h>

//------------------- Konfig support
Config config;

//------------------- Hõmérés
#define DALLAS18B20_PIN			A3
#define MOT_TEMP_SENSOR_NDX 	0
#define TEMP_DIFF_TO_DISPLAY  	0.5f	/* fél fokonkénti eltérésekkel foglalkozunk csak */
float lastMotTemp = -1.0f;	//A MOT utolsó mért hõmérséklete
OneWire oneWire(DALLAS18B20_PIN);
DallasTemperature tempSensors(&oneWire);

//------------------- Menü support
KY040RotaryEncoder *rotaryEncoder;

// https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library/blob/master/examples/pcdtest/pcdtest.ino
//  Hardware SPI (faster, but must use certain hardware pins):
//
//   Arduino pin          Nokia 5110 PCD8544 LCD
//   ----------------------------------------------------
//    -                   8 GND
//    pin10 (D7)          7 BL - 3.3V BlackLight
//    -                   6 Vcc - 3.3V
//    pin16 (D13/SCK)     5 CLK - Serial clock (SCLK)
//    pin14 (D11/MOSI)    4 DIN - Serial data input DIN (SDIN)
//    pin8  (D5)          3 DC - Data/Command select (D/C)
//    pin7  (D4)          2 CE - Chip select (CS, SCE#)
//    pin6  (D3)          1 RST - Reset (RST, RES#)
//
//Adafruit_PCD8544 pcd8544Display = Adafruit_PCD8544(5, 4, 3); //Hardware SPI (faster, but must use certain hardware pins):
//
// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)

Nokia5110Display nokia5110Display = Nokia5110Display(8, 7, 6, 5, 4); //Software SPI (slower updates, more flexible pin options):
#define LCD_BLACKLIGHT_PIN 9

// Menü paraméterek (https://gist.github.com/Xplorer001/a829f8750d5df40b090645d63e2a187f)
#define MAX_ITEM_SIZE 10

// -- Runtime adatok
long lastMiliSecForTempMeasure = -1;
char tempBuff[64];
#define DEGREE_SYMBOL_CODE 247

//------------------- Spot welding paraméterek
//Zero Cross Detection PIN
#define ZCD_PIN PIN2

//Triac/MOC vezérlés PIN
#define TRIAC_PIN 12
#define WELD_LED_PIN 11

//Hegesztés gomb
#define WELD_BUTTON_PIN A4
byte weldButtonPrevState = LOW; //A hegesztés gomb elõzõ állapota

// Gomb prell/debounce védelem  https://www.arduino.cc/en/Tutorial/Debounce
#define BUTTON_DEBOUNCE_TIME 20

//Periódus Számláló, hegesztés alatt megszakításkor inkrementálódik
volatile int8_t weldPeriodCnt = 0;

//Hegesztésben vagyunk?
volatile boolean isWelding = false;

//------------------- Beeper
#define BUZZER_PIN 	13

//--- Buzzer ---------------------------------------------------------------------------------------------------------------------------------------------

/**
 * Sipolás - Ha engedélyezve van
 */
void buzzer(void) {

	if (!config.configVars.beepState) {
		return;
	}

	tone(BUZZER_PIN, 1000);
	delay(500);
	tone(BUZZER_PIN, 800);
	delay(500);
	noTone(BUZZER_PIN);
}

/**
 * Riasztási hang
 */
void buzzerAlarm(void) {
	tone(BUZZER_PIN, 1000);
	delay(300);
	tone(BUZZER_PIN, 1000);
	delay(300);
	tone(BUZZER_PIN, 1000);
	delay(300);
	noTone(BUZZER_PIN);
}

/**
 * menu hang
 */
void buzzerMenu() {
	tone(BUZZER_PIN, 800);
	delay(10);
	noTone(BUZZER_PIN);
}

//--- Fõképernyõ -----------------------------------------------------------------------------------------------------------------------------------------
/**
 * Splash képernyõ
 */
void drawSplashScreen(void) {
	nokia5110Display.begin();
	nokia5110Display.clearDisplay();
	nokia5110Display.setTextSize(1);
	nokia5110Display.setTextColor(BLACK);

	nokia5110Display.println(" Arduino Nano");
	nokia5110Display.println(" Spot Welder");

	nokia5110Display.setCursor(8, 25);
	nokia5110Display.print("v");

	nokia5110Display.setTextSize(2);
	sprintf(tempBuff, "%s", config.configVars.version);
	nokia5110Display.setCursor(14, 18);
	nokia5110Display.print(tempBuff);

	nokia5110Display.setTextSize(1);
	nokia5110Display.setCursor(0, 40);
	nokia5110Display.println(" BT-Soft 2018");

	nokia5110Display.display();
}

/**
 * Main screen
 */
void drawMainDisplay(float currentMotTemp) {

	nokia5110Display.clearDisplay();
	nokia5110Display.setTextColor(BLACK);
	nokia5110Display.setTextSize(1);

	nokia5110Display.println("PWld  Pse  Wld");
	sprintf(tempBuff, "%-3d   %-3d  %-3d", config.configVars.preWeldPulseCnt, config.configVars.pausePulseCnt, config.configVars.weldPulseCnt);
	nokia5110Display.println(tempBuff);

	nokia5110Display.print("\nMOT Temp");

	nokia5110Display.setTextSize(2);
	nokia5110Display.setCursor(18, 32);
	dtostrf(currentMotTemp, 1, 1, tempBuff);
	nokia5110Display.print(tempBuff);

	nokia5110Display.setTextSize(1);
	nokia5110Display.setCursor(68, 38);
	sprintf(tempBuff, "%cC", DEGREE_SYMBOL_CODE);
	nokia5110Display.print(tempBuff);

	nokia5110Display.display();
}

/**
 * Magas hõmérséklet risztás
 */
void drawWarningDisplay(float currentMotTemp) {
	nokia5110Display.clearDisplay();
	nokia5110Display.setTextColor(BLACK);
	nokia5110Display.setTextSize(1);

	nokia5110Display.println("!!!!!!!!!!!!!!");
	nokia5110Display.println(" MOT Temp is");
	nokia5110Display.println("  too high!");
	nokia5110Display.println("!!!!!!!!!!!!!!");

	nokia5110Display.setTextSize(2);
	nokia5110Display.setCursor(18, 32);
	dtostrf(currentMotTemp, 1, 1, tempBuff);
	nokia5110Display.print(tempBuff);

	nokia5110Display.setTextSize(1);
	nokia5110Display.setCursor(68, 38);
	sprintf(tempBuff, "%cC", DEGREE_SYMBOL_CODE);
	nokia5110Display.print(tempBuff);

	nokia5110Display.display();
}

/**
 * Main display vezérlés
 * Ha riasztás van, akkor true-val térünk vissza
 */
bool mainDisplayController() {

	bool alarmed = false;

	if (millis() - lastMiliSecForTempMeasure > 1000) {

		//Hõmérséklet lekérése -> csak egy DS18B20 mérõnk van -> 0 az index
		tempSensors.requestTemperaturesByIndex(MOT_TEMP_SENSOR_NDX);
		while (!tempSensors.isConversionComplete()) {
			delayMicroseconds(100);
		}
		float currentMotTemp = tempSensors.getTempCByIndex(MOT_TEMP_SENSOR_NDX);

		//Magas a hõmérséklet?
		if (currentMotTemp >= config.configVars.motTempAlarm) {
			lastMotTemp = currentMotTemp;
			drawWarningDisplay(currentMotTemp);
			buzzerAlarm();
			alarmed = true;

		} else if (lastMotTemp == -1.0f || abs(lastMotTemp - currentMotTemp) > TEMP_DIFF_TO_DISPLAY) {
			//Csak az elsõ és a TEMP_DIFF_TO_DISPLAY-nál nagyobb eltérésekre reagálunk
			lastMotTemp = currentMotTemp;
			drawMainDisplay(currentMotTemp);
		}

		lastMiliSecForTempMeasure = millis();
	}

	return alarmed;
}

//--- Menü -----------------------------------------------------------------------------------------------------------------------------------------------
//
/**
 * ClickEncoder service hívás
 * ~1msec-enként meg kell hívni, ezt a Timer1 interrupt végzi
 */
void rotaryEncoderTimer1ServiceInterrupt(void) {
	rotaryEncoder->service();
}

typedef enum MenuState_t {
	OFF,	//Nem látható
	MAIN_MENU, //Main menü látható
	ITEM_MENU //Elem Beállító menü látható
};

MenuState_t menuState = OFF;

typedef struct MenuViewport_t {
	byte firstItem;
	byte lastItem;
	byte selectedItem;
} MenuViewPortT;
MenuViewPortT menuViewport;
#define MENU_VIEWPORT_SIZE 	3	/* Menü elemekbõl ennyi látszik */

/* változtatható érték típusa */
typedef enum valueType_t {
	BOOL, BYTE, FUNCT
};

typedef void (*voidFuncPtr)(void);
typedef struct MenuItem_t {
	String title;			// Menüfelirat
	valueType_t valueType;	// Érték típus
	void *valuePtr;			// Az érték pointere
	uint8_t maxValue;		// Minimális numerikus érték
	uint8_t minValue;		// Maximális numerikus érték
	voidFuncPtr f; 			// Egyéb mûveletek függvény pointere, vagy NULL, ha nincs
} MenuItemT;
#define LAST_MENUITEM_NDX 	8 /* Az utolsó menüelem indexe, 0-tõl indul */
MenuItemT menuItems[LAST_MENUITEM_NDX + 1];

/**
 * Menü alapállapotba
 */
void resetMenu(void) {
	//viewPort beállítás
	menuViewport.firstItem = 0;
	menuViewport.lastItem = MENU_VIEWPORT_SIZE - 1;
	menuViewport.selectedItem = 0;
}

/**
 * String érték megjelenítése
 */
void displayStringMenuItemPage(String menuItem, String value) {
	nokia5110Display.clearDisplay();

	nokia5110Display.setTextSize(1);
	nokia5110Display.setTextColor(BLACK, WHITE);
	nokia5110Display.setCursor(5, 0);
	nokia5110Display.print(menuItem);
	nokia5110Display.drawFastHLine(0, 10, 83, BLACK);

	nokia5110Display.setCursor(5, 15);
	nokia5110Display.print("Value");

	nokia5110Display.setTextSize(2);
	nokia5110Display.setCursor(5, 25);
	nokia5110Display.print(value);
	nokia5110Display.display();
}

/**
 * Integer érték megjelenítése
 */
//--- Menüelemek callback függvényei
void menuLcdContrast(void) {
	nokia5110Display.setContrast(config.configVars.contrast);
}
void menuLcdBackLight(void) {
	nokia5110Display.setBlackLightState(config.configVars.blackLightState);
}
void menuBeepState(void) {
	if (config.configVars.beepState) {
		buzzerMenu();
	}
}
void menuFactoryReset(void) {
	config.createDefaultConfig();
	config.wantSaveConfig = true;

	nokia5110Display.clearDisplay();
	nokia5110Display.setTextSize(1);
	nokia5110Display.setTextColor(BLACK, WHITE);
	nokia5110Display.setCursor(15, 0);
	nokia5110Display.print("RESET");
	nokia5110Display.drawFastHLine(0, 10, 83, BLACK);
	nokia5110Display.setTextSize(2);
	nokia5110Display.setCursor(5, 25);
	nokia5110Display.print("OK");

	nokia5110Display.display();

	menuState = OFF;
	resetMenu();
}

void menuExit(void) {
	menuState = OFF; //Kilépünk a menübõl
	resetMenu();
}

/**
 * Menü felhúzása
 */
void initMenuItems(void) {

	menuItems[0] = {"PreWeld pulse", BYTE, (void*)&config.configVars.preWeldPulseCnt, 0, 255, NULL};
	menuItems[1] = {"Pause pulse", BYTE, (void*)&config.configVars.pausePulseCnt, 0, 255, NULL};
	menuItems[2] = {"Weld pulse", BYTE, (void*)&config.configVars.weldPulseCnt, 1, 255, NULL};
	menuItems[3] = {"MOT T.Alrm", BYTE, (void*)&config.configVars.motTempAlarm, 30, 120, NULL};
	menuItems[4] = {"Contrast", BYTE, (void*)&config.configVars.contrast, 0, 255, menuLcdContrast};
	menuItems[5] = {"Disp light", BOOL, (void*)&config.configVars.blackLightState, 0, 1, menuLcdBackLight};
	menuItems[6] = {"Beep", BOOL, (void*)&config.configVars.beepState, 0, 1, menuBeepState};
	menuItems[7] = {"Fctry reset", FUNCT, NULL, 0, 0, menuFactoryReset};
	menuItems[8] = {"Exit", FUNCT, NULL, 0, 0, menuExit};

	resetMenu();
}

/**
 *
 */
void displayMenuItem(String item, int position, boolean selected) {
	nokia5110Display.setCursor(0, position);

	if (selected) {
		nokia5110Display.setTextColor(WHITE, BLACK);
		nokia5110Display.print(">" + item);
	} else {
		nokia5110Display.setTextColor(BLACK, WHITE);
		nokia5110Display.print(" " + item);
	}
}

/**
 * MainMenu kirajzolása
 *  - Menüfelirat
 *  - viewportban látható elemek megjeelnítése
 *  - kiválasztott elem hátterének megváltoztatása
 */
void drawMainMenu(void) {
	const byte linePos[] = { 15, 25, 35 };

	nokia5110Display.clearDisplay();
	nokia5110Display.setTextSize(1);
	nokia5110Display.setTextColor(BLACK, WHITE);
	nokia5110Display.setCursor(15, 0);
	nokia5110Display.print("MAIN MENU");
	nokia5110Display.drawFastHLine(0, 10, 83, BLACK);

	for (byte i = 0; i < MENU_VIEWPORT_SIZE; i++) {
		byte itemNdx = menuViewport.firstItem + i;
		displayMenuItem(menuItems[itemNdx].title, linePos[i], itemNdx == menuViewport.selectedItem /* selected? */);
	}
	nokia5110Display.display();
}

/**
 *
 */
void drawMenuItem() {

	MenuItemT p = menuItems[menuViewport.selectedItem];

	//Típus szerinti kiírás
	switch (p.valueType) {

	case BOOL:
		displayStringMenuItemPage(p.title, *(bool *) p.valuePtr ? "ON" : "OFF");
		break;

	case BYTE:
		displayStringMenuItemPage(p.title, String(*(byte *) p.valuePtr));
		break;
	}
}

/**
 * Menu kezelése
 */
void menuController(bool rotaryClicked, KY040RotaryEncoder::Direction rotaryDirection) {

	//	buzzerMenu();

	//
	// Nem látszik a fõmenü -> Belépünk a mübe
	//
	if (menuState == OFF && rotaryClicked) {
		menuState = MAIN_MENU;
		drawMainMenu();
		return;
	}

	//
	// Látszik a fõmenü
	//
	else if (menuState == MAIN_MENU) {

		//Nem klikkeltek -> csak tallózunk a menüben
		if (!rotaryClicked) {

			if (rotaryDirection == KY040RotaryEncoder::Direction::UP) {

				//Az utolsó elem a kiválasztott? Ha igen, akkor nem megyünk tovább
				if (menuViewport.selectedItem == LAST_MENUITEM_NDX) {
					return;
				}

				//A következõ menüelem lesz a kiválasztott
				menuViewport.selectedItem++;

				//A viewport aljánál túljutottunk? Ha igen, akkor scrollozunk egyet lefelé
				if (menuViewport.selectedItem > menuViewport.lastItem) {
					menuViewport.firstItem++;
					menuViewport.lastItem++;
				}

			} else if (rotaryDirection == KY040RotaryEncoder::Direction::DOWN) {

				//Az elsõ elem a kiválasztott? Ha igen, akkor nem megyünk tovább
				if (menuViewport.selectedItem == 0) {
					return;
				}

				//Az elõzõ menüelem lesz a kiválasztott
				menuViewport.selectedItem--;

				//A viewport aljánál túljutottunk? Ha igen, akkor scrollozunk egyet lefelé
				if (menuViewport.selectedItem < menuViewport.firstItem) {
					menuViewport.firstItem--;
					menuViewport.lastItem--;
				}
			}

			drawMainMenu();

			return;
		} //if (!rotaryClicked)

		//
		// Klikkeltek a main menüben egy menüelemre  -> Kinyerjük a kiválasztott menüelem pointerét
		//
		MenuItemT p = menuItems[menuViewport.selectedItem];

		//Ez egy integer típusú menüelem?
		switch (p.valueType) {

		case BOOL:
		case BYTE:
			drawMenuItem();
			menuState = ITEM_MENU;
			break;

		case FUNCT: //függvényt kell hívni
			p.f();
			break;
		}

		return;

	} //if (menuState == MAIN_MENU)

	//
	//Elem változtató menü látszik
	//
	else if (menuState == ITEM_MENU) {

		//Kinyerjük a kiválasztott menüelem pointerét
		MenuItemT p = menuItems[menuViewport.selectedItem];

		//Nem klikkeltek -> csak változtatják az elem értékét
		if (!rotaryClicked) {

			if (rotaryDirection == KY040RotaryEncoder::Direction::UP) {
				if (p.valueType == BYTE) {
					if (*(byte *) p.valuePtr < p.maxValue) {
						*(byte *) p.valuePtr = (*(byte *) p.valuePtr) + 1;
					}
				} else if (p.valueType == BOOL) {
					if (!*(bool *) p.valuePtr) { //ha mos false, akkor true-t csinálunk belõle
						*(bool *) p.valuePtr = true;
					}
				}
			} else {
				if (p.valueType == BYTE) {
					if (*(byte *) p.valuePtr > p.minValue) {
						*(byte *) p.valuePtr = (*(byte *) p.valuePtr) - 1;
					}
				} else if (p.valueType == BOOL) {
					if (*(bool *) p.valuePtr) { //ha most true, akkor false-t csinálunk belõle
						*(bool *) p.valuePtr = false;
					}
				}
			}

			drawMenuItem();

			//Ha van az almenühöz hozzá callback, akkor meghívjuk
			if (p.f != NULL) {
				p.f();
			}
			return;

		} // if (menuState == ITEM_MENU)

		//Klikkeltek -> kilépünk a menüelem-bõl a  fõmenübe
		menuState == MAIN_MENU;
		drawMainMenu();
	}
}
//--- Spot Welding ---------------------------------------------------------------------------------------------------------------------------------------
/**
 * ZCD interrupt
 */
void zeroCrossDetect(void) {

	if (isWelding) {
		weldPeriodCnt++;
	}
}

/**
 * Hegesztési protokoll
 */
void weldButtonPushed(void) {
	weldPeriodCnt = 0;
	isWelding = true;

//LED-be
	digitalWrite(WELD_LED_PIN, HIGH);

//Ha engedélyezve van a kettõs impulzus
	if (config.configVars.preWeldPulseCnt > 0) {
		digitalWrite(TRIAC_PIN, HIGH);	//triak be
		while (weldPeriodCnt <= config.configVars.preWeldPulseCnt) {
			//NOP
		}
	}

	digitalWrite(TRIAC_PIN, LOW);	//triak ki
//Ha engedélyezve van a két hegesztési fázis közötti várakozás, akkor most várunk
//Ennek csak akkor van értelme, ha volt elsõ fázis
	if (config.configVars.preWeldPulseCnt > 0 && config.configVars.pausePulseCnt > 0) {
		weldPeriodCnt = 0;
		while (weldPeriodCnt <= config.configVars.pausePulseCnt) {
			//NOP
		}
	}

//Fõ hegesztési fázis
	digitalWrite(TRIAC_PIN, HIGH);	//triak be
	weldPeriodCnt = 0;
	while (weldPeriodCnt <= config.configVars.weldPulseCnt) {
		//NOP
	}

	digitalWrite(TRIAC_PIN, LOW);	//triak ki

//LED ki
	digitalWrite(WELD_LED_PIN, LOW);
	isWelding = false;
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------

/**
 * Boot beállítások
 */
void setup() {

//Konfig felolvasása
//config = new Config();
	config.read();

//Menüelemek inicializálása
	initMenuItems();

//--- Display
	nokia5110Display.setBlackLightPin(LCD_BLACKLIGHT_PIN);
	nokia5110Display.setContrast(config.configVars.contrast);	//kontraszt
	nokia5110Display.setBlackLightState(config.configVars.blackLightState);	//háttérvilágítás
	drawSplashScreen();

//--- Rotary Encoder felhúzása
	rotaryEncoder = new KY040RotaryEncoder(A0, A1, A2);
	rotaryEncoder->setAccelerationEnabled(false);
	rotaryEncoder->setDoubleClickEnabled(true);

//--- ClickEncoder timer felhúzása
	Timer1.initialize(ROTARY_ENCODER_SERVICE_INTERVAL_IN_USEC);
	Timer1.attachInterrupt(rotaryEncoderTimer1ServiceInterrupt);

//Weld button
	pinMode(WELD_BUTTON_PIN, INPUT);

//--- ZCD Interrupt felhúzása
	pinMode(ZCD_PIN, INPUT);
	attachInterrupt(0, zeroCrossDetect, FALLING);

//--- Triac
	pinMode(TRIAC_PIN, OUTPUT);
	digitalWrite(TRIAC_PIN, LOW);

//Weld LED
	pinMode(WELD_LED_PIN, OUTPUT);
	digitalWrite(WELD_LED_PIN, LOW);

//--- Hõmérés
	tempSensors.begin();

//idõmérés indul
	lastMiliSecForTempMeasure = millis();
}

/**
 * Main loop
 */
void loop() {

//--- Hegesztés kezelése -------------------------------------------------------------------
//Kiolvassuk a weld button állapotát
	byte weldButtonCurrentState = digitalRead(WELD_BUTTON_PIN);

	if (weldButtonCurrentState == HIGH && weldButtonPrevState == LOW && !isWelding) {
		weldButtonPushed();
		menuState = OFF; //kilépünk a menübõl, ha épp benne voltunk
	}
	weldButtonPrevState = weldButtonCurrentState;

//--- MainDisplay kezelése ---
	if (menuState == OFF) {
		lastMotTemp = -1.0f; //kirõszakoljuk a mainScren kiíratását
		mainDisplayController();
	}

	//--- Menü kezelése ---
	//Rotary encoder olvasása
	KY040RotaryEncoder::KY040RotaryEncoderResult rotaryEncoderResult = rotaryEncoder->readRotaryEncoder();

	//Ha klikkeltek VAGY van irány, akkor a menüt piszkáljuk
	if (rotaryEncoderResult.clicked || rotaryEncoderResult.direction != KY040RotaryEncoder::Direction::NONE) {
		menuController(rotaryEncoderResult.clicked, rotaryEncoderResult.direction);
	}

	//Ha már nem vagyunk a menüben, és kell menteni valamit a konfigban, akkor azt most tesszük meg
	if (menuState == OFF && config.wantSaveConfig) {
		config.save();
	}
}

