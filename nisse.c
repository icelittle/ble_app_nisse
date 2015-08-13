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

#include <stdio.h>

#include "app_button.h"
#include "app_error.h"
#include "app_gpiote.h"
#include "app_timer.h"

#include "nisse.h"

#define SCAN_INTERVAL           APP_TIMER_TICKS(12, APP_TIMER_PRESCALER)
#define MAX_ROW                 8
#define MAX_COLUMN              12
#define MAX_REPORTS             16  // Number of HID reports to be kept in the report ring buffer

typedef struct
{
    uint8_t         report[INPUT_REPORT_KEYS_MAX_LEN];
} report_entry_t;

typedef struct
{
    report_entry_t  ring[MAX_REPORTS];
    report_entry_t* rp;
    report_entry_t* wp;
} report_buffer_t;

static const uint8_t            row_pins[MAX_ROW] = { 3, 4, 6, 5, 13, 31, 29, 30 };
static const uint8_t            column_pins[MAX_COLUMN] = { 16, 15, 12, 10, 11, 7, 1, 0, 2, 23, 21, 28 };

static app_gpiote_user_id_t     m_gpiote_user_id;
static app_timer_id_t           m_detection_delay_timer_id;
static bsp_event_callback_t     m_registered_callback = NULL;
static bsp_event_t              m_event = BSP_EVENT_NOTHING;
static int8_t                   m_xmit = XMIT_NONE;
static uint8_t                  m_input_report[INPUT_REPORT_KEYS_MAX_LEN];
static int8_t                   m_delay = MAX_DELAY;
static report_buffer_t          m_report_buffer;

void report_buffer_init(void)
{
    m_report_buffer.rp = m_report_buffer.wp = m_report_buffer.ring;
    for (report_entry_t* p = m_report_buffer.ring; p < &m_report_buffer.ring[MAX_REPORTS]; ++p)
        memset(p->report, 0, INPUT_REPORT_KEYS_MAX_LEN);
}

bool report_buffer_empty(void)
{
    return m_report_buffer.rp == m_report_buffer.wp;
}

bool report_buffer_full(void)
{
    return (m_report_buffer.wp + 1 == m_report_buffer.rp) ||
           (m_report_buffer.wp + 1 == &m_report_buffer.ring[MAX_REPORTS] && m_report_buffer.rp == m_report_buffer.ring);
}

bool push_report(const uint8_t* report)
{
    if (report_buffer_full())
        return false;

    memmove(m_report_buffer.wp->report, report, INPUT_REPORT_KEYS_MAX_LEN);
    if (++m_report_buffer.wp == &m_report_buffer.ring[MAX_REPORTS])
        m_report_buffer.wp = m_report_buffer.ring;
    return true;
}

bool pop_report(void)
{
    if (report_buffer_empty())
        return false;

    if (++m_report_buffer.rp == &m_report_buffer.ring[MAX_REPORTS])
        m_report_buffer.rp = m_report_buffer.ring;
    return true;
}

bool peek_report(uint8_t* report)
{
    if (report_buffer_empty())
        return false;

    memmove(report, m_report_buffer.rp->report, INPUT_REPORT_KEYS_MAX_LEN);
    return true;
}

uint32_t send_next_report(ble_hids_t* p_hids)
{
    static uint8_t  report[INPUT_REPORT_KEYS_MAX_LEN];
    uint32_t        err_code;

    if (!peek_report(report))
        return NRF_ERROR_NOT_FOUND;

    if (in_boot_mode())
        err_code = ble_hids_boot_kb_inp_rep_send(p_hids, INPUT_REPORT_KEYS_MAX_LEN, report);
    else
        err_code = ble_hids_inp_rep_send(p_hids, INPUT_REPORT_KEYS_INDEX, INPUT_REPORT_KEYS_MAX_LEN, report);

    if (err_code != BLE_ERROR_NO_TX_BUFFERS)
        pop_report();
    return err_code;
}

