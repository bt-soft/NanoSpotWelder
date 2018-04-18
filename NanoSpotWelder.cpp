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

//------------------- Hõmérés
#define DALLAS18B20_PIN			A2
#define MOT_TEMP_SENSOR_NDX 	0
#define TEMP_DIFF_TO_DISPLAY  	0.5f	/* fél fokonkénti eltérésekkel foglalkozunk csak */
float lastMotTemp = -1.0f;				//A MOT utolsó mért hõmérséklete
OneWire oneWire(DALLAS18B20_PIN);
DallasTemperature tempSensors(&oneWire);

//------------------- Menü support
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
long lastMiliSec = -1;					// hõmérséklet mmérésre, menü tétlenségi idõ detektálása*/
char tempBuff[64];
#define DEGREE_SYMBOL_CODE 		247		/* Az LCD-n a '°' jel kódja */

//------------------- Spot welding paraméterek
#define ZCD_PIN 				2		/* Zero Cross Detection PIN, megszakításban */
#define TRIAC_PIN 				12 		/* D12 Triac/MOC vezérlés PIN */
#define WELD_LED_PIN 			11		/* D11 Hegesztés LED visszajelzés */
#define WELD_BUTTON_PIN 		10 		/* D10 Hegesztés gomb */

//Periódus Számláló, hegesztés alatt megszakításkor inkrementálódik
volatile uint16_t weldPeriodCnt = 0;

//Mért hálózati frekvencia
float spotWelderSystemPeriodTime = 0.0;

//------------------- Beeper
#define BUZZER_PIN 				13

//--- --------------- Buzzer

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

//------------------------- Menu
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
 * Hõmérséklet kiírása a main- és az alarm screen-nél
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

	//Hõmérséklet kiírása
	nokia5110Display.print("\nMOT Temp");
	drawTempValue(currentMotTemp);

	nokia5110Display.display();
}

/**
 * Magas hõmérséklet riasztás
 */
void drawWarningDisplay(float currentMotTemp) {
	nokia5110Display.clearDisplay();
	nokia5110Display.setTextColor(BLACK);
	nokia5110Display.setTextSize(1);

	nokia5110Display.println("!!!!!!!!!!!!!!");
	nokia5110Display.println(" MOT Temp is");
	nokia5110Display.println("  too high!");
	nokia5110Display.println("!!!!!!!!!!!!!!");

	//Hõmérséklet kiírása
	drawTempValue(currentMotTemp);

	nokia5110Display.display();
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
		if (currentMotTemp >= config.configVars.motTempAlarm) {
			lastMotTemp = currentMotTemp;
			drawWarningDisplay(currentMotTemp);
			buzzerAlarm();
			highTemp = true;

		} else if (lastMotTemp == -1.0f || abs(lastMotTemp - currentMotTemp) > TEMP_DIFF_TO_DISPLAY) {
			//Csak az elsõ és a TEMP_DIFF_TO_DISPLAY-nál nagyobb eltérésekre reagálunk
			lastMotTemp = currentMotTemp;
			drawMainDisplay(currentMotTemp);
		}

		lastMiliSec = millis();
	}

	return highTemp;
}

typedef enum MenuState_t {
	OFF,	//Nem látható
	MAIN_MENU, //Main menü látható
	ITEM_MENU //Elem Beállító menü látható
};

MenuState_t menuState = OFF;

const byte MENU_VIEVPORT_LINEPOS[] = { 15, 25, 35 };
typedef struct MenuViewport_t {
		byte firstItem;
		byte lastItem;
		byte selectedItem;
} MenuViewPortT;
MenuViewPortT menuViewport;
#define MENU_VIEWPORT_SIZE 	3	/* Menü elemekbõl ennyi látszik */

/* változtatható érték típusa */
typedef enum valueType_t {
	BOOL, BYTE, PULSE, TEMP, FUNCT
};

