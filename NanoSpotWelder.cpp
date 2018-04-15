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
#define BACK_MENU_ITEM "Back"
char menu[][MAX_ITEM_SIZE] = { "Weld Options", "Temp alarm", "Beep", "Light", BACK_MENU_ITEM };
char subMenu_Weld[][MAX_ITEM_SIZE] = { "Preweld", "Pause", "Weld", BACK_MENU_ITEM };
int menuItemSelected;
int subMenuSelected;

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
 * Sipol�s
 */
void buzzer(void) {
	tone(BUZZER_PIN, 1000);
	delay(500);
	tone(BUZZER_PIN, 800);
	delay(500);
	noTone(BUZZER_PIN);
}

/**
 * Riaszt�s
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
	sprintf(&tempBuff[0], "%s", config.configVars.version);
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
	sprintf(&tempBuff[0], "%-3d   %-3d  %-3d", config.configVars.preWeldPulseCnt, config.configVars.pausePulseCnt, config.configVars.weldPulseCnt);
	nokia5110Display.println(tempBuff);

	nokia5110Display.print("\nMOT Temp");

	nokia5110Display.setTextSize(2);
	nokia5110Display.setCursor(18, 32);
	dtostrf(currentMotTemp, 1, 1, tempBuff);
	nokia5110Display.print(tempBuff);

	nokia5110Display.setTextSize(1);
	nokia5110Display.setCursor(68, 38);
	sprintf(&tempBuff[0], "%cC", DEGREE_SYMBOL_CODE);
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

	nokia5110Display.println("\n  MOT Temp is");
	nokia5110Display.println("   too high!");

	nokia5110Display.setTextSize(2);
	nokia5110Display.setCursor(18, 32);
	dtostrf(currentMotTemp, 1, 1, tempBuff);
	nokia5110Display.print(tempBuff);

	nokia5110Display.setTextSize(1);
	nokia5110Display.setCursor(68, 38);
	sprintf(&tempBuff[0], "%cC", DEGREE_SYMBOL_CODE);
	nokia5110Display.print(tempBuff);

	nokia5110Display.display();
}

/**
 * Main display vez�rl�s
 */
void mainDisplayController() {
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

		} else if (lastMotTemp == -1.0f || abs(lastMotTemp - currentMotTemp) > TEMP_DIFF_TO_DISPLAY) {
			//Csak az els� �s a TEMP_DIFF_TO_DISPLAY-n�l nagyobb elt�r�sekre reag�lunk
			lastMotTemp = currentMotTemp;
			drawMainDisplay(currentMotTemp);
		}

		lastMiliSec = millis();
	}
}

//--- Men� -----------------------------------------------------------------------------------------------------------------------------------------------
/**
 * ClickEncoder service h�v�s
 * ~1msec-enk�nt meg kell h�vni, ezt a Timer1 interrupt v�gzi
 */
void rotaryEncoderServiceInterrupt(void) {
	rotaryEncoder->service();
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

//--------------------------------------------------------------------------------------------------------------------------------------------------------

/**
 * Boot be�ll�t�sok
 */
void setup() {

	//Konfig felolvas�sa
	//config = new Config();
	config.read();

	//--- Display
	nokia5110Display.setBlackLightPin(LCD_BLACKLIGHT_PIN);
	nokia5110Display.contrast = config.configVars.contrast;
	nokia5110Display.contrastSet();
	drawSplashScreen();

	//--- Rotary Encoder felh�z�sa
	rotaryEncoder = new KY040RotaryEncoder(A0, A1, A2);
	rotaryEncoder->setAccelerationEnabled(false);
	rotaryEncoder->setDoubleClickEnabled(true);

	//--- ClickEncoder timer felh�z�sa
	Timer1.initialize(ROTARY_ENCODER_SERVICE_INTERVAL_IN_USEC);
	Timer1.attachInterrupt(rotaryEncoderServiceInterrupt);

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

	buzzer();
}

/**
 * Main loop
 */
void loop() {

	{	//Hegeszt�s kezel�se

		//Kiolvassuk a weld button �llapot�t
		byte weldButtonCurrentState = digitalRead(WELD_BUTTON_PIN);

		// if the button state changes to pressed, remember the start time
		if (weldButtonCurrentState == HIGH && weldButtonPrevState == LOW && !isWelding) {
			weldButtonPushed();
		}
		weldButtonPrevState = weldButtonCurrentState;
	}

	mainDisplayController();

}

