/*
 * Copyright 2015 Esrille Inc.
 *
 * This file is a modified version of bsp/bsp.c provided by
 * Nordic Semiconductor for using Esrille New Keyboard.
 */

/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include "bsp.h"
#include <stddef.h>
#include <stdio.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_error.h"

#ifndef BSP_SIMPLE
#include "app_timer.h"
#include "app_button.h"
#endif // BSP_SIMPLE

#include "nisse.h"

#define ADVERTISING_LED_ON_INTERVAL            200
#define ADVERTISING_LED_OFF_INTERVAL           1800

#define ADVERTISING_DIRECTED_LED_ON_INTERVAL   200
#define ADVERTISING_DIRECTED_LED_OFF_INTERVAL  200

#define ADVERTISING_WHITELIST_LED_ON_INTERVAL  200
#define ADVERTISING_WHITELIST_LED_OFF_INTERVAL 800

#define ADVERTISING_SLOW_LED_ON_INTERVAL       400
#define ADVERTISING_SLOW_LED_OFF_INTERVAL      4000

#define BONDING_INTERVAL                       100

#define SENT_OK_INTERVAL                       100
#define SEND_ERROR_INTERVAL                    500

#define RCV_OK_INTERVAL                        100
#define RCV_ERROR_INTERVAL                     500

#define ALERT_INTERVAL                         200

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)
static bsp_indication_t m_stable_state        = BSP_INDICATE_IDLE;
static uint32_t         m_app_ticks_per_100ms = 0;
static uint32_t         m_indication_type     = 0;
static app_timer_id_t   m_leds_timer_id;
static app_timer_id_t   m_alert_timer_id;

static uint32_t         m_leds_mask           = 0;

#endif // LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

#define BSP_MS_TO_TICK(MS) (m_app_ticks_per_100ms * (MS / 100))

#ifdef BSP_LED_2_MASK
#define ALERT_LED_MASK BSP_LED_2_MASK
#else
#define ALERT_LED_MASK BSP_LED_1_MASK
#endif // LED_2_MASK


#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)
/**@brief       Configure leds to indicate required state.
 * @param[in]   indicate   State to be indicated.
 */