typedef void (*voidFuncPtr)(void);
typedef struct MenuItem_t {
		String title;					// Menüfelirat
		valueType_t valueType;			// Érték típus
		void *valuePtr;					// Az érték pointere
		byte minValue;					// Minimális numerikus érték
		byte maxValue;					// Maximális numerikus érték
		voidFuncPtr callbackFunct; 		// Egyéb mûveletek függvény pointere, vagy NULL, ha nincs
} MenuItemT;
#define LAST_MENUITEM_NDX 	8 /* Az utolsó menüelem indexe, 0-tól indul */
MenuItemT menuItems[LAST_MENUITEM_NDX + 1];

//Menü inaktivitási idõ másodpercben
#define MENU_INACTIVE_IN_MSEC			(30 * 1000)

/**
 * Menü alapállapotba
 */
void resetMenu(void) {
	//viewPort beállítás
	menuViewport.firstItem = 0;
	menuViewport.lastItem = MENU_VIEWPORT_SIZE - 1;
	menuViewport.selectedItem = 0;
}

//--- Menüelemek callback függvényei
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

	//Default konfig létrehozása
	config.createDefaultConfig();
	config.wantSaveConfig = true;

	//Konfig beállítások érvényesítése
	menuLcdContrast();
	menuLcdBackLight();
	menuBeepState();

	//menü alapállapotba
	resetMenu();
	menuState = OFF;
}
void menuExit(void) {
	//menü alapállapotba
	resetMenu();
	menuState = OFF; //Kilépünk a menübõl
}

/**
 * Menü felhúzása
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
 * MainMenu kirajzolása
 *  - Menüfelirat
 *  - viewportban látható elemek megjeelnítése
 *  - kiválasztott elem hátterének megváltoztatása
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
 * Msec -> String konverzió
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
 * menüelem beállítõ képernyõ
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

	//Típus szerinti kiírás
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
 * Elembeállító menü kontroller
 */
void itemMenuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	if (menuState != ITEM_MENU) {
		return;
	}

	//Kinyerjük a kiválasztott menüelem pointerét
	MenuItemT p = menuItems[menuViewport.selectedItem];

	//Nem klikkeltek -> csak változtatják az elem értékét
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
						if (!*(bool *) p.valuePtr) { //ha most false, akkor true-t csinálunk belõle
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
		drawMenuItemValue();

		//Ha van az almenühöz hozzá callback, akkor azt is meghívjuk
		if (p.callbackFunct != NULL) {
			p.callbackFunct();
		}

		return;

	} //if (!rotaryClicked)

	//Klikkeltek az elem értékére -> kilépünk a menüelem-bõl a  fõmenübe
	menuState = MAIN_MENU;
	drawMainMenu();
}

/**
 * Fõmenü kontroller
 */
void mainMenuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	if (menuState != MAIN_MENU) {
		return;
	}

	//Nem klikkeltek -> csak tallózunk a menüben
	if (!rotaryClicked) {

		switch (rotaryDirection) {
			case RotaryEncoderAdapter::Direction::UP:

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
				break;

			case RotaryEncoderAdapter::Direction::DOWN:

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
				break;

			default:
				return;
		}

		drawMainMenu();
		return;

	} //if (!rotaryClicked)

	//
	// Klikkeltek a main menüben egy menüelemre
	// - Kinyerjük a kiválasztott menüelem pointerét
	// - Kirajzoltatjuk az elem értékét
	// - Átállítjuk az állapotott az elembeállításra
	//
	MenuItemT p = menuItems[menuViewport.selectedItem];

	//Típus szerint megyünk tovább
	switch (p.valueType) {
		//Ha ez egy értékbeállító almenü
		case BOOL:
		case BYTE:
		case PULSE:
		case TEMP:
			menuState = ITEM_MENU;
			itemMenuController(false, RotaryEncoderAdapter::Direction::NONE); //Kérünk egy menüelem beállító képernyõ kirajzolást
			break;

			//Csak egy függvényt kell hívni, az majd elintéz mindent
		case FUNCT:
			p.callbackFunct();
			break;
	}
}

/**
 * Menu kezelése
 */
