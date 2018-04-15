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
long lastMiliSec = -1;
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
 * Sipolás
 */
void buzzer(void) {
	tone(BUZZER_PIN, 1000);
	delay(500);
	tone(BUZZER_PIN, 800);
	delay(500);
	noTone(BUZZER_PIN);
}

/**
 * Riasztás
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
 */
void mainDisplayController() {
	if (millis() - lastMiliSec > 1000) {

		//Hõmérséklet lekérése -> csak egy DS18B20 mérõnk van -> 0 az index
		tempSensors.requestTemperaturesByIndex(MOT_TEMP_SENSOR_NDX);
		while (!tempSensors.isConversionComplete()) {
			delayMicroseconds(100);
		}
		float currentMotTemp = tempSensors.getTempCByIndex(MOT_TEMP_SENSOR_NDX);

		//Magas a hõmérséklet?
		if (currentMotTemp >= /*config->configVars.motTempAlarm*/50.0f) {
			lastMotTemp = currentMotTemp;
			drawWarningDisplay(currentMotTemp);
			buzzerAlarm();

		} else if (lastMotTemp == -1.0f || abs(lastMotTemp - currentMotTemp) > TEMP_DIFF_TO_DISPLAY) {
			//Csak az elsõ és a TEMP_DIFF_TO_DISPLAY-nál nagyobb eltérésekre reagálunk
			lastMotTemp = currentMotTemp;
			drawMainDisplay(currentMotTemp);
		}

		lastMiliSec = millis();
	}
}

//--- Menü -----------------------------------------------------------------------------------------------------------------------------------------------
/**
 * ClickEncoder service hívás
 * ~1msec-enként meg kell hívni, ezt a Timer1 interrupt végzi
 */
void rotaryEncoderServiceInterrupt(void) {
	rotaryEncoder->service();
}

int menuItem = 1;
int menuFrame = 1;
int menuPage = 1;
int lastMenuItem = 1;

#define MENU_PAGE_SIZE 	3	/* Menu elemekbõl ennyi látszik */
#define MENUITEMS_CNT 	7	/* Összes menuelemek száma */
String menuItems[MENUITEMS_CNT];

int volume = 50;

String language[3] = { "EN", "ES", "EL" };
int selectedLanguage = 0;

String difficulty[2] = { "EASY", "HARD" };
int selectedDifficulty = 0;

/**
 *
 */
