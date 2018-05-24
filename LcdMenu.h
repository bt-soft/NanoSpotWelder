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
#define DEGREE_SYMBOL_CODE 	247			/* Az LCD-n a '°' jel kódja */

class LcdMenu {

	private:
		const byte PROGMEM MENU_VIEVPORT_LINEPOS[3] = { 15, 25, 35 };	//Menüelemek sorai
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
			FORCE_MAIN_DISPLAY, 			//Izombõl kirajzoljuk a MainDisplay-t
			OFF,							//Nem látható
			MAIN_MENU, 						//Main menü látható
			SUB_MENU, 						//almenü látható
			ITEM_MENU 						//Elem Beállító menü látható
		};

		/* változtatható érték típusa */
		typedef enum MenuValueType_t {
			BOOL, BYTE, PULSE, TEMP, WELD, FUNCT
		};

	private:
		typedef void (LcdMenu::*voidFuncPtr)(void);

		//Egy menüelem szerkezete
		typedef struct MenuItem_t {
				char *title;					// Menüfelirat
				MenuValueType_t valueType;		// Érték típus
				void *valuePtr;					// Az érték pointere
				byte minValue;					// Minimális numerikus érték
				byte maxValue;					// Maximális numerikus érték
				voidFuncPtr callbackFunct; 		// Egyéb mûveletek függvény pointere, vagy NULL, ha nincs
		};

		//Egy menü szerkezete
		typedef struct Menu_t {
				char *menuTitle = NULL;				//Menü title
				byte menuItemsCnt = 0;			//255 almenü elég lesz...
				MenuItem_t *pMenuItems = NULL; 		//menüelemek területe
		};

		//Menük
		Menu_t *pMainMenu;
		Menu_t *pWeldSettingsSubMenu;
		Menu_t *pLcdSettingsSubMenu;
		Menu_t *pFactoryResetSubMenu;

		//Aktuális menü pointere
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
		 * Kiválasztott menüelem mögötti változtatható érték típusa
		 */
		MenuValueType_t getSelectedMenuItemValueType() {
			return getSelectedMenuItemPtr()->valueType;
		}

		//-- MenüÁlapotVáltások
		void enterMainMenu(void){
			menuState = LcdMenu::MAIN_MENU;
			prevMenuState = menuState;
			resetViewPort();
			drawCurrentMenu(); //Kirajzoltatjuk a fõmenüt
		}
		void exitMainMenu(void){
			pCurrentMenu = pMainMenu;
			menuState = OFF;
			prevMenuState = menuState;
		}

		void enterSubMenu(void){
			//Elmentjük az elõzõ állapotot
			memcpy(&prevMenuViewport, &menuViewport, sizeof(MenuViewport_t));
			prevMenuState = menuState;

			menuState = SUB_MENU;
			resetViewPort();
			drawCurrentMenu(); //kirajzoltatjuk az almenüt.
		}
		void exitSubMenu(void){
			//Visszaállítjuk az elõzõ állapotot
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

		//--- al menük
		/**
		 * WELD almenübe léptünk
		 */
		void enterWeldSettingsSubmenuCallBack(void) {
			pCurrentMenu = pWeldSettingsSubMenu;
			enterSubMenu();
		}
		/**
		 * LCD almenübe léptünk
		 */
		void enterLcdSettingsSubmenuCallBack(void) {
			pCurrentMenu = pLcdSettingsSubMenu;
			enterSubMenu();
		}
		/**
		 * Factory reset almenübe léptünk
		 */
		void enterFactoryResetSubmenuCallBack(void){
			pCurrentMenu = pFactoryResetSubMenu;
			enterSubMenu();
		}

		String msecToStr(long x);

		/**
		 * Egy új menüelem hozzáadása a menühöz
		 */
		void addMenuItem(Menu_t *pMenu, MenuItem_t menuItem) {
			pMenu->pMenuItems = realloc(pMenu->pMenuItems, (pMenu->menuItemsCnt + 1) * sizeof(MenuItem_t)); //memóriafoglalás
			memcpy(pMenu->pMenuItems + pMenu->menuItemsCnt, &menuItem, sizeof(MenuItem_t)); //menüelem odamásolása
			pMenu->menuItemsCnt++; //+1 menüelem van már
		}

		/**
		 * A kiválasztott menüelem pointerének elkérése
		 */
		MenuItem_t *getSelectedMenuItemPtr() {
			return pCurrentMenu->pMenuItems + menuViewport.selectedItem;
		}

};

#endif /* LCDMENU_H_ */
