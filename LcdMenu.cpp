/*
 * LcdMenu.cpp
 *
 *  Created on: 2018. ápr. 26.
 *      Author: u626374
 */

#include "LcdMenu.h"
#include "Buzzer.h"

/**
 * Konstruktor
 */
LcdMenu::LcdMenu(void) {

	//Software SPI (slower updates, more flexible pin options):
	nokia5110Display = new Nokia5110Display(PIN_LCD_SCLK, PIN_LCD_DIN, PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_RST);

	//--- Display
	nokia5110Display->setContrast(pConfig->configVars.contrast);	//kontraszt beállítása
	nokia5110Display->setBlackLightPin(PIN_LCD_BLACKLIGHT);	//háttérvilágítás PIN beállítása
	nokia5110Display->setBlackLightState(pConfig->configVars.blackLightState);	//háttérvilágítás beállítása a konfig szerint

	//Menüelemek inicializálása
	initMenuItems();
	resetMenu();
}

/**
 * Splash képernyõ kirajzolása
 */
void LcdMenu::drawSplashScreen(void) {
	nokia5110Display->begin();
	nokia5110Display->clearDisplay();
	nokia5110Display->setTextSize(1);
	nokia5110Display->setTextColor(BLACK);

	nokia5110Display->println(" Arduino Nano");
	nokia5110Display->println(" Spot Welder");

	nokia5110Display->setCursor(8, 25);
	nokia5110Display->print("v");

	nokia5110Display->setTextSize(2);
	sprintf(tempBuff, "%s", &pConfig->configVars.version);
	nokia5110Display->setCursor(14, 18);
	nokia5110Display->print(tempBuff);

	nokia5110Display->setTextSize(1);
	nokia5110Display->setCursor(0, 40);
	nokia5110Display->println(" BT-Soft 2018");

	nokia5110Display->display();
}

/**
 * Menü felhúzása
 */
void LcdMenu::initMenuItems(void) {

	menuItems[0] = {"Weld mode", WELD, &pConfig->configVars.pulseCountWeldMode, 0, 1, NULL};
	menuItems[1] = {"PreWeld pulse", PULSE, &pConfig->configVars.preWeldPulseCnt, 0, 255, NULL};
	menuItems[2] = {"Pause pulse", PULSE, &pConfig->configVars.pausePulseCnt, 0, 255, NULL};
	menuItems[3] = {"Weld pulse", PULSE, &pConfig->configVars.weldPulseCnt, 1, 255, NULL};
	menuItems[4] = {"MOT T.Alrm", TEMP, &pConfig->configVars.motTempAlarm, 50, 90, NULL};
	menuItems[5] = {"Contrast", BYTE, &pConfig->configVars.contrast, 0, 255, &LcdMenu::menuLcdContrast};
	menuItems[6] = {"Disp light", BOOL, &pConfig->configVars.blackLightState, 0, 1, &LcdMenu::menuLcdBackLight};
	menuItems[7] = {"Beep", BOOL, &pConfig->configVars.beepState, 0, 1, &LcdMenu::menuBeepState};
	menuItems[8] = {"Fctry reset", FUNCT, NULL, 0, 0, &LcdMenu::menuFactoryReset};
	menuItems[9] = {"Exit menu", FUNCT, NULL, 0, 0, &LcdMenu::menuExit};
}

/**
 * Menü alapállapotba
 */
void LcdMenu::resetMenu(void) {
	//viewPort beállítás
	menuViewport.firstItem = 0;
	menuViewport.lastItem = MENU_VIEWPORT_SIZE - 1;
	menuViewport.selectedItem = 0;
}

/**
 * Hõmérséklet kiírása a main- és az alarm screen-nél
 */
void LcdMenu::drawTempValue(float *pCurrentMotTemp) {
	nokia5110Display->setTextSize(2);
	nokia5110Display->setCursor(abs(*pCurrentMotTemp) > 99.9 ? 0 : 18, 32);
	dtostrf(*pCurrentMotTemp, 1, 1, tempBuff);
	nokia5110Display->print(tempBuff);

	nokia5110Display->setTextSize(1);
	nokia5110Display->setCursor(72, 38);
	sprintf(tempBuff, "%cC", DEGREE_SYMBOL_CODE);
	nokia5110Display->print(tempBuff);
}

/**
 * Main screen
 */
