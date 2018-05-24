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
#define DEGREE_SYMBOL_CODE 	247			/* Az LCD-n a '�' jel k�dja */

class LcdMenu {

	private:
		const byte PROGMEM MENU_VIEVPORT_LINEPOS[3] = { 15, 25, 35 };	//Men�elemek sorai
		char tempBuff[32];
		Nokia5110DisplayWrapper *nokia5110Display;

		typedef struct MenuViewport_t {
				byte firstItem;
				byte lastItem;
				byte selectedItem;
		};
		MenuViewport_t menuViewport;

	public:
		typedef enum MenuState_t {
			FORCE_MAIN_DISPLAY, 			//Izomb�l kirajzoljuk a MainDisplay-t
			OFF,							//Nem l�that�
			MAIN_MENU, 						//Main men� l�that�
			SUB_MENU, 						//almen� l�that�
			ITEM_MENU 						//Elem Be�ll�t� men� l�that�
		};

		/* v�ltoztathat� �rt�k t�pusa */
		typedef enum MenuValueType_t {
			BOOL, BYTE, PULSE, TEMP, WELD, FUNCT
		};

	private:
		typedef void (LcdMenu::*voidFuncPtr)(void);

		//Egy men�elem szerkezete
		typedef struct MenuItem_t {
				char *title;					// Men�felirat
				MenuValueType_t valueType;		// �rt�k t�pus
				void *valuePtr;					// Az �rt�k pointere
				byte minValue;					// Minim�lis numerikus �rt�k
				byte maxValue;					// Maxim�lis numerikus �rt�k
				voidFuncPtr callbackFunct; 		// Egy�b m�veletek f�ggv�ny pointere, vagy NULL, ha nincs
		};

		//Egy men� szerkezete
		typedef struct Menu_t {
				char *menuTitle = NULL;				//Men� title
				byte menuItemsCnt = 0;			//255 almen� el�g lesz...
				MenuItem_t *pMenuItems = NULL; 		//men�elemek ter�lete
		};

		//Men�k
		Menu_t *pMainMenu;
		Menu_t *pWeldSettingsSubMenu;
		Menu_t *pLcdSettingsSubMenu;
		Menu_t *pFactoryResetSubMenu;

		//Aktu�lis men� pointere
		Menu_t *pCurrentMenu;

		MenuViewport_t prevMenuViewport;
		MenuState_t prevMenuState;


	public:
		MenuState_t menuState = OFF;

		LcdMenu(void);
		void drawSplashScreen(void);
		void resetViewPort(void);
		void drawTemperatureValue(float *pCurrentMotTemp);
		void drawMainDisplay(float *pCurrentMotTemp);
		void drawWarningDisplay(float *pCurrentMotTemp);
		void drawCurrentMenu(void);
		void drawMenuItemValue();
		void stepDown(void);
		void stepUp(void);
		void incSelectedValue(void);
		void decSelectedValue(void);
		void invokeMenuItemCallBackFunct(void);

		/**
		 * Kiv�lasztott men�elem m�g�tti v�ltoztathat� �rt�k t�pusa
		 */
		MenuValueType_t getSelectedMenuItemValueType() {
			return getSelectedMenuItemPtr()->valueType;
		}

		//-- Men��lapotV�lt�sok
		void enterMainMenu(void){
			menuState = LcdMenu::MAIN_MENU;
			prevMenuState = menuState;
			resetViewPort();
			drawCurrentMenu(); //Kirajzoltatjuk a f�men�t
		}
		void exitMainMenu(void){
			pCurrentMenu = pMainMenu;
			menuState = OFF;
			prevMenuState = menuState;
		}

		void enterSubMenu(void){
			//Elmentj�k az el�z� �llapotot
			memcpy(&prevMenuViewport, &menuViewport, sizeof(MenuViewport_t));
			prevMenuState = menuState;

			menuState = SUB_MENU;
			resetViewPort();
			drawCurrentMenu(); //kirajzoltatjuk az almen�t.
		}
		void exitSubMenu(void){
			//Vissza�ll�tjuk az el�z� �llapotot
			memcpy(&menuViewport, &prevMenuViewport, sizeof(MenuViewport_t));
			menuState = prevMenuState;

			pCurrentMenu = pMainMenu;
			drawCurrentMenu();
		}

		void enterItemMenu(void){
			prevMenuState = menuState;
			menuState = ITEM_MENU;
		}
		void exitItemMenu(void){
			menuState = prevMenuState;
			drawCurrentMenu();
		}

	private:
		void initMenuItems(void);
		void lcdContrastCallBack(void);
		void lcdBiasCallBack(void);
		void lcdBackLightCallBack(void);
		void beepStateCallBack(void);
		void factoryResetCallBack(void);
		void menuExitCallBack(void);

		//--- al men�k
		/**
		 * WELD almen�be l�pt�nk
		 */
		void enterWeldSettingsSubmenuCallBack(void) {
			pCurrentMenu = pWeldSettingsSubMenu;
			enterSubMenu();
		}
		/**
		 * LCD almen�be l�pt�nk
		 */
		void enterLcdSettingsSubmenuCallBack(void) {
			pCurrentMenu = pLcdSettingsSubMenu;
			enterSubMenu();
		}
		/**
		 * Factory reset almen�be l�pt�nk
		 */
		void enterFactoryResetSubmenuCallBack(void){
			pCurrentMenu = pFactoryResetSubMenu;
			enterSubMenu();
		}

		String msecToStr(long x);

		/**
		 * Egy �j men�elem hozz�ad�sa a men�h�z
		 */
		void addMenuItem(Menu_t *pMenu, MenuItem_t menuItem) {
			pMenu->pMenuItems = realloc(pMenu->pMenuItems, (pMenu->menuItemsCnt + 1) * sizeof(MenuItem_t)); //mem�riafoglal�s
			memcpy(pMenu->pMenuItems + pMenu->menuItemsCnt, &menuItem, sizeof(MenuItem_t)); //men�elem odam�sol�sa
			pMenu->menuItemsCnt++; //+1 men�elem van m�r
		}

		/**
		 * A kiv�lasztott men�elem pointer�nek elk�r�se
		 */
		MenuItem_t *getSelectedMenuItemPtr() {
			return pCurrentMenu->pMenuItems + menuViewport.selectedItem;
		}

};

#endif /* LCDMENU_H_ */
