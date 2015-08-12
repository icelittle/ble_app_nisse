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

#include <stdint.h>
#include "ble_hci.h"
#include "app_error.h"
#include "app_trace.h"
#include "app_util.h"
#include "pstorage.h"

#include "switch.h"

#define SIGNATURE   DEVICE_MANAGER_MAX_BONDS
#define EEPROM_SIZE _EEPROMSIZE

#define nSWITCH_DISABLE_LOGS    // Enable this macro to disable any logs from this module.

#ifndef SWITCH_DISABLE_LOGS
#define SW_LOG  app_trace_log
#else
#define SW_LOG(...)
#endif

typedef struct {
    uint32_t signature;
    dm_device_instance_t devices[DEVICE_MANAGER_MAX_BONDS];
    uint8_t current_index;
    uint8_t eeprom[DEVICE_MANAGER_MAX_BONDS][EEPROM_SIZE];
} switch_context_t;

STATIC_ASSERT(sizeof(switch_context_t) % 4 == 0);

static dm_device_instance_t m_current_device;
static pstorage_handle_t    m_storage_handle;
static switch_context_t     m_switch_context;

static void switch_pstorage_callback(pstorage_handle_t* handle,
                                     uint8_t            op_code,
                                     uint32_t           reason,
                                     uint8_t*           p_data,
                                     uint32_t           param_len)
{
    if (reason != NRF_SUCCESS)
        SW_LOG("[SW]: switch_pstorage_callback: %lx\n", reason);
}

static uint32_t switch_save(void)
{
    uint32_t          err_code;
    pstorage_handle_t block_handle;

    err_code = pstorage_block_identifier_get(&m_storage_handle, 0, &block_handle);
    if (err_code == NRF_SUCCESS) {
        err_code = pstorage_update(&block_handle,
                                   (uint8_t *) &m_switch_context,
                                   sizeof(switch_context_t),
                                   0);
    }
    return err_code;
}

uint32_t switch_init(bool erase_bonds)
{
    uint32_t                err_code;
    pstorage_module_param_t param;
    pstorage_handle_t       block_handle;

    // Register with storage module.
    param.block_count = 1;
    param.block_size  = sizeof(switch_context_t);
    param.cb          = switch_pstorage_callback;
    err_code = pstorage_register(&param, &m_storage_handle);
    APP_ERROR_CHECK(err_code);

    err_code = pstorage_block_identifier_get(&m_storage_handle, 0, &block_handle);
    if (err_code == NRF_SUCCESS) {
        err_code = pstorage_load((uint8_t *) &m_switch_context, &block_handle, sizeof(switch_context_t), 0);
        if (err_code == NRF_SUCCESS && m_switch_context.signature == SIGNATURE && !erase_bonds) {
            if (DEVICE_MANAGER_MAX_BONDS <= m_switch_context.current_index)
                m_switch_context.current_index = 0;
            for (unsigned i = 0; i < DEVICE_MANAGER_MAX_BONDS; ++i) {
                if (DEVICE_MANAGER_MAX_BONDS <= m_switch_context.devices[i])
                    m_switch_context.devices[i] = DM_INVALID_ID;
            }
        } else {
            m_switch_context.signature = SIGNATURE;
            memset(m_switch_context.devices, DM_INVALID_ID, DEVICE_MANAGER_MAX_BONDS);
            memmove(m_switch_context.eeprom, eeprom_data, EEPROM_SIZE);
            m_switch_context.current_index = 0;
            err_code = switch_save();
        }
    }

    m_current_device = m_switch_context.devices[m_switch_context.current_index];

    SW_LOG("[SW]: switch_init: %u [sizeof(switch_context_t) = %u]\n", m_switch_context.current_index, sizeof(switch_context_t));

    return err_code;
}

uint32_t switch_select(uint8_t index)
{
    if (DEVICE_MANAGER_MAX_BONDS <= index)
        index = 0;
    m_switch_context.current_index = index;
    m_current_device = m_switch_context.devices[index];

    SW_LOG("[SW]: switch_select: %u\n", m_switch_context.current_index);

    return switch_save();
}

uint32_t switch_update(dm_handle_t const* p_handle)
{
    if (p_handle->device_id < DEVICE_MANAGER_MAX_BONDS) {
        m_switch_context.devices[m_switch_context.current_index] = p_handle->device_id;
        m_current_device = p_handle->device_id;
    }
    SW_LOG("[SW]: switch_update: %u\n", m_switch_context.current_index);
    return switch_save();
}

uint32_t switch_reset(dm_handle_t const* p_handle)
{
    dm_device_delete(p_handle);
    m_switch_context.devices[m_switch_context.current_index] = DM_INVALID_ID;
    m_current_device = DM_INVALID_ID;
    return switch_save();
}

uint8_t switch_get_current(void)
{
    return m_switch_context.current_index;
}

uint8_t switch_get_current_device(void)
{
    return m_current_device;
}

uint32_t switch_filter(dm_handle_t const* p_handle,
                       dm_event_t const*  p_event,
                       uint16_t           conn_handle)
{
    uint32_t err_code = 0;
    bool disconnect = false;

    switch (p_event->event_id) {
    case DM_EVT_CONNECTION:
        if (p_handle->device_id != DM_INVALID_ID) {
            if (m_current_device != DM_INVALID_ID) {
                if (p_handle->device_id != m_current_device)
                    disconnect = true;
            } else {
                for (unsigned i = 0; i < DEVICE_MANAGER_MAX_BONDS; ++i) {
                    if (m_switch_context.devices[i] == p_handle->device_id) {
                        disconnect = true;
                        break;
                    }
                }
            }
        }
        if (disconnect) {
            err_code = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            SW_LOG("[SW]: switch_filter: %02x %02x\n", m_current_device, p_handle->device_id);
            err_code = 1;
        }
        break;
    default:
        break;
    }

    return err_code;
}

uint8_t eeprom_read(uint8_t index)
{
    if (EEPROM_SIZE <= index)
        return 0;
    return m_switch_context.eeprom[m_switch_context.current_index][index];
}

void eeprom_write(uint8_t index, uint8_t val)
{
    if (EEPROM_SIZE <= index)
        return;
    m_switch_context.eeprom[m_switch_context.current_index][index] = val;
    switch_save();
}
