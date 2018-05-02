/*
 * LcdMenu.cpp
 *
 *  Created on: 2018. �pr. 26.
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
	nokia5110Display->setContrast(pConfig->configVars.contrast);	//kontraszt be�ll�t�sa
	nokia5110Display->setBlackLightPin(PIN_LCD_BLACKLIGHT);	//h�tt�rvil�g�t�s PIN be�ll�t�sa
	nokia5110Display->setBlackLightState(pConfig->configVars.blackLightState);	//h�tt�rvil�g�t�s be�ll�t�sa a konfig szerint

	//Men�elemek inicializ�l�sa
	initMenuItems();
	resetMenu();
}

/**
 * Splash k�perny� kirajzol�sa
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
 * Men� felh�z�sa
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
 * Men� alap�llapotba
 */
void LcdMenu::resetMenu(void) {
	//viewPort be�ll�t�s
	menuViewport.firstItem = 0;
	menuViewport.lastItem = MENU_VIEWPORT_SIZE - 1;
	menuViewport.selectedItem = 0;
}

/**
 * H�m�rs�klet ki�r�sa a main- �s az alarm screen-n�l
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

	//Pulzussz�ml�l� m�d
	if (pConfig->configVars.pulseCountWeldMode) {
		nokia5110Display->println("Pwld  Paus Wld");
		sprintf(tempBuff, "%-3d   %-3d  %-3d", pConfig->configVars.preWeldPulseCnt, pConfig->configVars.pausePulseCnt, pConfig->configVars.weldPulseCnt);
		nokia5110Display->println(tempBuff);
	} else { //k�zi hegeszt�s
		nokia5110Display->setTextColor(WHITE, BLACK);
		nokia5110Display->println("Manual welding");
		nokia5110Display->setTextColor(BLACK, WHITE);
	}

	//H�m�rs�klet ki�r�sa
	nokia5110Display->setCursor(0, 22);
	nokia5110Display->print("MOT Temp:");
	this->drawTempValue(pCurrentMotTemp);

	nokia5110Display->display();
}

/**
 * Magas h�m�rs�klet riaszt�s
 */
void LcdMenu::drawWarningDisplay(float *pCurrentMotTemp) {
	nokia5110Display->clearDisplay();
	nokia5110Display->setTextColor(BLACK);
	nokia5110Display->setTextSize(1);

	nokia5110Display->println("!!!!!!!!!!!!!!");
	nokia5110Display->println(" MOT Temp is");
	nokia5110Display->println("  too high!");
	nokia5110Display->println("!!!!!!!!!!!!!!");

	//H�m�rs�klet ki�r�sa
	this->drawTempValue(pCurrentMotTemp);

	nokia5110Display->display();
}

/**
 * MainMenu kirajzol�sa
 *  - Men�felirat
 *  - viewportban l�that� elemek megjeeln�t�se
 *  - kiv�lasztott elem h�tter�nek megv�ltoztat�sa
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
 * men�elem be�ll�t� k�perny�
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

	//T�pus szerinti ki�r�s
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

	//M�rt�kegys�g
	nokia5110Display->setTextSize(1);
	switch (p.valueType) {

		case TEMP: //�C ki�r�s
			nokia5110Display->setCursor(55, 30);
			sprintf(tempBuff, "%cC", DEGREE_SYMBOL_CODE);
			nokia5110Display->print(tempBuff);
			break;

		case PULSE: //msec ki�r�s
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
 * Men�ben lefel� l�pked�s
 */
void LcdMenu::stepDown(void) {
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
}

/**
 * Men�ben lefel� l�pked�s
 */
void LcdMenu::stepUp(void) {
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

}

/**
 * A kiv�lasztott men�elem �rt�k�nek n�vel�se
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
			if (!*(bool *) pSelectedMenuItem->valuePtr) { //ha most false, akkor true-t csin�lunk bel�le
				*(bool *) pSelectedMenuItem->valuePtr = true;
			}
			break;
	}
}
/**
 * A kiv�lasztott men�elem �rt�k�nek cs�kkent�se
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
			if (*(bool *) pSelectedMenuItem->valuePtr) { //ha most true, akkor false-t csin�lunk bel�le
				*(bool *) pSelectedMenuItem->valuePtr = false;
			}
			break;
	}
}

/**
 * Men�elemhez tartoz� seg�df�ggv�ny megh�v�sa - ha van
 */
void LcdMenu::invokeMenuItemCallBackFunct(void) {

	MenuItemT *pSelectedMenuItem = this->getSelectedItemPtr();
	if (pSelectedMenuItem->callbackFunct != NULL) {
		(this->*(pSelectedMenuItem->callbackFunct))();
	}
}

//------------------------------------------------------------------- Men�elemek callback f�ggv�nyei
/**
 * Kontraszt kezel�se
 */
void LcdMenu::menuLcdContrast(void) {
	nokia5110Display->setContrast(pConfig->configVars.contrast);
	pConfig->wantSaveConfig = true;
}

/**
 * H�tt�rvil�g�t�s kezel�se
 */
void LcdMenu::menuLcdBackLight(void) {
	nokia5110Display->setBlackLightState(pConfig->configVars.blackLightState);
	pConfig->wantSaveConfig = true;
}

/**
 * Beep kezel�se
 */
void LcdMenu::menuBeepState(void) {
	if (pConfig->configVars.beepState) {
		pBuzzer->buzzerMenu();
	}
	pConfig->wantSaveConfig = true;
}

/**
 * Gy�ri be�ll�t�sok vissza�ll�t�sa
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

	//Default konfig l�trehoz�sa
	pConfig->createDefaultConfig();
	pConfig->wantSaveConfig = true;

	//Konfig be�ll�t�sok �rv�nyes�t�se
	menuLcdContrast();
	menuLcdBackLight();
	menuBeepState();

	//men� alap�llapotba
	resetMenu();
	menuState = FORCE_MAIN_DISPLAY;
}

/**
 * Kil�p�s a mn�b�l
 */
void LcdMenu::menuExit(void) {

	nokia5110Display->clearDisplay();
	nokia5110Display->setTextColor(BLACK, WHITE);
	nokia5110Display->setTextSize(1);
	nokia5110Display->setCursor(0, 20);
	nokia5110Display->print("Exit from menu");
	nokia5110Display->display();

	//men� alap�llapotba
	resetMenu();
	menuState = FORCE_MAIN_DISPLAY; //Kil�p�nk a men�b�l
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
