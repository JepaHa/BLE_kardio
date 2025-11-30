#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Data types that can be sent via Bluetooth
 */
enum ble_data_type {
    BLE_DATA_TYPE_SPO2, /**< SpO2 (Oxygen Saturation) data */
    BLE_DATA_TYPE_HRS,  /**< HRS (Heart Rate Service) data */
};

/**
 * @brief Initialize BLE Manager
 *
 * This function initializes the high-level Bluetooth management service.
 * It should be called once at system startup.
 *
 * @return 0 on success, negative error code on failure
 */
int ble_manager_init(void);

/**
 * @brief Send data via Bluetooth
 *
 * This is a high-level universal function that:
 * - Enables Bluetooth if disabled
 * - Waits for connection and notification subscription
 * - Sends the data via GATT notifications based on data type
 * - Optionally disables Bluetooth after sending (if no active connections)
 *
 * @param data_type Type of data to send (SPO2, HRS, etc.)
 * @param value Data value (for SPO2: 0-100, for HRS: ignored)
 *
 * @return 0 on success, negative error code on failure
 */
int ble_manager_send_data(enum ble_data_type data_type, uint8_t value);

/**
 * @brief Send SpO2 data via Bluetooth (convenience wrapper)
 *
 * @param spo2_value SpO2 value in percent (0-100)
 *
 * @return 0 on success, negative error code on failure
 */
int ble_manager_send_spo2(uint8_t spo2_value);

/**
 * @brief Enable Bluetooth and wait for connection
 *
 * Enables Bluetooth stack, starts advertising, and waits for a connection
 * and notification subscription.
 *
 * @param connection_timeout_ms Maximum time to wait for connection
 * (milliseconds)
 * @param notification_timeout_ms Maximum time to wait for notification
 * subscription after connection (milliseconds)
 *
 * @return 0 on success, negative error code on failure
 */
int ble_manager_enable_and_wait(uint32_t connection_timeout_ms,
                                uint32_t notification_timeout_ms);

/**
 * @brief Disable Bluetooth if no active connections
 *
 * Disables Bluetooth stack if there are no active connections.
 *
 * @return 0 on success, negative error code on failure
 */
int ble_manager_disable_if_idle(void);

/**
 * @brief Handle Bluetooth disconnection event
 *
 * Called when a Bluetooth device disconnects. This function will
 * disable Bluetooth stack since there are no active connections.
 *
 * @return 0 on success, negative error code on failure
 */
int ble_manager_on_disconnected(void);

/**
 * @brief Check if Bluetooth is enabled
 *
 * @return true if Bluetooth is enabled, false otherwise
 */
bool ble_manager_is_enabled(void);

/**
 * @brief Check if there are active connections
 *
 * @return true if there are active connections, false otherwise
 */
bool ble_manager_has_connections(void);

#endif /* BLE_MANAGER_H */
