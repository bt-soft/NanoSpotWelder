#include <Arduino.h>
#include <WString.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "Config.h"
#include "Nokia5110Display.h"
#include "RotaryEncoderAdapter.h"

//Serial konzol debug ON
//#define SERIAL_DEBUG

//------------------- Konfig support
Config config;

//------------------- H�m�r�s
#define DALLAS18B20_PIN			A2
#define MOT_TEMP_SENSOR_NDX 	0
#define TEMP_DIFF_TO_DISPLAY  	0.5f	/* f�l fokonk�nti elt�r�sekkel foglalkozunk csak */
float lastMotTemp = -1.0f;				//A MOT utols� m�rt h�m�rs�klete
OneWire oneWire(DALLAS18B20_PIN);
DallasTemperature tempSensors(&oneWire);

//------------------- Men� support
//Rotary Encoder
/*
 * KY-040- Rotary Encoder
 *
 *
 * https://www.google.hu/imgres?imgurl=https%3A%2F%2Fi0.wp.com%2Fhenrysbench.capnfatz.com%2Fwp-content%2Fuploads%2F2015%2F05%2FKeyes-KY-040-Rotary-Encoder-Pin-Outs.png&imgrefurl=http%3A%2F%2Fhenrysbench.capnfatz.com%2Fhenrys-bench%2Farduino-sensors-and-input%2Fkeyes-ky-040-arduino-rotary-encoder-user-manual%2F&docid=_c4mGtu-_4T-7M&tbnid=C4A0RLRlbinkiM%3A&vet=10ahUKEwjKlcq8lqvaAhWNLlAKHSK5Bs4QMwh8KDgwOA..i&w=351&h=109&bih=959&biw=1920&q=ky-040%20rotary%20encoder%20simulation%20proteus&ved=0ahUKEwjKlcq8lqvaAhWNLlAKHSK5Bs4QMwh8KDgwOA&iact=mrc&uact=8
 *  +--------------+
 *  |          CLK |-- Encoder pin A
 *  |           DT |-- Encoder pin B
 *  |           SW |-- PushButton SW
 *  |         +VCC |-- +5V
 *  |          GND |-- Encoder pin C
 *  +--------------+
 *
 * https://github.com/0xPIT/encoder/issues/7
 */

#define ENCODER_CLK        3   		/* This pin must have a minimum 0.47 uF capacitor */
#define ENCODER_DT         A0      /* data pin */
#define ENCODER_SW         A1      /* switch pin (active LOW) */
RotaryEncoderAdapter rotaryEncoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW);

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
#define LCD_BLACKLIGHT_PIN 		9

// -- Runtime adatok
long lastMiliSec = -1;					// h�m�rs�klet mm�r�sre, men� t�tlens�gi id� detekt�l�sa*/
char tempBuff[64];
#define DEGREE_SYMBOL_CODE 		247		/* Az LCD-n a '�' jel k�dja */

//------------------- Spot welding param�terek
#define ZCD_PIN 				2		/* Zero Cross Detection PIN, megszak�t�sban */
#define TRIAC_PIN 				12 		/* D12 Triac/MOC vez�rl�s PIN */
#define WELD_LED_PIN 			11		/* D11 Hegeszt�s LED visszajelz�s */
#define WELD_BUTTON_PIN 		10 		/* D10 Hegeszt�s gomb */

//Peri�dus Sz�ml�l�, hegeszt�s alatt megszak�t�skor inkrement�l�dik
volatile uint16_t weldPeriodCnt = 0;

//M�rt h�l�zati frekvencia
float spotWelderSystemPeriodTime = 0.0;

//------------------- Beeper
#define BUZZER_PIN 				13

//--- --------------- Buzzer

/**
 * Sipol�s - Ha enged�lyezve van
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
void buzzerMenu() {
	tone(BUZZER_PIN, 800);
	delay(10);
	noTone(BUZZER_PIN);
}

//------------------------- Menu
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
 * H�m�rs�klet ki�r�sa a main- �s az alarm screen-n�l
 */
