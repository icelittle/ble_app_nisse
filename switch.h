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

#ifndef SWITCH_H__
#define SWITCH_H__

#include "device_manager.h"

//
// Bonding switcher
//
//   The number of bondings to be managed is same as DEVICE_MANAGER_MAX_BONDS.
//   pstorage must be initialized before initializing the switch module.
//   PSTORAGE_NUM_OF_PAGES must be incremented by one.
//
uint32_t switch_init(bool erase_bonds);

uint32_t switch_select(uint8_t index);

uint32_t switch_update(dm_handle_t const* p_handle);

uint32_t switch_reset(dm_handle_t const* p_handle);

uint8_t switch_get_current(void);

uint8_t switch_get_current_device(void);

uint32_t switch_filter(dm_handle_t const* p_handle,
                       dm_event_t const*  p_event,
                       uint16_t           conn_handle);

// Defined in the customized device_manager_peripheral.c
uint32_t switch_whitelist_create(dm_application_instance_t const* p_handle,
                                 ble_gap_whitelist_t            * p_whitelist);

//
// EEPROM emulation for each bonding
//

#define _EEPROMSIZE 12
#define __EEPROM_DATA(a, b, c, d, e, f, g, h) \
    uint8_t eeprom_data[_EEPROMSIZE] = { (a), (b), (c), (d), (e), (f), (g) }

uint8_t eeprom_read(uint8_t index);

void eeprom_write(uint8_t index, uint8_t val);

extern uint8_t eeprom_data[_EEPROMSIZE];

#endif // SWITCH_H__