void menuController(bool rotaryClicked, RotaryEncoderAdapter::Direction rotaryDirection) {

	buzzerMenu();
	switch (menuState) {
		case OFF: 	// Nem látszik a fõmenü -> Ha kikkeltek, akkor belépünk a mübe
			if (rotaryClicked) {
				menuState = MAIN_MENU;
				drawMainMenu(); //Kirajzoltatjuk a fõmenüt
			}
			break;

		case MAIN_MENU: //Látszik a fõmenü
			mainMenuController(rotaryClicked, rotaryDirection);
			break;

		case ITEM_MENU: //Elem változtató menü látszik
			itemMenuController(rotaryClicked, rotaryDirection);
			break;
	}
}

/**
 * Menü inaktivitás kezelése
 */
void menuInactiveController() {
	switch (menuState) {
		case MAIN_MENU:
			resetMenu(); //Kilépünk a menübõl
			menuState = OFF;
			break;

		case ITEM_MENU:
			drawMainMenu(); //Kilépünk az almenübõl
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
 * Hegesztési protokoll
 */
void weldButtonPushed(void) {
	weldPeriodCnt = 0;

	//interrupt on
	sei();

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

	//interrupt tiltása
	cli();
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
	config.read();

	//Rotary encoder init
	rotaryEncoder.init();

	//Menüelemek inicializálása
	initMenuItems();

	//--- Display
	nokia5110Display.setBlackLightPin(LCD_BLACKLIGHT_PIN);
	nokia5110Display.setContrast(config.configVars.contrast);	//kontraszt
	nokia5110Display.setBlackLightState(config.configVars.blackLightState);	//háttérvilágítás
	drawSplashScreen();

	//Weld button PIN
	pinMode(WELD_BUTTON_PIN, INPUT);

	//--- ZCD Interrupt felhúzása
	pinMode(ZCD_PIN, INPUT);
	attachInterrupt(0, zeroCrossDetect, FALLING);

	//--- Triac PIN
	pinMode(TRIAC_PIN, OUTPUT);
	digitalWrite(TRIAC_PIN, LOW);

	//Weld LED PIN
	pinMode(WELD_LED_PIN, OUTPUT);
	digitalWrite(WELD_LED_PIN, LOW);

	//--- Hõmérés
	tempSensors.begin();

	//idõmérés indul
	lastMiliSec = millis();

	//kiszámítjuk a periódus idõt
	spotWelderSystemPeriodTime = 1 / (float) SYSTEM_FREQUENCY;

}

/**
 * Main loop
 */
void loop() {

	static bool firstTime = true;	//elsõ futás jelzése
	static byte weldButtonPrevState = LOW;   //A hegesztés gomb elõzõ állapota

	//--- Hegesztés kezelése -------------------------------------------------------------------
	//Kiolvassuk a weld button állapotát
	byte weldButtonCurrentState = digitalRead(WELD_BUTTON_PIN);
	if (weldButtonCurrentState != weldButtonPrevState && weldButtonCurrentState == HIGH && weldButtonPrevState == LOW) {
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
	if (firstTime) {
		firstTime = false;
		rotaryEncoder.SetChanged();   // force an update on active rotary
		rotaryEncoder.readRotaryEncoder();   //elsõ kiolvasást eldobjuk
	}

	RotaryEncoderAdapter::RotaryEncoderResult rotaryEncoderResult = rotaryEncoder.readRotaryEncoder();
	//Ha klikkeltek VAGY van irány, akkor a menüt piszkáljuk
	if (rotaryEncoderResult.clicked || rotaryEncoderResult.direction != RotaryEncoderAdapter::Direction::NONE) {
		menuController(rotaryEncoderResult.clicked, rotaryEncoderResult.direction);
		//menütétlenség 'reset'
		lastMiliSec = millis();
	}

	//Menü tétlenség figyelése
	if (menuState != OFF && ((millis() - lastMiliSec) > MENU_INACTIVE_IN_MSEC)) {
		menuInactiveController();
		lastMiliSec = millis();
	}

	//Ha már nem vagyunk a menüben, és kell menteni valamit a konfigban, akkor azt most tesszük meg
	if (menuState == OFF && config.wantSaveConfig) {
		config.save();
	}
}