void drawTempValue(float currentMotTemp) {
	nokia5110Display.setTextSize(2);
	nokia5110Display.setCursor(18, 32);
	dtostrf(currentMotTemp, 1, 1, tempBuff);
	nokia5110Display.print(tempBuff);

	nokia5110Display.setTextSize(1);
	nokia5110Display.setCursor(68, 38);
	sprintf(tempBuff, "%cC", DEGREE_SYMBOL_CODE);
	nokia5110Display.print(tempBuff);
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

	//H�m�rs�klet ki�r�sa
	nokia5110Display.print("\nMOT Temp");
	drawTempValue(currentMotTemp);

	nokia5110Display.display();
}

/**
 * Magas h�m�rs�klet riaszt�s
 */
void drawWarningDisplay(float currentMotTemp) {
	nokia5110Display.clearDisplay();
	nokia5110Display.setTextColor(BLACK);
	nokia5110Display.setTextSize(1);

	nokia5110Display.println("!!!!!!!!!!!!!!");
	nokia5110Display.println(" MOT Temp is");
	nokia5110Display.println("  too high!");
	nokia5110Display.println("!!!!!!!!!!!!!!");

	//H�m�rs�klet ki�r�sa
	drawTempValue(currentMotTemp);

	nokia5110Display.display();
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
		if (currentMotTemp >= config.configVars.motTempAlarm) {
			lastMotTemp = currentMotTemp;
			drawWarningDisplay(currentMotTemp);
			buzzerAlarm();
			highTemp = true;

		} else if (lastMotTemp == -1.0f || abs(lastMotTemp - currentMotTemp) > TEMP_DIFF_TO_DISPLAY) {
			//Csak az els� �s a TEMP_DIFF_TO_DISPLAY-n�l nagyobb elt�r�sekre reag�lunk
			lastMotTemp = currentMotTemp;
			drawMainDisplay(currentMotTemp);
		}

		lastMiliSec = millis();
	}

	return highTemp;
}

typedef enum MenuState_t {
	OFF,	//Nem l�that�
	MAIN_MENU, //Main men� l�that�
	ITEM_MENU //Elem Be�ll�t� men� l�that�
};

MenuState_t menuState = OFF;

const byte MENU_VIEVPORT_LINEPOS[] = { 15, 25, 35 };
typedef struct MenuViewport_t {
		byte firstItem;
		byte lastItem;
		byte selectedItem;
} MenuViewPortT;
MenuViewPortT menuViewport;
#define MENU_VIEWPORT_SIZE 	3	/* Men� elemekb�l ennyi l�tszik */

/* v�ltoztathat� �rt�k t�pusa */
typedef enum valueType_t {
	BOOL, BYTE, PULSE, TEMP, FUNCT
};

typedef void (*voidFuncPtr)(void);
typedef struct MenuItem_t {
		String title;					// Men�felirat
		valueType_t valueType;			// �rt�k t�pus
		void *valuePtr;					// Az �rt�k pointere
		byte minValue;					// Minim�lis numerikus �rt�k
		byte maxValue;					// Maxim�lis numerikus �rt�k
		voidFuncPtr callbackFunct; 		// Egy�b m�veletek f�ggv�ny pointere, vagy NULL, ha nincs
} MenuItemT;
#define LAST_MENUITEM_NDX 	8 /* Az utols� men�elem indexe, 0-t�l indul */
MenuItemT menuItems[LAST_MENUITEM_NDX + 1];

//Men� inaktivit�si id� m�sodpercben
#define MENU_INACTIVE_IN_MSEC			(30 * 1000)

/**
 * Men� alap�llapotba
 */
void resetMenu(void) {
	//viewPort be�ll�t�s
	menuViewport.firstItem = 0;
	menuViewport.lastItem = MENU_VIEWPORT_SIZE - 1;
	menuViewport.selectedItem = 0;
}