void LcdMenu::drawMainDisplay(float *pCurrentMotTemp) {

	nokia5110Display->clearDisplay();
	nokia5110Display->setTextColor(BLACK);
	nokia5110Display->setTextSize(1);

	//Pulzusszámláló mód
	if (pConfig->configVars.pulseCountWeldMode) {
		nokia5110Display->println("Pwld  Paus Wld");
		sprintf(tempBuff, "%-3d   %-3d  %-3d", pConfig->configVars.preWeldPulseCnt, pConfig->configVars.pausePulseCnt, pConfig->configVars.weldPulseCnt);
		nokia5110Display->println(tempBuff);
	} else { //kézi hegesztés
		nokia5110Display->setTextColor(WHITE, BLACK);
		nokia5110Display->println("Manual welding");
		nokia5110Display->setTextColor(BLACK, WHITE);
	}

	//Hõmérséklet kiírása
	nokia5110Display->setCursor(0, 22);
	nokia5110Display->print("MOT Temp:");
	this->drawTempValue(pCurrentMotTemp);

	nokia5110Display->display();
}

/**
 * Magas hõmérséklet riasztás
 */
void LcdMenu::drawWarningDisplay(float *pCurrentMotTemp) {
	nokia5110Display->clearDisplay();
	nokia5110Display->setTextColor(BLACK);
	nokia5110Display->setTextSize(1);

	nokia5110Display->println("!!!!!!!!!!!!!!");
	nokia5110Display->println(" MOT Temp is");
	nokia5110Display->println("  too high!");
	nokia5110Display->println("!!!!!!!!!!!!!!");

	//Hõmérséklet kiírása
	this->drawTempValue(pCurrentMotTemp);

	nokia5110Display->display();
}

/**
 * MainMenu kirajzolása
 *  - Menüfelirat
 *  - viewportban látható elemek megjeelnítése
 *  - kiválasztott elem hátterének megváltoztatása
 */
void LcdMenu::drawMainMenu(void) {

	nokia5110Display->clearDisplay();
	nokia5110Display->setTextSize(1);
	nokia5110Display->setTextColor(BLACK, WHITE);
	nokia5110Display->setCursor(15, 0);
	nokia5110Display->print("MAIN MENU");
	nokia5110Display->drawFastHLine(0, 10, 83, BLACK);

	for (byte i = 0; i < MENU_VIEWPORT_SIZE; i++) {

		byte itemNdx = menuViewport.firstItem + i;

		nokia5110Display->setCursor(0, MENU_VIEVPORT_LINEPOS[i]);

		//selected?
		if (itemNdx == menuViewport.selectedItem) {
			nokia5110Display->setTextColor(WHITE, BLACK);
			nokia5110Display->print(">");
		} else {
			nokia5110Display->setTextColor(BLACK, WHITE);
			nokia5110Display->print(" ");
		}
		nokia5110Display->print(menuItems[itemNdx].title);

	}
	nokia5110Display->display();
}

/**
 * menüelem beállítõ képernyõ
 */
void LcdMenu::drawMenuItemValue() {

	MenuItemT p = menuItems[menuViewport.selectedItem];

	nokia5110Display->clearDisplay();

	nokia5110Display->setTextSize(1);
	nokia5110Display->setTextColor(BLACK, WHITE);
	nokia5110Display->setCursor(5, 0);
	nokia5110Display->print(p.title);
	nokia5110Display->drawFastHLine(0, 10, 83, BLACK);

	nokia5110Display->setCursor(5, 15);

	//Prompt
	switch (p.valueType) {
		case TEMP:
		case BYTE:
		case BOOL:
			nokia5110Display->print("Value");
			break;

		case WELD:
			nokia5110Display->print("Mode");
			break;

		case PULSE:
			nokia5110Display->print("Pulse");
			break;
	}

	nokia5110Display->setTextSize(2);
	nokia5110Display->setCursor(5, 25);

	//Típus szerinti kiírás
	String dspValue = "unknown";
	switch (p.valueType) {
		case BOOL:
			dspValue = *(bool *) p.valuePtr ? "ON" : "OFF";
			break;

		case WELD:
			dspValue = *(bool *) p.valuePtr ? "Pulse" : "Manual";
			break;

		case PULSE:
		case TEMP:
		case BYTE:
			dspValue = String(*(byte *) p.valuePtr);
			break;
	}
	nokia5110Display->print(dspValue);

	//Mértékegység
	nokia5110Display->setTextSize(1);
	switch (p.valueType) {

		case TEMP: //°C kiírás
			nokia5110Display->setCursor(55, 30);
			sprintf(tempBuff, "%cC", DEGREE_SYMBOL_CODE);
			nokia5110Display->print(tempBuff);
			break;

		case PULSE: //msec kiírás
			if (pConfig->spotWelderSystemPeriodTime > 0.0) {
				nokia5110Display->setCursor(35, 40);
				nokia5110Display->print(msecToStr(pConfig->spotWelderSystemPeriodTime * 1000.0 * *(byte *) p.valuePtr));
			}
			break;

		default:
			break;
	}

	nokia5110Display->display();
}

/**
 * Menüben lefelé lépkedés
 */
void LcdMenu::stepDown(void) {
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
}

/**
 * Menüben lefelé lépkedés
 */