void initMenuItems() {

	menuItems[0] = String("Contrast");
	menuItems[1] = String("Volume");
	menuItems[2] = String("Language");
	menuItems[3] = String("Difficulty");
	sprintf(tempBuff, "Light:%s", !config.configVars.boolBits.bits.blackLightState ? "ON" : "OFF");
	menuItems[4] = String(tempBuff);
	menuItems[5] = String("Reset");
	menuItems[6] = String("Exit");

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
 *
 */
void displayStringMenuPage(String menuItem, String value) {
	nokia5110Display.setTextSize(1);
	nokia5110Display.clearDisplay();
	nokia5110Display.setTextColor(BLACK, WHITE);
	nokia5110Display.setCursor(15, 0);
	nokia5110Display.print(menuItem);
	nokia5110Display.drawFastHLine(0, 10, 83, BLACK);
	nokia5110Display.setCursor(5, 15);
	nokia5110Display.print("Value");
	nokia5110Display.setTextSize(2);
	nokia5110Display.setCursor(5, 25);
	nokia5110Display.print(value);
	nokia5110Display.setTextSize(2);
	nokia5110Display.display();
}

/**
 *
 */
void displayIntMenuPage(String menuItem, int value) {
	displayStringMenuPage(menuItem, String(value));
}

/**
 *
 */
void drawMenu() {
#define LINE_1 15
#define LINE_2 25
#define LINE_3 35

	if (menuPage == 1) {
		nokia5110Display.setTextSize(1);
		nokia5110Display.clearDisplay();
		nokia5110Display.setTextColor(BLACK, WHITE);
		nokia5110Display.setCursor(15, 0);
		nokia5110Display.print("MAIN MENU");
		nokia5110Display.drawFastHLine(0, 10, 83, BLACK);

		if (menuItem == 1 && menuFrame == 1) {
			displayMenuItem(menuItems[0], LINE_1, true);
			displayMenuItem(menuItems[1], LINE_2, false);
			displayMenuItem(menuItems[2], LINE_3, false);
		} else if (menuItem == 2 && menuFrame == 1) {
			displayMenuItem(menuItems[0], LINE_1, false);
			displayMenuItem(menuItems[1], LINE_2, true);
			displayMenuItem(menuItems[2], LINE_3, false);
		} else if (menuItem == 3 && menuFrame == 1) {
			displayMenuItem(menuItems[0], LINE_1, false);
			displayMenuItem(menuItems[1], LINE_2, false);
			displayMenuItem(menuItems[2], LINE_3, true);

		} else if (menuItem == 4 && menuFrame == 2) {
			displayMenuItem(menuItems[1], LINE_1, false);
			displayMenuItem(menuItems[2], LINE_2, false);
			displayMenuItem(menuItems[3], LINE_3, true);
		} else if (menuItem == 3 && menuFrame == 2) {
			displayMenuItem(menuItems[1], LINE_1, false);
			displayMenuItem(menuItems[2], LINE_2, true);
			displayMenuItem(menuItems[3], LINE_3, false);
		} else if (menuItem == 2 && menuFrame == 2) {
			displayMenuItem(menuItems[1], LINE_1, true);
			displayMenuItem(menuItems[2], LINE_2, false);
			displayMenuItem(menuItems[3], LINE_3, false);

		} else if (menuItem == 5 && menuFrame == 3) {
			displayMenuItem(menuItems[2], LINE_1, false);
			displayMenuItem(menuItems[3], LINE_2, false);
			displayMenuItem(menuItems[4], LINE_3, true);

		} else if (menuItem == 6 && menuFrame == 4) {
			displayMenuItem(menuItems[3], LINE_1, false);
			displayMenuItem(menuItems[4], LINE_2, false);
			displayMenuItem(menuItems[5], LINE_3, true);

		} else if (menuItem == 5 && menuFrame == 4) {
			displayMenuItem(menuItems[3], LINE_1, false);
			displayMenuItem(menuItems[4], LINE_2, true);
			displayMenuItem(menuItems[5], LINE_3, false);
		} else if (menuItem == 4 && menuFrame == 4) {
			displayMenuItem(menuItems[3], LINE_1, true);
			displayMenuItem(menuItems[4], LINE_2, false);
			displayMenuItem(menuItems[5], LINE_3, false);
		} else if (menuItem == 3 && menuFrame == 3) {
			displayMenuItem(menuItems[2], LINE_1, true);
			displayMenuItem(menuItems[3], LINE_2, false);
			displayMenuItem(menuItems[4], LINE_3, false);
		} else if (menuItem == 2 && menuFrame == 2) {
			displayMenuItem(menuItems[1], LINE_1, true);
			displayMenuItem(menuItems[2], LINE_2, false);
			displayMenuItem(menuItems[3], LINE_3, false);
		} else if (menuItem == 4 && menuFrame == 3) {
			displayMenuItem(menuItems[2], LINE_1, false);
			displayMenuItem(menuItems[3], LINE_2, true);
			displayMenuItem(menuItems[4], LINE_3, false);
		}
		nokia5110Display.display();

	} else if (menuPage == 2 && menuItem == 1) {
		displayIntMenuPage(menuItems[0], config.configVars.contrast);
	} else if (menuPage == 2 && menuItem == 2) {
		displayIntMenuPage(menuItems[1], volume);
	} else if (menuPage == 2 && menuItem == 3) {
		displayStringMenuPage(menuItems[2], language[selectedLanguage]);
	} else if (menuPage == 2 && menuItem == 4) {
		displayStringMenuPage(menuItems[3], difficulty[selectedDifficulty]);
	} else if (menuPage == 2 && menuItem == 4) {
		displayStringMenuPage(menuItems[3], difficulty[selectedDifficulty]);
	}

}
bool inMenu = false;
/**
 * Menu kezelése
 */
void menuController() {

	rotaryEncoder->readRotaryEncoder();

	//ha nem vagyunk a menu-ben
	if (!inMenu) {
		//Ha nem klikkeltek ÉS nincs irány, akkor nem megyünk tovább
		if (!rotaryEncoder->isClicked() && rotaryEncoder->getDirection() == KY040RotaryEncoder::Direction::NONE) {
			return;
		}
	}

	inMenu = true;
	drawMenu();
	KY040RotaryEncoder::Direction direction = rotaryEncoder->getDirection();
	boolean isClicked = isClicked = rotaryEncoder->isClicked();

	bool changed = false;

	if (direction == KY040RotaryEncoder::Direction::UP) {
		if (menuPage == 1) {

			if (menuItem == 2 && menuFrame == 2) {
				menuFrame--;
			}

			if (menuItem == 4 && menuFrame == 4) {
				menuFrame--;
			}
			if (menuItem == 3 && menuFrame == 3) {
				menuFrame--;
			}
			lastMenuItem = menuItem;
			menuItem--;
			if (menuItem == 0) {
				menuItem = 1;
			}
		} else if (menuPage == 2 && menuItem == 1) {
			config.configVars.contrast++;
			nokia5110Display.setContrast(config.configVars.contrast);
			changed = true;
		} else if (menuPage == 2 && menuItem == 2) {
			volume++;
		} else if (menuPage == 2 && menuItem == 3) {
			selectedLanguage++;
			if (selectedLanguage == 3) {
				selectedLanguage = 0;
			}
		} else if (menuPage == 2 && menuItem == 4) {
			selectedDifficulty++;
			if (selectedDifficulty == 2) {
				selectedDifficulty = 0;
			}

		}
	} else if (direction == KY040RotaryEncoder::Direction::DOWN) {
		//We have turned the Rotary Encoder Clockwise
		if (menuPage == 1) {

			if (menuItem == 3 && lastMenuItem == 2) {
				menuFrame++;
			} else if (menuItem == 4 && lastMenuItem == 3) {
				menuFrame++;
			} else if (menuItem == 5 && lastMenuItem == 4 && menuFrame != 4) {
				menuFrame++;
			}
			lastMenuItem = menuItem;
			menuItem++;
			if (menuItem == 7) {
				menuItem--;
			}

		} else if (menuPage == 2 && menuItem == 1) {
			config.configVars.contrast--;
			nokia5110Display.setContrast(config.configVars.contrast);
			changed = true;
		} else if (menuPage == 2 && menuItem == 2) {
			volume--;
		} else if (menuPage == 2 && menuItem == 3) {

			selectedLanguage--;
			if (selectedLanguage == -1) {
				selectedLanguage = 2;
			}
		} else if (menuPage == 2 && menuItem == 4) {
			selectedDifficulty--;
			if (selectedDifficulty == -1) {
				selectedDifficulty = 1;
			}
		}
	} else if (isClicked) { //Middle Button is Pressed

		// Backlight Control
		if (menuPage == 1 && menuItem == 5) {
			config.configVars.boolBits.bits.blackLightState = !config.configVars.boolBits.bits.blackLightState;
			sprintf(tempBuff, "Light:%s", !config.configVars.boolBits.bits.blackLightState ? "ON" : "OFF");
			menuItems[4] = String(tempBuff);

			changed = true;
			nokia5110Display.setBlackLightState(config.configVars.boolBits.bits.blackLightState);
		}

		// Reset
		if (menuPage == 1 && menuItem == 6) {
			config.createDefaultConfig();
		} else if (menuPage == 1 && menuItem <= 4) {
			menuPage = 2;
		} else if (menuPage == 2) {
			menuPage = 1;
		}
	}

	//ha vátoztattak az értékeken, akkor mentünk egyet
	if (changed) {
		config.save();
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

	digitalWrite(WELD_LED_PIN, HIGH);

	digitalWrite(TRIAC_PIN, HIGH);	//triak be
	while (weldPeriodCnt <= config.configVars.preWeldPulseCnt) {
	}

	digitalWrite(TRIAC_PIN, LOW);	//triak ki
	weldPeriodCnt = 0;
	while (weldPeriodCnt <= config.configVars.pausePulseCnt) {
	}

	digitalWrite(TRIAC_PIN, HIGH);	//triak be
	weldPeriodCnt = 0;
	while (weldPeriodCnt <= config.configVars.weldPulseCnt) {
	}

	digitalWrite(TRIAC_PIN, LOW);	//triak ki

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
	nokia5110Display.setBlackLightState(config.configVars.boolBits.bits.blackLightState); //háttérvilágítás
	drawSplashScreen();

	//--- Rotary Encoder felhúzása
	rotaryEncoder = new KY040RotaryEncoder(A0, A1, A2);
	rotaryEncoder->setAccelerationEnabled(false);
	//rotaryEncoder->setDoubleClickEnabled(true);

	//--- ClickEncoder timer felhúzása
	Timer1.initialize(ROTARY_ENCODER_SERVICE_INTERVAL_IN_USEC);
	Timer1.attachInterrupt(rotaryEncoderServiceInterrupt);

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
	lastMiliSec = millis();

//	buzzer();
}

/**
 * Main loop
 */
void loop() {

	//--- Hegesztés kezelése -------------------------------------------------------------------
	//Kiolvassuk a weld button állapotát
	byte weldButtonCurrentState = digitalRead(WELD_BUTTON_PIN);

	// if the button state changes to pressed, remember the start time
	if (weldButtonCurrentState == HIGH && weldButtonPrevState == LOW && !isWelding) {
		weldButtonPushed();
	}
	weldButtonPrevState = weldButtonCurrentState;
	//------------------------------------------------------------------------------------------

	mainDisplayController();

	menuController();

}

