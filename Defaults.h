/*
 * Defaults.h
 *
 *  Created on: 2018. �pr. 15.
 *      Author: bt-soft
 */

#ifndef DEFAULTS_H_
#define DEFAULTS_H_

#define DEF_CONTRAST					40 		/* LCD kontraszt */
#define DEF_BIAS						4 		/* LCD el�fesz�t�s */

#define DEF_PULSE_COUNT_WELD_MODE		true	/* pulzussz�ml�l�sos hegeszt�si m�d */
#define DEF_BEEP_STATE					true  	/* Beep */
#define DEF_BACKLIGHT_STATE				true 	/* LCD h�tt�rvil�g�t�s */

///  1 peri�dus: 100Hz -> 10ms
#define DEF_PREWELD_PULSE_CNT 			2		/* 20ms */
#define DEF_PAUSE_PULSE_CNT 			5	 	/* 50ms */
#define DEF_WELD_PULSE_CNT 				4 		/* 40ms */

#define DEF_MOT_TEMP_ALARM 				50 		/* MOT h�m�rs�klet riaszt�s bekapcsol�si hat�r �C-ban */

#define SYSTEM_FREQUENCY				50.0	/* A ~230V-os h�l�zat frekvenci�ja Hz-ben */

//sz�m�tott h�l�zati peri�dus id�
#define SYSTEM_PERIOD_TIME				1.0/SYSTEM_FREQUENCY

#endif /* DEFAULTS_H_ */
