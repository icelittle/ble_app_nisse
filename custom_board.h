/*
 * Copyright 2015 Esrille Inc. All Rights Reserved.
 *
 * This file is supplied to you for use solely and exclusively on the
 * Esrille New Keyboard - NISSE from Esrille Inc.
 *
 * Thie file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the file NOTICE
 * for copying permission.
 */

#ifndef CUSTOM_H__
#define CUSTOM_H__

#define LED_0          22   // L1
#define LED_1          20   // L2
#define LED_2          18   // L3

#define BSP_LED_0      LED_0
#define BSP_LED_1      LED_1
#define BSP_LED_2      LED_2

#define LEDS_LIST {LED_0, LED_1, LED_2}

#define BUTTONS_NUMBER 0
#define LEDS_NUMBER    3
#define BUTTONS_MASK   0x00000000
#define LEDS_MASK      (BSP_LED_0_MASK | BSP_LED_1_MASK | BSP_LED_2_MASK)
#define LEDS_INV_MASK  0x00000000

#define BSP_LED_0_MASK    (1<<LED_0)
#define BSP_LED_1_MASK    (1<<LED_1)
#define BSP_LED_2_MASK    (1<<LED_2)

#define RX_PIN_NUMBER  9
#define TX_PIN_NUMBER  8
#define CTS_PIN_NUMBER 24       // Not used
#define RTS_PIN_NUMBER 25       // Not used
#define HWFC           false

#endif  // CUSTOM_H__
