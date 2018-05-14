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
 *  Created on: 2018. �pr. 26.
 *      Author: bt-soft
 */

#ifndef LCDMENU_H_
#define LCDMENU_H_

#include "NanoSpotWederPinouts.h"
#include "Config.h"
#include "Nokia5110DisplayWrapper.h"


#define MENU_VIEWPORT_SIZE 	3			/* Men� elemekb�l ennyi l�tszik egyszerre */
#define LAST_MENUITEM_NDX 	9 			/* Az utols� men�elem indexe, 0-t�l indul */
#define DEGREE_SYMBOL_CODE 	247			/* Az LCD-n a '�' jel k�dja */

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
		FORCE_MAIN_DISPLAY, //Izomb�l kirajzoljuk a MainDisplay-t
		OFF,	//Nem l�that�
		MAIN_MENU, //Main men� l�that�
		ITEM_MENU //Elem Be�ll�t� men� l�that�
	};

	/* v�ltoztathat� �rt�k t�pusa */
	typedef enum valueType_t {
		BOOL, BYTE, PULSE, TEMP, WELD, FUNCT
	};

	typedef void (LcdMenu::*voidFuncPtr)(void);
	typedef struct MenuItem_t {
		char *title;				// Men�felirat
		valueType_t valueType;		// �rt�k t�pus
		void *valuePtr;				// Az �rt�k pointere
		byte minValue;				// Minim�lis numerikus �rt�k
		byte maxValue;				// Maxim�lis numerikus �rt�k
		voidFuncPtr callbackFunct; 	// Egy�b m�veletek f�ggv�ny pointere, vagy NULL, ha nincs
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
	 * Kiv�lasztott men�elem pointer�nek elk�r�se
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
	const byte PROGMEM MENU_VIEVPORT_LINEPOS[3] = { 15, 25, 35 };	//Men�elemek sorai
	String msecToStr(long x);

};

#endif /* LCDMENU_H_ */