//--- Men�elemek callback f�ggv�nyei
void menuLcdContrast(void) {
	nokia5110Display.setContrast(config.configVars.contrast);
	config.wantSaveConfig = true;
}
void menuLcdBackLight(void) {
	nokia5110Display.setBlackLightState(config.configVars.blackLightState);
	config.wantSaveConfig = true;
}
void menuBeepState(void) {
	if (config.configVars.beepState) {
		buzzerMenu();
	}
	config.wantSaveConfig = true;
}
void menuFactoryReset(void) {
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

	//Default konfig l�trehoz�sa
	config.createDefaultConfig();
	config.wantSaveConfig = true;

	//Konfig be�ll�t�sok �rv�nyes�t�se
	menuLcdContrast();
	menuLcdBackLight();
	menuBeepState();

	//men� alap�llapotba
	resetMenu();
	menuState = OFF;
}
void menuExit(void) {
	//men� alap�llapotba
	resetMenu();
	menuState = OFF; //Kil�p�nk a men�b�l
}

/**
 * Men� felh�z�sa
 */
void initMenuItems(void) {

	menuItems[0] = {"PreWeld pulse", PULSE, &config.configVars.preWeldPulseCnt, 0, 255, NULL};
	menuItems[1] = {"Pause pulse", PULSE, &config.configVars.pausePulseCnt, 0, 255, NULL};
	menuItems[2] = {"Weld pulse", PULSE, &config.configVars.weldPulseCnt, 1, 255, NULL};
	menuItems[3] = {"MOT T.Alrm", TEMP, &config.configVars.motTempAlarm, 30, 120, NULL};
	menuItems[4] = {"Contrast", BYTE, &config.configVars.contrast, 0, 255, menuLcdContrast};
	menuItems[5] = {"Disp light", BOOL, &config.configVars.blackLightState, 0, 1, menuLcdBackLight};
	menuItems[6] = {"Beep", BOOL, &config.configVars.beepState, 0, 1, menuBeepState};
	menuItems[7] = {"Fctry reset", FUNCT, NULL, 0, 0, menuFactoryReset};
	menuItems[8] = {"Exit menu", FUNCT, NULL, 0, 0, menuExit};

	resetMenu();
}

/**
 * MainMenu kirajzol�sa
 *  - Men�felirat
 *  - viewportban l�that� elemek megjeeln�t�se
 *  - kiv�lasztott elem h�tter�nek megv�ltoztat�sa
 */
void drawMainMenu(void) {

	nokia5110Display.clearDisplay();
	nokia5110Display.setTextSize(1);
	nokia5110Display.setTextColor(BLACK, WHITE);
	nokia5110Display.setCursor(15, 0);
	nokia5110Display.print("MAIN MENU");
	nokia5110Display.drawFastHLine(0, 10, 83, BLACK);

	for (byte i = 0; i < MENU_VIEWPORT_SIZE; i++) {

		byte itemNdx = menuViewport.firstItem + i;

		nokia5110Display.setCursor(0, MENU_VIEVPORT_LINEPOS[i]);

		//selected?
		if (itemNdx == menuViewport.selectedItem) {
			nokia5110Display.setTextColor(WHITE, BLACK);
			nokia5110Display.print(">" + menuItems[itemNdx].title);
		} else {
			nokia5110Display.setTextColor(BLACK, WHITE);
			nokia5110Display.print(" " + menuItems[itemNdx].title);
		}

	}
	nokia5110Display.display();
}

/**
 * Msec -> String konverzi�
 */
String time2Str(long x) {
#define SEC_IN_MSEC 1000
#define MIN_IN_MSEC (60 * SEC_IN_MSEC)

	String result = "";
	int min = x / MIN_IN_MSEC;
	if (min > 0) {
		result += min;
		result += "m";
	}
	x %= MIN_IN_MSEC;

	int sec = x / SEC_IN_MSEC;
	if (sec > 0) {
		result += " ";
		result += sec + "s";
	}
	x %= SEC_IN_MSEC;

	if (x > 0) {
		result += " ";
		result + x;
		result += "ms";
	}

	return result;
}

