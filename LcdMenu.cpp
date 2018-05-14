/*
 * LcdMenu.cpp
 *
 *  Created on: 2018. ápr. 26.
 *      Author: bt-soft
 */

#include "LcdMenu.h"
#include "Buzzer.h"

/**
 * Konstruktor
 */
LcdMenu::LcdMenu(void) {

	//Software SPI
	// D9 - BackLight LED
	// D8 - Serial clock out (SCLK)
	// D7 - Serial data out (DIN)
	// D6 - Data/Command select (D/C)
	// D5 - LCD chip select (CS)
	// D4 - LCD reset (RST)

	nokia5110Display = new Nokia5110DisplayWrapper(PIN_LCD_SCLK, PIN_LCD_DIN, PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_RST, PIN_LCD_BLACKLIGHT);

	//--- Háttérvilágítás kezelése
	nokia5110Display->setBlackLightState(pConfig->configVars.blackLightState); //háttérvilágítás beállítása a konfig szerint

	//Menüelemek inicializálása
	initMenuItems();
	resetMenu();
}

/**
 * Splash képernyõ kirajzolása
 */
void LcdMenu::drawSplashScreen(void) {
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
	menuItems[5] = {"LCD contrast", BYTE, &pConfig->configVars.contrast, 0, 127, &LcdMenu::lcdContrastCallBack};
	//menuItems[6] = {"LCD bias", BYTE, &pConfig->configVars.bias, 0, 7, &LcdMenu::lcdBiasCallBack};
	menuItems[6] = {"LCD light", BOOL, &pConfig->configVars.blackLightState, 0, 1, &LcdMenu::lcdBackLightCallBack};
	menuItems[7] = {"Beep", BOOL, &pConfig->configVars.beepState, 0, 1, &LcdMenu::beepStateCallBack};
	menuItems[8] = {"Fctry reset", FUNCT, NULL, 0, 0, &LcdMenu::factoryResetCallBack};
	menuItems[9] = {"Exit menu", FUNCT, NULL, 0, 0, &LcdMenu::exitCallBack};
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
	nokia5110Display->setCursor(abs(*pCurrentMotTemp) > 99.9 ? 0 : 10, 32);
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
			if (SYSTEM_PERIOD_TIME > 0.0) {
				nokia5110Display->setCursor(35, 40);
				nokia5110Display->print(msecToStr(*(byte *) p.valuePtr));
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
 * LCD Kontraszt kezelése
 */
void LcdMenu::lcdContrastCallBack(void) {
	nokia5110Display->setContrast(pConfig->configVars.contrast);
	pConfig->wantSaveConfig = true;
}

/**
 * LCD Elõfeszítés kezelése
 */
void LcdMenu::lcdBiasCallBack(void) {
	nokia5110Display->setBias(pConfig->configVars.bias);
	pConfig->wantSaveConfig = true;
}

/**
 * LCD Háttérvilágítás kezelése
 */
void LcdMenu::lcdBackLightCallBack(void) {
	nokia5110Display->setBlackLightState(pConfig->configVars.blackLightState);
	pConfig->wantSaveConfig = true;
}

/**
 * Beep kezelése
 */
void LcdMenu::beepStateCallBack(void) {
	if (pConfig->configVars.beepState) {
		pBuzzer->buzzerMenu();
	}
	pConfig->wantSaveConfig = true;
}

/**
 * Gyári beállítások visszaállítása
 */
void LcdMenu::factoryResetCallBack(void) {
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
	lcdContrastCallBack();
	lcdBackLightCallBack();
	beepStateCallBack();

	//menü alapállapotba
	resetMenu();
	menuState = FORCE_MAIN_DISPLAY;
}

/**
 * Kilépés a menübõl
 */
void LcdMenu::exitCallBack(void) {

	nokia5110Display->clearDisplay();
	nokia5110Display->setTextColor(BLACK, WHITE);
	nokia5110Display->setTextSize(1);
	nokia5110Display->setCursor(0, 20);
	nokia5110Display->print("Return...");
	nokia5110Display->display();

	//menü alapállapotba
	resetMenu();
	menuState = FORCE_MAIN_DISPLAY; //Kilépünk a menübõl
}

// ------------------------------------------------------ utils
/**
 * msec -> String konverter
 */
String LcdMenu::msecToStr(long pulseCnt) {

	//A ZCD miatt SYSTEM_FREQUENCY-n (100Hz vagy 120Hz-en) vagyunk
	long fullPulseTime = (SYSTEM_PERIOD_TIME / 2) * 1000 * pulseCnt;

	byte sec = fullPulseTime / 1000;
	int msec = fullPulseTime - (sec * 1000);
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
