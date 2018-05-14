/*
 * Defaults.h
 *
 *  Created on: 2018. ápr. 15.
 *      Author: bt-soft
 */

#ifndef DEFAULTS_H_
#define DEFAULTS_H_

#define DEF_CONTRAST					40 		/* LCD kontraszt */
#define DEF_BIAS						4 		/* LCD elõfeszítés */

#define DEF_PULSE_COUNT_WELD_MODE		true	/* pulzusszámlálásos hegesztési mód */
#define DEF_BEEP_STATE					true  	/* Beep */
#define DEF_BACKLIGHT_STATE				true 	/* LCD háttérvilágítás */

///  1 periódus: 100Hz -> 10ms
#define DEF_PREWELD_PULSE_CNT 			2		/* 20ms */
#define DEF_PAUSE_PULSE_CNT 			5	 	/* 50ms */
#define DEF_WELD_PULSE_CNT 				4 		/* 40ms */

#define DEF_MOT_TEMP_ALARM 				50 		/* MOT hõmérséklet riasztás bekapcsolási határ °C-ban */

#define SYSTEM_FREQUENCY				50.0	/* A ~230V-os hálózat frekvenciája Hz-ben */

//számított hálózati periódus idõ
#define SYSTEM_PERIOD_TIME				1.0/SYSTEM_FREQUENCY

#endif /* DEFAULTS_H_ */