static uint32_t bsp_led_indication(bsp_indication_t indicate)
{
    uint32_t err_code   = NRF_SUCCESS;
    uint32_t next_delay = 0;

    m_leds_mask = 0;
    switch (switch_get_current()) {
    case 0:
        m_leds_mask = BSP_LED_0_MASK;
        break;
    case 1:
        m_leds_mask = BSP_LED_1_MASK;
        break;
    case 2:
        m_leds_mask = BSP_LED_2_MASK;
        break;
    }

    switch (indicate)
    {
        case BSP_INDICATE_IDLE:
            LEDS_OFF(LEDS_MASK & ~ALERT_LED_MASK);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_SCANNING:
        case BSP_INDICATE_ADVERTISING:
            LEDS_OFF(LEDS_MASK & ~m_leds_mask & ~ALERT_LED_MASK);

            // in advertising blink LED_0
            if (LED_IS_ON(m_leds_mask))
            {
                LEDS_OFF(m_leds_mask);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING ? ADVERTISING_LED_OFF_INTERVAL :
                             ADVERTISING_SLOW_LED_OFF_INTERVAL;
            }
            else
            {
                LEDS_ON(m_leds_mask);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING ? ADVERTISING_LED_ON_INTERVAL :
                             ADVERTISING_SLOW_LED_ON_INTERVAL;
            }

            m_stable_state = indicate;
            err_code       = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(next_delay), NULL);
            break;

        case BSP_INDICATE_ADVERTISING_WHITELIST:
            LEDS_OFF(LEDS_MASK & ~m_leds_mask & ~ALERT_LED_MASK);

            // in advertising quickly blink LED_0
            if (LED_IS_ON(m_leds_mask))
            {
                LEDS_OFF(m_leds_mask);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_WHITELIST ?
                             ADVERTISING_WHITELIST_LED_OFF_INTERVAL :
                             ADVERTISING_SLOW_LED_OFF_INTERVAL;
            }
            else
            {
                LEDS_ON(m_leds_mask);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_WHITELIST ?
                             ADVERTISING_WHITELIST_LED_ON_INTERVAL :
                             ADVERTISING_SLOW_LED_ON_INTERVAL;
            }
            m_stable_state = indicate;
            err_code       = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(next_delay), NULL);
            break;

        case BSP_INDICATE_ADVERTISING_SLOW:
            LEDS_OFF(LEDS_MASK & ~m_leds_mask & ~ALERT_LED_MASK);

            // in advertising slowly blink LED_0
            if (LED_IS_ON(m_leds_mask))
            {
                LEDS_OFF(m_leds_mask);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_SLOW ? ADVERTISING_SLOW_LED_OFF_INTERVAL :
                             ADVERTISING_SLOW_LED_OFF_INTERVAL;
            }
            else
            {
                LEDS_ON(m_leds_mask);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_SLOW ? ADVERTISING_SLOW_LED_ON_INTERVAL :
                             ADVERTISING_SLOW_LED_ON_INTERVAL;
            }
            m_stable_state = indicate;
            err_code       = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(next_delay), NULL);
            break;

        case BSP_INDICATE_ADVERTISING_DIRECTED:
            LEDS_OFF(LEDS_MASK & ~m_leds_mask & ~ALERT_LED_MASK);

            // in advertising very quickly blink LED_0
            if (LED_IS_ON(m_leds_mask))
            {
                LEDS_OFF(m_leds_mask);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_DIRECTED ?
                             ADVERTISING_DIRECTED_LED_OFF_INTERVAL :
                             ADVERTISING_SLOW_LED_OFF_INTERVAL;
            }
            else
            {
                LEDS_ON(m_leds_mask);
                next_delay = indicate ==
                             BSP_INDICATE_ADVERTISING_DIRECTED ?
                             ADVERTISING_DIRECTED_LED_ON_INTERVAL :
                             ADVERTISING_SLOW_LED_ON_INTERVAL;
            }
            m_stable_state = indicate;
            err_code       = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(next_delay), NULL);
            break;

        case BSP_INDICATE_BONDING:
            LEDS_OFF(LEDS_MASK & ~m_leds_mask & ~ALERT_LED_MASK);

            // in bonding fast blink LED_0
            if (LED_IS_ON(m_leds_mask))
            {
                LEDS_OFF(m_leds_mask);
            }
            else
            {
                LEDS_ON(m_leds_mask);
            }

            m_stable_state = indicate;
            err_code       =
                app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(BONDING_INTERVAL), NULL);
            break;

        case BSP_INDICATE_CONNECTED:
            LEDS_OFF(LEDS_MASK & ~m_leds_mask & ~ALERT_LED_MASK);
            LEDS_ON(m_leds_mask);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_SENT_OK:
            // when sending shortly invert LED_1
            LEDS_INVERT(BSP_LED_1_MASK);
            err_code = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(SENT_OK_INTERVAL), NULL);
            break;

        case BSP_INDICATE_SEND_ERROR:
            // on receving error invert LED_1 for long time
            LEDS_INVERT(BSP_LED_1_MASK);
            err_code = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(SEND_ERROR_INTERVAL), NULL);
            break;

        case BSP_INDICATE_RCV_OK:
            // when receving shortly invert LED_1
            LEDS_INVERT(BSP_LED_1_MASK);
            err_code = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(RCV_OK_INTERVAL), NULL);
            break;

        case BSP_INDICATE_RCV_ERROR:
            // on receving error invert LED_1 for long time
            LEDS_INVERT(BSP_LED_1_MASK);
            err_code = app_timer_start(m_leds_timer_id, BSP_MS_TO_TICK(RCV_ERROR_INTERVAL), NULL);
            break;

        case BSP_INDICATE_FATAL_ERROR:
            // on fatal error turn on all leds
            LEDS_ON(LEDS_MASK);
            break;

        case BSP_INDICATE_ALERT_0:
        case BSP_INDICATE_ALERT_1:
        case BSP_INDICATE_ALERT_2:
        case BSP_INDICATE_ALERT_3:
        case BSP_INDICATE_ALERT_OFF:
            err_code   = app_timer_stop(m_alert_timer_id);
            next_delay = (uint32_t)BSP_INDICATE_ALERT_OFF - (uint32_t)indicate;

            // a little trick to find out that if it did not fall through ALERT_OFF
            if (next_delay && (err_code == NRF_SUCCESS))
            {
                if (next_delay > 1)
                {
                    err_code = app_timer_start(m_alert_timer_id, BSP_MS_TO_TICK(
                                                   (next_delay * ALERT_INTERVAL)), NULL);
                }
                LEDS_ON(ALERT_LED_MASK);
            }
            else
            {
                LEDS_OFF(ALERT_LED_MASK);
            }
            break;

        case BSP_INDICATE_USER_STATE_OFF:
            LEDS_OFF(LEDS_MASK);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_USER_STATE_0:
            LEDS_OFF(LEDS_MASK & ~m_leds_mask);
            LEDS_ON(m_leds_mask);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_USER_STATE_1:
            LEDS_OFF(LEDS_MASK & ~BSP_LED_1_MASK);
            LEDS_ON(BSP_LED_1_MASK);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_USER_STATE_2:
            LEDS_OFF(LEDS_MASK & ~(m_leds_mask | BSP_LED_1_MASK));
            LEDS_ON(m_leds_mask | BSP_LED_1_MASK);
            m_stable_state = indicate;
            break;

        case BSP_INDICATE_USER_STATE_3:

        case BSP_INDICATE_USER_STATE_ON:
            LEDS_ON(LEDS_MASK);
            m_stable_state = indicate;
            break;

        default:
            break;
    }

    return err_code;
}


