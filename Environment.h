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
 * Environment.h
 *
 *  Created on: 2018. m�j. 6.
 *      Author: bt-soft
 */

#ifndef ENVIRONMENT_H_
#define ENVIRONMENT_H_

//Serial konzol debug ON
//#define SERIAL_DEBUG
#ifdef SERIAL_DEBUG
#define SERIAL_BAUD_RATE 9600
#endif

// Digit�lis (DS18B30) a h�m�r� szenzor? Ha LM335, akkor ki kell kommentezni
//#define USE_DIGITAL_TEMPERATURE_SENSOR

#endif /* ENVIRONMENT_H_ */