/**
 * men�elem be�ll�t� k�perny�
 */
void drawMenuItemValue() {

	MenuItemT p = menuItems[menuViewport.selectedItem];

	nokia5110Display.clearDisplay();

	nokia5110Display.setTextSize(1);
	nokia5110Display.setTextColor(BLACK, WHITE);
	nokia5110Display.setCursor(5, 0);
	nokia5110Display.print(p.title);
	nokia5110Display.drawFastHLine(0, 10, 83, BLACK);

	nokia5110Display.setCursor(5, 15);
	switch (p.valueType) {
		case TEMP:
		case BYTE:
		case BOOL:
			nokia5110Display.print("Value");
			break;

		case PULSE:
			nokia5110Display.print("Pulse");
			break;
	}

	nokia5110Display.setTextSize(2);
	nokia5110Display.setCursor(5, 25);

	//T�pus szerinti ki�r�s
	String dspValue = "unknown";
	switch (p.valueType) {
		case BOOL:
			dspValue = *(bool *) p.valuePtr ? "ON" : "OFF";
			break;

		case PULSE:
		case TEMP:
		case BYTE:
			dspValue = String(*(byte *) p.valuePtr);
			break;
	}
	nokia5110Display.print(dspValue);

	nokia5110Display.setTextSize(1);
	switch (p.valueType) {
		case BYTE:
		case BOOL:
			break;

		case TEMP:
			nokia5110Display.setCursor(55, 30);
			sprintf(tempBuff, "%cC", DEGREE_SYMBOL_CODE);
			nokia5110Display.print(tempBuff);
			break;

		case PULSE:
			if (spotWelderSystemPeriodTime > 0.0) {
				//nokia5110Display.setCursor(55, 30);

				nokia5110Display.setCursor(20, 40);
				byte value = *(byte *) p.valuePtr;
				long pulseLenght = spotWelderSystemPeriodTime * 1000.0 * value;
				nokia5110Display.print(time2Str(pulseLenght));
			}
			break;
	}

	nokia5110Display.display();
}

/**
 * Elembe�ll�t� men� kontroller
 */
void itemMenuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	if (menuState != ITEM_MENU) {
		return;
	}

	//Kinyerj�k a kiv�lasztott men�elem pointer�t
	MenuItemT p = menuItems[menuViewport.selectedItem];

	//Nem klikkeltek -> csak v�ltoztatj�k az elem �rt�k�t
	if (!rotaryClicked) {

		switch (rotaryDirection) {

			case RotaryEncoderAdapter::Direction::UP:

				switch (p.valueType) {
					case BYTE:
					case PULSE:
					case TEMP:
						if (*(byte *) p.valuePtr < p.maxValue) {
							(*(byte *) p.valuePtr)++;
						}
						break;

					case BOOL:
						if (!*(bool *) p.valuePtr) { //ha most false, akkor true-t csin�lunk bel�le
							*(bool *) p.valuePtr = true;
						}
						break;
				}
				break;

			case RotaryEncoderAdapter::Direction::DOWN:
				switch (p.valueType) {
					case BYTE:
					case PULSE:
					case TEMP:
						if (*(byte *) p.valuePtr > p.minValue) {
							(*(byte *) p.valuePtr)--;
						}
						break;
					case BOOL:
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
		drawMenuItemValue();

		//Ha van az almen�h�z hozz� callback, akkor azt is megh�vjuk
		if (p.callbackFunct != NULL) {
			p.callbackFunct();
		}

		return;

	} //if (!rotaryClicked)

	//Klikkeltek az elem �rt�k�re -> kil�p�nk a men�elem-b�l a  f�men�be
	menuState = MAIN_MENU;
	drawMainMenu();
}

/**
 * F�men� kontroller
 */
void mainMenuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	if (menuState != MAIN_MENU) {
		return;
	}

	//Nem klikkeltek -> csak tall�zunk a men�ben
	if (!rotaryClicked) {

		switch (rotaryDirection) {
			case RotaryEncoderAdapter::Direction::UP:

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
				break;

			case RotaryEncoderAdapter::Direction::DOWN:

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
				break;

			default:
				return;
		}

		drawMainMenu();
		return;

	} //if (!rotaryClicked)

	//
	// Klikkeltek a main men�ben egy men�elemre
	// - Kinyerj�k a kiv�lasztott men�elem pointer�t
	// - Kirajzoltatjuk az elem �rt�k�t
	// - �t�ll�tjuk az �llapotott az elembe�ll�t�sra
	//
	MenuItemT p = menuItems[menuViewport.selectedItem];

	//T�pus szerint megy�nk tov�bb
	switch (p.valueType) {
		//Ha ez egy �rt�kbe�ll�t� almen�
		case BOOL:
		case BYTE:
		case PULSE:
		case TEMP:
			menuState = ITEM_MENU;
			itemMenuController(false, RotaryEncoderAdapter::Direction::NONE); //K�r�nk egy men�elem be�ll�t� k�perny� kirajzol�st
			break;

			//Csak egy f�ggv�nyt kell h�vni, az majd elint�z mindent
		case FUNCT:
			p.callbackFunct();
			break;
	}
}

