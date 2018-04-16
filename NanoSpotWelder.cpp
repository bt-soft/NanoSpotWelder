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

//------------------- H�m�r�s
#define DALLAS18B20_PIN			A3
#define MOT_TEMP_SENSOR_NDX 	0
#define TEMP_DIFF_TO_DISPLAY  	0.5f	/* f�l fokonk�nti elt�r�sekkel foglalkozunk csak */
float lastMotTemp = -1.0f;	//A MOT utols� m�rt h�m�rs�klete
OneWire oneWire(DALLAS18B20_PIN);
DallasTemperature tempSensors(&oneWire);

//------------------- Men� support
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

// Men� param�terek (https://gist.github.com/Xplorer001/a829f8750d5df40b090645d63e2a187f)
#define MAX_ITEM_SIZE 10

// -- Runtime adatok
long lastMiliSec = -1;
char tempBuff[64];
#define DEGREE_SYMBOL_CODE 247

//------------------- Spot welding param�terek
//Zero Cross Detection PIN
#define ZCD_PIN PIN2

//Triac/MOC vez�rl�s PIN
#define TRIAC_PIN 12
#define WELD_LED_PIN 11

//Hegeszt�s gomb
#define WELD_BUTTON_PIN A4
byte weldButtonPrevState = LOW; //A hegeszt�s gomb el�z� �llapota

// Gomb prell/debounce v�delem  https://www.arduino.cc/en/Tutorial/Debounce
#define BUTTON_DEBOUNCE_TIME 20

//Peri�dus Sz�ml�l�, hegeszt�s alatt megszak�t�skor inkrement�l�dik
volatile int8_t weldPeriodCnt = 0;

//Hegeszt�sben vagyunk?
volatile boolean isWelding = false;

//------------------- Beeper
#define BUZZER_PIN 	13

//--- Buzzer ---------------------------------------------------------------------------------------------------------------------------------------------

/**
 * Sipol�s - Ha enged�lyezve van
 */
void buzzer(void) {

	if (!config.configVars.bits.beepState) {
		return;
	}

	tone(BUZZER_PIN, 1000);
	delay(500);
	tone(BUZZER_PIN, 800);
	delay(500);
	noTone(BUZZER_PIN);
}

/**
 * Riaszt�si hang
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
void buzzerMenu(){
	tone(BUZZER_PIN, 800);
	delay(10);
	noTone(BUZZER_PIN);
}


//--- F�k�perny� -----------------------------------------------------------------------------------------------------------------------------------------
/**
 * Splash k�perny�
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
 * Magas h�m�rs�klet riszt�s
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
 * Main display vez�rl�s
 * Ha riaszt�s van, akkor true-val t�r�nk vissza
 */
bool mainDisplayController() {

	bool alarmed = false;

	if (millis() - lastMiliSec > 1000) {

		//H�m�rs�klet lek�r�se -> csak egy DS18B20 m�r�nk van -> 0 az index
		tempSensors.requestTemperaturesByIndex(MOT_TEMP_SENSOR_NDX);
		while (!tempSensors.isConversionComplete()) {
			delayMicroseconds(100);
		}
		float currentMotTemp = tempSensors.getTempCByIndex(MOT_TEMP_SENSOR_NDX);

		//Magas a h�m�rs�klet?
		if (currentMotTemp >= /*config->configVars.motTempAlarm*/50.0f) {
			lastMotTemp = currentMotTemp;
			drawWarningDisplay(currentMotTemp);
			buzzerAlarm();
			alarmed = true;

		} else if (lastMotTemp == -1.0f || abs(lastMotTemp - currentMotTemp) > TEMP_DIFF_TO_DISPLAY) {
			//Csak az els� �s a TEMP_DIFF_TO_DISPLAY-n�l nagyobb elt�r�sekre reag�lunk
			lastMotTemp = currentMotTemp;
			drawMainDisplay(currentMotTemp);
		}

		lastMiliSec = millis();
	}

	return alarmed;
}

