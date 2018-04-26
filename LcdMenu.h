/*
 * LcdMenu.h
 *
 *  Created on: 2018. ápr. 26.
 *      Author: u626374
 */

#ifndef LCDMENU_H_
#define LCDMENU_H_

#include "NanoSpotWederPinouts.h"
#include "Nokia5110Display.h"
#include "Config.h"

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

#define MENU_VIEWPORT_SIZE 	3			/* Menü elemekbõl ennyi látszik egyszerre */
#define LAST_MENUITEM_NDX 	9 			/* Az utolsó menüelem indexe, 0-tól indul */
#define DEGREE_SYMBOL_CODE 	247			/* Az LCD-n a '°' jel kódja */

class LcdMenu {

private:
	char tempBuff[32];
	Nokia5110Display *nokia5110Display;

public:
	typedef enum MenuState_t {
		OFF,	//Nem látható
		MAIN_MENU, //Main menü látható
		ITEM_MENU //Elem Beállító menü látható
	};

	/* változtatható érték típusa */
	typedef enum valueType_t {
		BOOL, BYTE, PULSE, TEMP, WELD, FUNCT
	};

	typedef struct MenuViewport_t {
		byte firstItem;
		byte lastItem;
		byte selectedItem;
	} MenuViewPortT;

	typedef void (LcdMenu::*voidFuncPtr)(void);
	typedef struct MenuItem_t {
		char *title;				// Menüfelirat
		valueType_t valueType;		// Érték típus
		void *valuePtr;				// Az érték pointere
		byte minValue;				// Minimális numerikus érték
		byte maxValue;				// Maximális numerikus érték
		voidFuncPtr callbackFunct; 	// Egyéb mûveletek függvény pointere, vagy NULL, ha nincs
	} MenuItemT;

public:
	MenuState_t menuState = OFF;
	MenuViewPortT menuViewport;
	MenuItemT menuItems[LAST_MENUITEM_NDX + 1];

public:
	/**
	 * Konstruktor
	 */
	LcdMenu(void);
	void drawSplashScreen(void);
	void resetMenu(void);
	void drawTempValue(float *pCurrentMotTemp);
	void drawMainDisplay(float *pCurrentMotTemp);
	void drawWarningDisplay(float *pCurrentMotTemp);
	void drawMainMenu(void);
	void drawMenuItemValue();

private:
	void initMenuItems(void);
	void menuLcdContrast(void);
	void menuLcdBackLight(void);
	void menuBeepState(void);
	void menuFactoryReset(void);
	void menuExit(void);

private:
	const byte MENU_VIEVPORT_LINEPOS[3] PROGMEM = { 15, 25, 35 };
	String msecToStr(long x);

};

#endif /* LCDMENU_H_ */
