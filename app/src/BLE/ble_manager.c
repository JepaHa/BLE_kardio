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
#include <zephyr/zbus/zbus.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "BLE/ble_manager.h"
#include "BLE/ble_init.h"
#include "BLE/GATT/spo2.h"
#include "BLE/GATT/hrs.h"
#include "zbus/zbus_channels.h"

LOG_MODULE_REGISTER(ble_manager, CONFIG_LOG_DEFAULT_LEVEL);

#define DEFAULT_CONNECTION_TIMEOUT_MS   10000 /* 10 seconds */
#define DEFAULT_NOTIFICATION_TIMEOUT_MS 500   /* 5 seconds */
#define FIRST_CONNECTION_TIMEOUT_MS                                            \
    60000 /* 60 seconds for first connection                                   \
           */

static bool first_connection_attempted = false;

/**
 * @brief Ensure Bluetooth connection is established
 *
 * This helper function handles the common logic of enabling Bluetooth
 * and waiting for connection. It can be reused by multiple send functions.
 *
 * @return 0 on success (connected), negative error code on failure (e.g.,
 * -ETIMEDOUT)
 */
static int ble_manager_ensure_connection(void)
{
    int err;
    uint32_t connection_timeout;

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
        /* Use longer timeout for first connection attempt */
        if (!first_connection_attempted) {
            connection_timeout = FIRST_CONNECTION_TIMEOUT_MS;
            LOG_INF("First connection attempt - waiting up to %u seconds",
                    connection_timeout / 1000);
            first_connection_attempted = true;
        } else {
            connection_timeout = DEFAULT_CONNECTION_TIMEOUT_MS;
        }

        err = ble_manager_enable_and_wait(connection_timeout,
                                          DEFAULT_NOTIFICATION_TIMEOUT_MS);
        if (err) {
            /* Connection timeout - disable Bluetooth and enter power saving
             * mode */
            LOG_WRN("No connection available after %u ms, entering power "
                    "saving mode",
                    connection_timeout);
            ble_manager_disable_if_idle();
            return -ETIMEDOUT; /* Return error - connection failed */
        }
    }

    /* ble_manager_enable_and_wait() already verified connection, no need to
     * check again */
    return 0;
}

/**
 * @brief Zbus listener handler for sensor data channel
 *
 * This handler is called automatically when data is published to the
 * sensor_data_chan channel. It reads the data and sends it via BLE.
 *
 * @param chan Pointer to the zbus channel
 */
void ble_manager_sensor_data_handler(const struct zbus_channel *chan)
{
    const struct sensor_data *data;
    int err;

    /* Get pointer to message data from channel */
    data = zbus_chan_const_msg(chan);

    if (data == NULL) {
        LOG_ERR("Failed to get message from zbus channel");
        return;
    }

    LOG_INF("Received sensor data from zbus: pulse=%d, spo2=%d", data->pulse,
            data->spo2);

    /* Send data via BLE using unified function */
    err = ble_manager_send_sensor_data(data->pulse, data->spo2);
    if (err) {
        LOG_ERR("Failed to send sensor data via BLE (err %d)", err);
    }
}

int ble_manager_wait_for_first_connection(void)
{
    int err;

    LOG_INF("Waiting for first connection (timeout: %u seconds)...",
            FIRST_CONNECTION_TIMEOUT_MS / 1000);

    /* Enable Bluetooth stack */
    err = ble_enable_stack();
    if (err) {
        LOG_ERR("Failed to enable Bluetooth stack (err %d)", err);
        return err;
    }

    /* Wait for connection with extended timeout */
    err = ble_manager_enable_and_wait(FIRST_CONNECTION_TIMEOUT_MS,
                                      DEFAULT_NOTIFICATION_TIMEOUT_MS);
    if (err) {
        /* Connection timeout - disable Bluetooth and enter power saving mode */
        LOG_WRN("No connection established after %u seconds, entering power "
                "saving mode",
                FIRST_CONNECTION_TIMEOUT_MS / 1000);
        ble_manager_disable_if_idle();
        first_connection_attempted = true; /* Mark as attempted */
        return 0;                          /* Not an error, just timeout */
    }

    /* Connection established */
    LOG_INF("First connection established successfully");
    first_connection_attempted = true;
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

/**
 * @brief Disable Bluetooth if no active connections
 *
 * Disables Bluetooth stack if there are no active connections.
 * Uses short delay (200ms) for quick power saving after data transmission.
 *
 * @return 0 on success, negative error code on failure
 */
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
    /* Small delay to ensure any pending operations complete */
    k_msleep(200); /* Short delay before disabling */

    /* Check again after delay - connection might have been established */
    if (ble_has_active_connections()) {
        LOG_INF(
            "Connection established during delay, keeping Bluetooth enabled");
        return 0;
    }

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

/**
 * @brief Send sensor data (heartrate and SpO2) via Bluetooth
 *
 * This is the main function for sending sensor data. It sends:
 * - Heartrate to HRS (Heart Rate Service) - only heartrate
 * - Heartrate and SpO2 to SpO2 service - both values
 *
 * @param heartrate Heart rate in bpm (beats per minute)
 * @param spo2 SpO2 value (oxygen saturation)
 *
 * @return 0 on success, negative error code on failure
 */
int ble_manager_send_sensor_data(uint16_t heartrate, uint16_t spo2)
{
    int err;

    /* Ensure Bluetooth connection is established */
    err = ble_manager_ensure_connection();
    if (err) {
        LOG_ERR("Failed to establish connection (err %d)", err);
        return err;
    }

    /* ble_manager_ensure_connection() guarantees connection or returns error */
    /* Send heartrate to HRS service (only heartrate) */
    LOG_INF("Sending HRS: %d bpm", heartrate);
    hrs_send(heartrate);

    /* Send SpO2 data with pulse rate to SpO2 service (both heartrate and spo2)
     */
    /* Note: spo2_send expects uint8_t for spo2_value, so we need to clamp it */
    uint8_t spo2_value = (spo2 > 255) ? 255 : (uint8_t)spo2;
    LOG_INF("Sending SpO2: %d%%, PR: %d bpm", spo2_value, heartrate);
    spo2_send(spo2_value, heartrate);

    /* Wait a bit to ensure data is sent before disconnecting */
    k_msleep(100); /* Small delay to ensure notifications are sent */

    /* Disable Bluetooth after sending data (data comes in batches, no
     * consecutive packets) */
    ble_manager_disable_if_idle();

    return 0;
}