/**
 * Menu kezel�se
 */
void menuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	buzzerMenu();
	switch (menuState) {
		case OFF: 	// Nem l�tszik a f�men� -> Ha kikkeltek, akkor bel�p�nk a m�be
			if (rotaryClicked) {
				menuState = MAIN_MENU;
				drawMainMenu(); //Kirajzoltatjuk a f�men�t
			}
			break;

		case MAIN_MENU: //L�tszik a f�men�
			mainMenuController(rotaryClicked, rotaryDirection);
			break;

		case ITEM_MENU: //Elem v�ltoztat� men� l�tszik
			itemMenuController(rotaryClicked, rotaryDirection);
			break;
	}
}

/**
 * Men� inaktivit�s kezel�se
 */
void menuInactiveController() {
	switch (menuState) {
		case MAIN_MENU:
			resetMenu(); //Kil�p�nk a men�b�l
			menuState = OFF;
			break;

		case ITEM_MENU:
			drawMainMenu(); //Kil�p�nk az almen�b�l
			menuState = MAIN_MENU;
	}
}
//--- Spot Welding ---------------------------------------------------------------------------------------------------------------------------------------
/**
 * ZCD interrupt
 */
void zeroCrossDetect(void) {
	weldPeriodCnt++;
}

/**
 * Hegeszt�si protokoll
 */