static bool scan(void)
{
    bool result = false;
    int8_t row;
    uint8_t column;

    while (m_xmit == XMIT_IN_ORDER) {
        if (!push_report(m_input_report))
            break;
        if (!m_input_report[2] && !peekMacro())
            m_xmit = XMIT_NONE;
        else if (m_input_report[2] && m_input_report[2] == peekMacro())
            m_input_report[2] = 0;    // BRK
        else
            m_input_report[2] = getMacro();
    }

    if (m_xmit != XMIT_IN_ORDER) {
        for (row = MAX_ROW - 1; 0 <= row; --row) {
            nrf_gpio_cfg_input(row_pins[row], NRF_GPIO_PIN_NOPULL);
        }

        for (row = MAX_ROW - 1; 0 <= row; --row) {
            nrf_gpio_cfg_output(row_pins[row]);

            // Add a short delay before scanning columns.
            for (column = 0; column < MAX_COLUMN; ++column) {
                nrf_gpio_pin_read(column_pins[column]);
            }

            for (column = 0; column < MAX_COLUMN; ++column) {
                if (!nrf_gpio_pin_read(column_pins[column])) {
                    result = true;
                    onPressed(row, column);
                }
            }
            nrf_gpio_cfg_input(row_pins[row], NRF_GPIO_PIN_NOPULL);
        }
        for (row = MAX_ROW - 1; 0 <= row; --row) {
            nrf_gpio_cfg_output(row_pins[row]);
        }

        m_xmit = makeReport(m_input_report);
        switch (m_xmit) {
        case XMIT_BRK:
            memset(m_input_report + 2, 0, 6);
            break;
        case XMIT_NORMAL:
            break;
        case XMIT_IN_ORDER:
            for (unsigned char i = 2; i < 8; ++i)
                emitKey(m_input_report[i]);
            m_input_report[2] = beginMacro(6);
            memset(m_input_report + 3, 0, 5);
            break;
        case XMIT_MACRO:
            m_xmit = XMIT_IN_ORDER;
            m_input_report[0] = 0;
            m_input_report[2] = beginMacro(MAX_MACRO_SIZE);
            memset(m_input_report + 3, 0, 5);
            break;
        default:
            break;
        }
    }

    do {
        if (m_xmit && !push_report(m_input_report))
            break;
        if (m_xmit == XMIT_IN_ORDER) {
            if (!m_input_report[2] && !peekMacro())
                m_xmit = XMIT_NONE;
            else if (m_input_report[2] && m_input_report[2] == peekMacro())
                m_input_report[2] = 0;    // BRK
            else
                m_input_report[2] = getMacro();
        } else
            break;
    } while (m_xmit == XMIT_IN_ORDER);

    process_output_report(getLED());

    return result || !report_buffer_empty();
}

static void gpiote_event_handler(uint32_t event_pins_low_to_high, uint32_t event_pins_high_to_low)
{
    app_gpiote_user_disable(m_gpiote_user_id);
    app_timer_stop(m_detection_delay_timer_id);
    scan();
    app_timer_start(m_detection_delay_timer_id, SCAN_INTERVAL, NULL);
    m_delay = MAX_DELAY;
}

static void detection_delay_timeout_handler(void* p_context)
{
    if (!scan())
        --m_delay;
    if (m_delay <= 0) {
        app_timer_stop(m_detection_delay_timer_id);
        app_gpiote_user_enable(m_gpiote_user_id);
    }
    send_next_report(&m_hids);
    if (m_event != BSP_EVENT_NOTHING && m_registered_callback) {
        m_registered_callback(m_event);
        m_event = BSP_EVENT_NOTHING;
    }
}

void set_event(bsp_event_t event)
{
    m_event = event;
}

void buttons_init(bsp_event_callback_t callback)
{
    initKeyboard();

    m_registered_callback = callback;

    app_timer_create(&m_detection_delay_timer_id,
                     APP_TIMER_MODE_REPEATED,
                     detection_delay_timeout_handler);

    for (int8_t row = MAX_ROW - 1; 0 <= row; --row) {
        nrf_gpio_pin_clear(row_pins[row]);
        nrf_gpio_cfg_output(row_pins[row]);
    }

    uint32_t pins_transition_mask = 0;
    for (uint8_t column = 0; column < MAX_COLUMN; ++column) {
        nrf_gpio_cfg_input(column_pins[column], NRF_GPIO_PIN_PULLUP);
        pins_transition_mask |= (1u << column_pins[column]);
    }

    app_gpiote_user_register(&m_gpiote_user_id,
                             pins_transition_mask,
                             pins_transition_mask,
                             gpiote_event_handler);
    app_gpiote_user_enable(m_gpiote_user_id);
}

bool button_is_pushed(uint8_t row, uint8_t column)
{
    int8_t r;
    bool result;

    for (r = MAX_ROW - 1; 0 <= r; --r) {
        nrf_gpio_cfg_input(row_pins[r], NRF_GPIO_PIN_NOPULL);
    }
    nrf_gpio_cfg_output(row_pins[row]);
    result = !nrf_gpio_pin_read(column_pins[column]);
    for (r = MAX_ROW - 1; 0 <= r; --r) {
        nrf_gpio_cfg_output(row_pins[r]);
    }
    return result;
}

void process_output_report(uint8_t report)
{
    uint8_t led = controlLED(report);

    if (led & LED_NUM_LOCK)
        nrf_gpio_pin_set(LED_0);
    else
        nrf_gpio_pin_clear(LED_0);

    if (led & LED_CAPS_LOCK)
        nrf_gpio_pin_set(LED_1);
    else
        nrf_gpio_pin_clear(LED_1);

    if (led & LED_SCROLL_LOCK)
        nrf_gpio_pin_set(LED_2);
    else
        nrf_gpio_pin_clear(LED_2);
}

void app_context_load(dm_handle_t const * p_handle)
{
    initKeyboard();
    m_xmit = XMIT_NONE;
    report_buffer_init();
}