/**@brief Handle events from leds timer.
 *
 * @note Timer handler does not support returning an error code.
 * Errors from bsp_led_indication() are not propagated.
 *
 * @param[in]   p_context   parameter registered in timer start function.
 */
static void leds_timer_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    if (m_indication_type & BSP_INIT_LED)
    {
        UNUSED_VARIABLE(bsp_led_indication(m_stable_state));
    }
}


/**@brief Handle events from alert timer.
 *
 * @param[in]   p_context   parameter registered in timer start function.
 */
static void alert_timer_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    LEDS_INVERT(ALERT_LED_MASK);
}
#endif // #if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)


/**@brief Configure indicators to required state.
 */
uint32_t bsp_indication_set(bsp_indication_t indicate)
{
    uint32_t err_code = NRF_SUCCESS;

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

    if (m_indication_type & BSP_INIT_LED)
    {
        err_code = bsp_led_indication(indicate);
    }

#endif // LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)
    return err_code;
}


uint32_t bsp_init(uint32_t type, uint32_t ticks_per_100ms, bsp_event_callback_t callback)
{
    uint32_t err_code = NRF_SUCCESS;

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)
    m_app_ticks_per_100ms = ticks_per_100ms;
    m_indication_type     = type;
#else
    UNUSED_VARIABLE(ticks_per_100ms);
#endif // LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

#if LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

    if (type & BSP_INIT_LED)
    {
        LEDS_OFF(LEDS_MASK);
        NRF_GPIO->DIRSET = LEDS_MASK;
    }

    // timers module must be already initialized!
    if (err_code == NRF_SUCCESS)
    {
        err_code =
            app_timer_create(&m_leds_timer_id, APP_TIMER_MODE_SINGLE_SHOT, leds_timer_handler);
    }

    if (err_code == NRF_SUCCESS)
    {
        err_code =
            app_timer_create(&m_alert_timer_id, APP_TIMER_MODE_REPEATED, alert_timer_handler);
    }
#endif // LEDS_NUMBER > 0 && !(defined BSP_SIMPLE)

    return err_code;
}
