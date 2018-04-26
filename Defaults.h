/*
 * Defaults.h
 *
 *  Created on: 2018. ápr. 15.
 *      Author: BT
 */

#ifndef DEFAULTS_H_
#define DEFAULTS_H_

#define DEF_CONTRAST				40 		/* LCD kontraszt */
#define DEF_PULSE_COUNT_WELD_MODE	true	/* pulzusszámlálásos hegesztési mód */
#define DEF_BEEP_STATE				false  	/* Beep */
#define DEF_BACKLIGHT_STATE			false 	/* LCD háttérvilágítás */

///  1 periódus: 100Hz -> 10ms
#define DEF_PREWELD_PULSE_CNT 		2		/* 20ms */
#define DEF_PAUSE_PULSE_CNT 		50	 	/* 500ms */
#define DEF_WELD_PULSE_CNT 			20 		/* 200ms */


#define DEF_MOT_TEMP_ALARM 			70 		/* MOT hõmérséklet riasztás */

#endif /* DEFAULTS_H_ */
