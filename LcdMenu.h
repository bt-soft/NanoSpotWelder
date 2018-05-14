/*
 *
 * Copyright 2018 - BT-Soft
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * LcdMenu.h
 *
 *  Created on: 2018. ápr. 26.
 *      Author: bt-soft
 */

#ifndef LCDMENU_H_
#define LCDMENU_H_

#include "NanoSpotWederPinouts.h"
#include "Config.h"
#include "Nokia5110DisplayWrapper.h"


#define MENU_VIEWPORT_SIZE 	3			/* Menü elemekbõl ennyi látszik egyszerre */
#define LAST_MENUITEM_NDX 	9 			/* Az utolsó menüelem indexe, 0-tól indul */
#define DEGREE_SYMBOL_CODE 	247			/* Az LCD-n a '°' jel kódja */

class LcdMenu {

private:
	char tempBuff[32];
	Nokia5110DisplayWrapper *nokia5110Display;

	typedef struct MenuViewport_t {
		byte firstItem;
		byte lastItem;
		byte selectedItem;
	} MenuViewPortT;
	MenuViewPortT menuViewport;

public:
	typedef enum MenuState_t {
		FORCE_MAIN_DISPLAY, //Izombõl kirajzoljuk a MainDisplay-t
		OFF,	//Nem látható
		MAIN_MENU, //Main menü látható
		ITEM_MENU //Elem Beállító menü látható
	};

	/* változtatható érték típusa */
	typedef enum valueType_t {
		BOOL, BYTE, PULSE, TEMP, WELD, FUNCT
	};

	typedef void (LcdMenu::*voidFuncPtr)(void);
	typedef struct MenuItem_t {
		char *title;				// Menüfelirat
		valueType_t valueType;		// Érték típus
		void *valuePtr;				// Az érték pointere
		byte minValue;				// Minimális numerikus érték
		byte maxValue;				// Maximális numerikus érték
		voidFuncPtr callbackFunct; 	// Egyéb mûveletek függvény pointere, vagy NULL, ha nincs
	} MenuItemT;

	MenuState_t menuState = OFF;
	MenuItemT menuItems[LAST_MENUITEM_NDX + 1];

	LcdMenu(void);
	void drawSplashScreen(void);
	void resetMenu(void);
	void drawTempValue(float *pCurrentMotTemp);
	void drawMainDisplay(float *pCurrentMotTemp);
	void drawWarningDisplay(float *pCurrentMotTemp);
	void drawMainMenu(void);
	void drawMenuItemValue();
	void stepDown(void);
	void stepUp(void);
	void incSelectedValue(void);
	void decSelectedValue(void);
	void invokeMenuItemCallBackFunct(void);

	/**
	 * Kiválasztott menüelem pointerének elkérése
	 */
	MenuItemT *getSelectedItemPtr() {
		return &menuItems[menuViewport.selectedItem];
	}

private:
	void initMenuItems(void);
	void lcdContrastCallBack(void);
	void lcdBiasCallBack(void);
	void lcdBackLightCallBack(void);
	void beepStateCallBack(void);
	void factoryResetCallBack(void);
	void exitCallBack(void);

private:
	const byte PROGMEM MENU_VIEVPORT_LINEPOS[3] = { 15, 25, 35 };	//Menüelemek sorai
	String msecToStr(long x);

};

#endif /* LCDMENU_H_ */