void LcdMenu::stepUp(void) {
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

}

/**
 * A kiválasztott menüelem értékének növelése
 */
void LcdMenu::incSelectedValue(void) {
	MenuItemT *pSelectedMenuItem = this->getSelectedItemPtr();

	switch (pSelectedMenuItem->valueType) {
		case LcdMenu::BYTE:
		case LcdMenu::PULSE:
		case LcdMenu::TEMP:
			if (*(byte *) pSelectedMenuItem->valuePtr < pSelectedMenuItem->maxValue) {
				(*(byte *) pSelectedMenuItem->valuePtr)++;
			}
			break;

		case LcdMenu::WELD:
		case LcdMenu::BOOL:
			if (!*(bool *) pSelectedMenuItem->valuePtr) { //ha most false, akkor true-t csinálunk belõle
				*(bool *) pSelectedMenuItem->valuePtr = true;
			}
			break;
	}
}
/**
 * A kiválasztott menüelem értékének csökkentése
 */
void LcdMenu::decSelectedValue(void) {

	MenuItemT *pSelectedMenuItem = this->getSelectedItemPtr();

	switch (pSelectedMenuItem->valueType) {
		case LcdMenu::BYTE:
		case LcdMenu::PULSE:
		case LcdMenu::TEMP:
			if (*(byte *) pSelectedMenuItem->valuePtr > pSelectedMenuItem->minValue) {
				(*(byte *) pSelectedMenuItem->valuePtr)--;
			}
			break;

		case LcdMenu::WELD:
		case LcdMenu::BOOL:
			if (*(bool *) pSelectedMenuItem->valuePtr) { //ha most true, akkor false-t csinálunk belõle
				*(bool *) pSelectedMenuItem->valuePtr = false;
			}
			break;
	}
}

/**
 * Menüelemhez tartozó segédfüggvény meghívása - ha van
 */
void LcdMenu::invokeMenuItemCallBackFunct(void) {

	MenuItemT *pSelectedMenuItem = this->getSelectedItemPtr();
	if (pSelectedMenuItem->callbackFunct != NULL) {
		(this->*(pSelectedMenuItem->callbackFunct))();
	}
}

//------------------------------------------------------------------- Menüelemek callback függvényei
/**
 * Kontraszt kezelése
 */
void LcdMenu::menuLcdContrast(void) {
	nokia5110Display->setContrast(pConfig->configVars.contrast);
	pConfig->wantSaveConfig = true;
}

/**
 * Háttérvilágítás kezelése
 */
void LcdMenu::menuLcdBackLight(void) {
	nokia5110Display->setBlackLightState(pConfig->configVars.blackLightState);
	pConfig->wantSaveConfig = true;
}

/**
 * Beep kezelése
 */
void LcdMenu::menuBeepState(void) {
	if (pConfig->configVars.beepState) {
		pBuzzer->buzzerMenu();
	}
	pConfig->wantSaveConfig = true;
}

/**
 * Gyári beállítások visszaállítása
 */
void LcdMenu::menuFactoryReset(void) {
	nokia5110Display->clearDisplay();
	nokia5110Display->setTextSize(1);
	nokia5110Display->setTextColor(BLACK, WHITE);
	nokia5110Display->setCursor(15, 0);
	nokia5110Display->print("RESET");
	nokia5110Display->drawFastHLine(0, 10, 83, BLACK);
	nokia5110Display->setTextSize(2);
	nokia5110Display->setCursor(5, 25);
	nokia5110Display->print("Ok...");

	nokia5110Display->display();

	//Default konfig létrehozása
	pConfig->createDefaultConfig();
	pConfig->wantSaveConfig = true;

	//Konfig beállítások érvényesítése
	menuLcdContrast();
	menuLcdBackLight();
	menuBeepState();

	//menü alapállapotba
	resetMenu();
	menuState = FORCE_MAIN_DISPLAY;
}

/**
 * Kilépés a mnübõl
 */
void LcdMenu::menuExit(void) {

	nokia5110Display->clearDisplay();
	nokia5110Display->setTextColor(BLACK, WHITE);
	nokia5110Display->setTextSize(1);
	nokia5110Display->setCursor(0, 20);
	nokia5110Display->print("Exit from menu");
	nokia5110Display->display();

	//menü alapállapotba
	resetMenu();
	menuState = FORCE_MAIN_DISPLAY; //Kilépünk a menübõl
}

// ------------------------------------------------------ utils
/**
 * msec -> String konverter
 */
String LcdMenu::msecToStr(long x) {

	byte sec = x / 1000;
	byte msec = x % (sec * 1000);
	String res = "";

	if (sec > 0) {
		res += sec;
		res += "s ";

	}
	if (msec > 0) {
		res += msec;
		res += "ms";
	}

	return res;
}
