/** @file
 *  @brief High-level Bluetooth Manager implementation
 *
 * This module provides a high-level API for managing Bluetooth operations:
 * - Enabling/disabling Bluetooth stack
 * - Managing advertising
 * - Sending data via GATT notifications
 * - Connection management
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "BLE/ble_manager.h"
#include "BLE/ble_init.h"
#include "BLE/ble_advertising.h"
#include "BLE/GATT/spo2.h"
#include "BLE/GATT/hrs.h"

LOG_MODULE_REGISTER(ble_manager, CONFIG_LOG_DEFAULT_LEVEL);

#define DEFAULT_CONNECTION_TIMEOUT_MS   1000 /* 10 seconds */
#define DEFAULT_NOTIFICATION_TIMEOUT_MS 5000 /* 5 seconds */

int ble_manager_init(void)
{
    LOG_INF("BLE Manager initialized");
    return 0;
}

int ble_manager_enable_and_wait(uint32_t connection_timeout_ms,
                                uint32_t notification_timeout_ms)
{
    int err;

    /* Check if already enabled */
    if (ble_is_enabled()) {
        LOG_INF("Bluetooth already enabled");
        /* Check if already connected */
        if (ble_has_active_connections()) {
            LOG_INF("Already connected");
            return 0;
        }
    } else {
        /* Enable Bluetooth stack */
        LOG_INF("Enabling Bluetooth stack...");
        err = ble_enable_stack();
        if (err) {
            LOG_ERR("Failed to enable Bluetooth stack (err %d)", err);
            return err;
        }
    }

    /* Wait for connection */
    LOG_INF("Waiting for connection (timeout: %u ms)...",
            connection_timeout_ms);
    uint32_t wait_time = 0;
    bool connected = false;

    while (wait_time < connection_timeout_ms) {
        if (ble_has_active_connections()) {
            connected = true;
            LOG_INF("Connection established");
            break;
        }
        k_msleep(1000);
        wait_time += 1000;
    }

    if (!connected) {
        LOG_WRN("Connection timeout after %u ms - Bluetooth is enabled and "
                "advertising",
                connection_timeout_ms);
        return -ETIMEDOUT;
    }

    /* Wait for notification subscription */
    LOG_INF("Waiting for notification subscription (timeout: %u ms)...",
            notification_timeout_ms);
    k_msleep(notification_timeout_ms);

    LOG_INF("Bluetooth ready for data transmission");
    return 0;
}

int ble_manager_disable_if_idle(void)
{
    if (!ble_is_enabled()) {
        LOG_INF("Bluetooth already disabled");
        return 0;
    }

    if (ble_has_active_connections()) {
        LOG_INF("Active connections exist, keeping Bluetooth enabled");
        return 0;
    }

    LOG_INF("No active connections, disabling Bluetooth...");
    k_msleep(1000); /* Wait a bit before disabling */

    int err = ble_disable_stack();
    if (err) {
        LOG_ERR("Failed to disable Bluetooth stack (err %d)", err);
        return err;
    }

    LOG_INF("Bluetooth disabled");
    return 0;
}

int ble_manager_on_disconnected(void)
{
    LOG_INF("Device disconnected, disabling Bluetooth");
    return ble_manager_disable_if_idle();
}

int ble_manager_send_data(enum ble_data_type data_type, uint8_t value)
{
    int err;

    /* Enable Bluetooth if not already enabled */
    if (!ble_is_enabled()) {
        err = ble_enable_stack();
        if (err) {
            LOG_ERR("Failed to enable Bluetooth stack (err %d)", err);
            return err;
        }
    }

    /* Wait for connection if not already connected */
    if (!ble_has_active_connections()) {
        err = ble_manager_enable_and_wait(DEFAULT_CONNECTION_TIMEOUT_MS,
                                          DEFAULT_NOTIFICATION_TIMEOUT_MS);
        if (err) {
            /* Connection timeout - disable Bluetooth and wait for next call */
            LOG_WRN("No connection available, disabling Bluetooth");
            ble_manager_disable_if_idle();
            return 0; /* Return success, but data wasn't sent */
        }
    }

    /* Check again if we have connection before sending */
    if (!ble_has_active_connections()) {
        LOG_WRN("No active connection, disabling Bluetooth");
        ble_manager_disable_if_idle();
        return 0; /* Return success, but data wasn't sent */
    }

    /* Send data based on type */
    switch (data_type) {
    case BLE_DATA_TYPE_SPO2:
        LOG_INF("Sending SpO2: %d%%", value);
        spo2_send(value);
        break;

    case BLE_DATA_TYPE_HRS:
        LOG_INF("Sending HRS data");
        hrs_notify();
        break;

    default:
        LOG_ERR("Unknown data type: %d", data_type);
        return -EINVAL;
    }

    /* Optionally disable Bluetooth if no active connections */
    /* Note: This is done after sending to allow for potential retries */
    ble_manager_disable_if_idle();

    return 0;
}

int ble_manager_send_spo2(uint8_t spo2_value)
{
    return ble_manager_send_data(BLE_DATA_TYPE_SPO2, spo2_value);
}

bool ble_manager_is_enabled(void)
{
    return ble_is_enabled();
}

bool ble_manager_has_connections(void)
{
    return ble_has_active_connections();
}