void weldButtonPushed(void) {
	weldPeriodCnt = 0;

	//interrupt on
	sei();

	//LED-be
	digitalWrite(WELD_LED_PIN, HIGH);

	//Ha enged�lyezve van a kett�s impulzus
	if (config.configVars.preWeldPulseCnt > 0) {
		digitalWrite(TRIAC_PIN, HIGH);	//triak be
		while (weldPeriodCnt <= config.configVars.preWeldPulseCnt) {
			//NOP
		}
	}

	digitalWrite(TRIAC_PIN, LOW);	//triak ki
	//Ha enged�lyezve van a k�t hegeszt�si f�zis k�z�tti v�rakoz�s, akkor most v�runk
	//Ennek csak akkor van �rtelme, ha volt els� f�zis
	if (config.configVars.preWeldPulseCnt > 0 && config.configVars.pausePulseCnt > 0) {
		weldPeriodCnt = 0;
		while (weldPeriodCnt <= config.configVars.pausePulseCnt) {
			//NOP
		}
	}

	//F� hegeszt�si f�zis
	digitalWrite(TRIAC_PIN, HIGH);	//triak be
	weldPeriodCnt = 0;
	while (weldPeriodCnt <= config.configVars.weldPulseCnt) {
		//NOP
	}

	digitalWrite(TRIAC_PIN, LOW);	//triak ki

	//LED ki
	digitalWrite(WELD_LED_PIN, LOW);

	//interrupt tilt�sa
	cli();
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
	config.read();

	//Rotary encoder init
	rotaryEncoder.init();

	//Men�elemek inicializ�l�sa
	initMenuItems();

	//--- Display
	nokia5110Display.setBlackLightPin(LCD_BLACKLIGHT_PIN);
	nokia5110Display.setContrast(config.configVars.contrast);	//kontraszt
	nokia5110Display.setBlackLightState(config.configVars.blackLightState);	//h�tt�rvil�g�t�s
	drawSplashScreen();

	//Weld button PIN
	pinMode(WELD_BUTTON_PIN, INPUT);

	//--- ZCD Interrupt felh�z�sa
	pinMode(ZCD_PIN, INPUT);
	attachInterrupt(0, zeroCrossDetect, FALLING);

	//--- Triac PIN
	pinMode(TRIAC_PIN, OUTPUT);
	digitalWrite(TRIAC_PIN, LOW);

	//Weld LED PIN
	pinMode(WELD_LED_PIN, OUTPUT);
	digitalWrite(WELD_LED_PIN, LOW);

	//--- H�m�r�s
	tempSensors.begin();

	//id�m�r�s indul
	lastMiliSec = millis();

	//kisz�m�tjuk a peri�dus id�t
	spotWelderSystemPeriodTime = 1 / (float) SYSTEM_FREQUENCY;

}

/**
 * Main loop
 */
void loop() {

	static bool firstTime = true;	//els� fut�s jelz�se
	static byte weldButtonPrevState = LOW;   //A hegeszt�s gomb el�z� �llapota

	//--- Hegeszt�s kezel�se -------------------------------------------------------------------
	//Kiolvassuk a weld button �llapot�t
	byte weldButtonCurrentState = digitalRead(WELD_BUTTON_PIN);
	if (weldButtonCurrentState != weldButtonPrevState && weldButtonCurrentState == HIGH && weldButtonPrevState == LOW) {
		weldButtonPushed();
		menuState = OFF; //kil�p�nk a men�b�l, ha �pp benne voltunk
	}
	weldButtonPrevState = weldButtonCurrentState;

	//--- MainDisplay kezel�se ---
	if (menuState == OFF) {
		lastMotTemp = -1.0f; //kir�szakoljuk a mainScren ki�rat�s�t
		mainDisplayController();
	}

	//--- Men� kezel�se ---
	if (firstTime) {
		firstTime = false;
		rotaryEncoder.SetChanged();   // force an update on active rotary
		rotaryEncoder.readRotaryEncoder();   //els� kiolvas�st eldobjuk
	}

	RotaryEncoderAdapter::RotaryEncoderResult rotaryEncoderResult = rotaryEncoder.readRotaryEncoder();
	//Ha klikkeltek VAGY van ir�ny, akkor a men�t piszk�ljuk
	if (rotaryEncoderResult.clicked || rotaryEncoderResult.direction != RotaryEncoderAdapter::Direction::NONE) {
		menuController(rotaryEncoderResult.clicked, rotaryEncoderResult.direction);
		//men�t�tlens�g 'reset'
		lastMiliSec = millis();
	}

	//Men� t�tlens�g figyel�se
	if (menuState != OFF && ((millis() - lastMiliSec) > MENU_INACTIVE_IN_MSEC)) {
		menuInactiveController();
		lastMiliSec = millis();
	}

	//Ha m�r nem vagyunk a men�ben, �s kell menteni valamit a konfigban, akkor azt most tessz�k meg
	if (menuState == OFF && config.wantSaveConfig) {
		config.save();
	}
}

