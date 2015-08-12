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

#ifndef NISSE_H__
#define NISSE_H__

#include <stdint.h>
#include <string.h>

#include "nrf.h"
#include "nrf_assert.h"
#include "device_manager.h"
#include "ble_srv_common.h"
#include "ble_hids.h"
#include "bsp.h"

#include "switch.h"
#include "Keyboard.h"

#define APP_TIMER_PRESCALER             0
#define APP_TIMER_MAX_TIMERS            (4+BSP_APP_TIMERS_NUMBER+1)
#define APP_TIMER_OP_QUEUE_SIZE         4

#define INPUT_REPORT_KEYS_INDEX         0
#define INPUT_REPORT_KEYS_MAX_LEN       8

//
// HID report ring buffer
//
void report_buffer_init(void);
bool report_buffer_empty(void);
bool report_buffer_full(void);
bool push_report(const uint8_t* report);
bool pop_report(void);
bool peek_report(uint8_t* report);

//
// HID helper
//
extern bool in_boot_mode();
uint32_t send_next_report(ble_hids_t* p_hids);
void process_output_report(uint8_t report);

//
// BSP helper
//
void set_event(bsp_event_t event);
void app_context_load(dm_handle_t const * p_handle);

//
// Keyboard key matrix
//
void buttons_init(bsp_event_callback_t callback);
bool button_is_pushed(uint8_t row, uint8_t column);

//
// Battery helper
//
uint32_t battery_voltage_get(void);

extern ble_hids_t   m_hids;                 // Structure used to identify the HID service.
extern dm_handle_t  m_bonded_peer_handle;   // Device reference handle to the current bonded central.

#endif // NISSE_H__