//--- Men� -----------------------------------------------------------------------------------------------------------------------------------------------
//
//
//
//

bool isInMenu = false;
typedef struct MenuViewport_t {
	byte firstItem;
	byte lastItem;
	byte selectedItem;
} MenuViewPort;
MenuViewPort menuViewport;

#define MENU_VIEWPORT_SIZE 	3	/* Men� elemekb�l ennyi l�tszik */
#define LAST_MENUITEM_NDX 	6	/* Az utols� men�elem indexe, 0-t�l indul */
String menuItems[LAST_MENUITEM_NDX + 1];

int volume = 50;

String language[] = { "EN", "ES", "EL" };
int selectedLanguage = 0;

String difficulty[] = { "EASY", "HARD" };
int selectedDifficulty = 0;

/**
 *
 */
void initMenuItems() {

	menuItems[0] = String("Contrast");
	menuItems[1] = String("Volume");
	menuItems[2] = String("Language");
	menuItems[3] = String("Difficulty");
	sprintf(tempBuff, "Light:%s", !config.configVars.bits.blackLightState ? "ON" : "OFF");
	menuItems[4] = String(tempBuff);
	menuItems[5] = String("Reset");
	menuItems[6] = String("Exit");

	//viewPort be�ll�t�s
	menuViewport.firstItem = 0;
	menuViewport.lastItem = MENU_VIEWPORT_SIZE - 1;
	menuViewport.selectedItem = 0;
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
 * String �rt�k megjelen�t�se
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
 * Integer �rt�k megjelen�t�se
 */
void displayIntMenuPage(String menuItem, int value) {
	displayStringMenuPage(menuItem, String(value));
}

/**
 *
 */
void drawMenu() {
	const byte linePos[] = { 15, 25, 35 };

	nokia5110Display.clearDisplay();
	nokia5110Display.setTextSize(1);
	nokia5110Display.setTextColor(BLACK, WHITE);
	nokia5110Display.setCursor(15, 0);
	nokia5110Display.print("MAIN MENU");
	nokia5110Display.drawFastHLine(0, 10, 83, BLACK);

	for (byte i = 0; i < MENU_VIEWPORT_SIZE; i++) {
		byte itemNdx = menuViewport.firstItem + i;
		displayMenuItem(menuItems[itemNdx], linePos[i], itemNdx == menuViewport.selectedItem /* selected? */);
	}
	nokia5110Display.display();
}

/**
 * Menu kezel�se
 */
void menuController(bool rotaryClicked, KY040RotaryEncoder::Direction rotaryDirection) {

	//Csak a forgat�ssal foglalkozunk
	if (!rotaryClicked) {

		if (rotaryDirection == KY040RotaryEncoder::Direction::UP) {

			//Az utols� elem a kiv�lasztott? Ha igen, akkor nem megy�nk tov�bb
			if (menuViewport.selectedItem == LAST_MENUITEM_NDX) {
				return;
			}

			//A k�vetkez� men�elem lesz a kiv�lasztott
			menuViewport.selectedItem++;

			//A viewport alj�n�l t�ljutottunk? Ha igen, akkor scrollozunk egyet lefel�
			if (menuViewport.selectedItem > menuViewport.lastItem) {
				menuViewport.firstItem++;
				menuViewport.lastItem++;
			}

		} else if (rotaryDirection == KY040RotaryEncoder::Direction::DOWN) {

			//Az els� elem a kiv�lasztott? Ha igen, akkor nem megy�nk tov�bb
			if (menuViewport.selectedItem == 0) {
				return;
			}

			//Az el�z� men�elem lesz a kiv�lasztott
			menuViewport.selectedItem--;

			//A viewport alj�n�l t�ljutottunk? Ha igen, akkor scrollozunk egyet lefel�
			if (menuViewport.selectedItem < menuViewport.firstItem) {
				menuViewport.firstItem--;
				menuViewport.lastItem--;
			}

		}
	} else {
		//Kikk esem�nyek feldolgoz�sa
		if(menuViewport.selectedItem  == LAST_MENUITEM_NDX){ //exit?
			isInMenu = false; //Kil�p�nk a men�b�l
			return;
		}
	}


	drawMenu();

//	bool changed = false;
//
//	if (direction == KY040RotaryEncoder::Direction::UP) {
//		if (menuPage == 1) {
//
//			if (selectedMenuItem == 2 && menuFrame == 2) {
//				menuFrame--;
//			}
//
//			if (selectedMenuItem == 4 && menuFrame == 4) {
//				menuFrame--;
//			}
//			if (selectedMenuItem == 3 && menuFrame == 3) {
//				menuFrame--;
//			}
//			lastMenuItem = selectedMenuItem;
//			selectedMenuItem--;
//			if (selectedMenuItem == 0) {
//				selectedMenuItem = 1;
//			}
//		} else if (menuPage == 2 && selectedMenuItem == 1) {
//			config.configVars.contrast++;
//			nokia5110Display.setContrast(config.configVars.contrast);
//			changed = true;
//		} else if (menuPage == 2 && selectedMenuItem == 2) {
//			volume++;
//		} else if (menuPage == 2 && selectedMenuItem == 3) {
//			selectedLanguage++;
//			if (selectedLanguage == 3) {
//				selectedLanguage = 0;
//			}
//		} else if (menuPage == 2 && selectedMenuItem == 4) {
//			selectedDifficulty++;
//			if (selectedDifficulty == 2) {
//				selectedDifficulty = 0;
//			}
//
//		}
//	} else if (direction == KY040RotaryEncoder::Direction::DOWN) {
//		//We have turned the Rotary Encoder Clockwise
//		if (menuPage == 1) {
//
//			if (selectedMenuItem == 3 && lastMenuItem == 2) {
//				menuFrame++;
//			} else if (selectedMenuItem == 4 && lastMenuItem == 3) {
//				menuFrame++;
//			} else if (selectedMenuItem == 5 && lastMenuItem == 4 && menuFrame != 4) {
//				menuFrame++;
//			}
//			lastMenuItem = selectedMenuItem;
//			selectedMenuItem++;
//			if (selectedMenuItem == 7) {
//				selectedMenuItem--;
//			}
//
//		} else if (menuPage == 2 && selectedMenuItem == 1) {
//			config.configVars.contrast--;
//			nokia5110Display.setContrast(config.configVars.contrast);
//			changed = true;
//		} else if (menuPage == 2 && selectedMenuItem == 2) {
//			volume--;
//		} else if (menuPage == 2 && selectedMenuItem == 3) {
//
//			selectedLanguage--;
//			if (selectedLanguage == -1) {
//				selectedLanguage = 2;
//			}
//		} else if (menuPage == 2 && selectedMenuItem == 4) {
//			selectedDifficulty--;
//			if (selectedDifficulty == -1) {
//				selectedDifficulty = 1;
//			}
//		}
//	} else if (isClicked) { //Middle Button is Pressed
//
//		// Backlight Control
//		if (menuPage == 1 && selectedMenuItem == 5) {
//			config.configVars.boolBits.bits.blackLightState = !config.configVars.boolBits.bits.blackLightState;
//			sprintf(tempBuff, "Light:%s", !config.configVars.boolBits.bits.blackLightState ? "ON" : "OFF");
//			menuItems[4] = String(tempBuff);
//
//			changed = true;
//			nokia5110Display.setBlackLightState(config.configVars.boolBits.bits.blackLightState);
//		}
//
//		// Reset
//		if (menuPage == 1 && selectedMenuItem == 6) {
//			config.createDefaultConfig();
//		} else if (menuPage == 1 && selectedMenuItem <= 4) {
//			menuPage = 2;
//		} else if (menuPage == 2) {
//			menuPage = 1;
//		}
//	}
//
//ha v�toztattak az �rt�keken, akkor ment�nk egyet
//	if (changed) {
//		config.save();
//	}
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
 * Hegeszt�si protokoll
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

/**
 * ClickEncoder service h�v�s
 * ~1msec-enk�nt meg kell h�vni, ezt a Timer1 interrupt v�gzi
 */
void rotaryEncoderTimer1ServiceInterrupt(void) {
	rotaryEncoder->service();
}

//--------------------------------------------------------------------------------------------------------------------------------------------------------

/**
 * Boot be�ll�t�sok
 */
void setup() {

	//Konfig felolvas�sa
	//config = new Config();
	config.read();

	//Men�elemek inicializ�l�sa
	initMenuItems();

	//--- Display
	nokia5110Display.setBlackLightPin(LCD_BLACKLIGHT_PIN);
	nokia5110Display.setContrast(config.configVars.contrast);	//kontraszt
	nokia5110Display.setBlackLightState(config.configVars.bits.blackLightState);	//h�tt�rvil�g�t�s
	drawSplashScreen();

	//--- Rotary Encoder felh�z�sa
	rotaryEncoder = new KY040RotaryEncoder(A0, A1, A2);
	rotaryEncoder->setAccelerationEnabled(false);
	rotaryEncoder->setDoubleClickEnabled(true);

	//--- ClickEncoder timer felh�z�sa
	Timer1.initialize(ROTARY_ENCODER_SERVICE_INTERVAL_IN_USEC);
	Timer1.attachInterrupt(rotaryEncoderTimer1ServiceInterrupt);

	//Weld button
	pinMode(WELD_BUTTON_PIN, INPUT);

	//--- ZCD Interrupt felh�z�sa
	pinMode(ZCD_PIN, INPUT);
	attachInterrupt(0, zeroCrossDetect, FALLING);

	//--- Triac
	pinMode(TRIAC_PIN, OUTPUT);
	digitalWrite(TRIAC_PIN, LOW);

	//Weld LED
	pinMode(WELD_LED_PIN, OUTPUT);
	digitalWrite(WELD_LED_PIN, LOW);

	//--- H�m�r�s
	tempSensors.begin();

	//id�m�r�s indul
	lastMiliSec = millis();

	//Beepel�nk, ha enged�lyezve van
	buzzer();
}

/**
 * Main loop
 */
void loop() {

	//--- Hegeszt�s kezel�se -------------------------------------------------------------------
	//Kiolvassuk a weld button �llapot�t
	byte weldButtonCurrentState = digitalRead(WELD_BUTTON_PIN);

	// if the button state changes to pressed, remember the start time
	if (weldButtonCurrentState == HIGH && weldButtonPrevState == LOW && !isWelding) {
		weldButtonPushed();
	}
	weldButtonPrevState = weldButtonCurrentState;
	//------------------------------------------------------------------------------------------

	if (!isInMenu) {
		lastMotTemp = -1.0f; //kir�szakoljuk a mainScren ki�rat�s�t
		mainDisplayController();
	}

	//Rotary encoder olvas�sa
	KY040RotaryEncoder::KY040RotaryEncoderResult rotaryEncoderResult = rotaryEncoder->readRotaryEncoder();
	//delay(510);

	//Ha klikkeltek VAGY van ir�ny, akkor a men�t piszk�ljuk
	if (rotaryEncoderResult.clicked || rotaryEncoderResult.direction != KY040RotaryEncoder::Direction::NONE) {
		buzzerMenu();

		//klikkre l�p�nk be a men�be
		if(rotaryEncoderResult.clicked && !isInMenu){
			isInMenu = true;
		}
		menuController(rotaryEncoderResult.clicked, rotaryEncoderResult.direction);
	}
}

