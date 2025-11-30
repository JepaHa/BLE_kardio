#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Wait for first connection with extended timeout
 *
 * This function enables Bluetooth and waits for the first connection
 * with an extended timeout (60 seconds). If no connection is established,
 * Bluetooth is disabled and the device enters power saving mode.
 *
 * @return 0 on success (connected or timeout), negative error code on failure
 */
int ble_manager_wait_for_first_connection(void);

/**
 * @brief Send sensor data (heartrate and SpO2) via Bluetooth
 *
 * This is the main unified function for sending sensor data. It sends:
 * - Heartrate to HRS (Heart Rate Service) - only heartrate
 * - Heartrate and SpO2 to SpO2 service - both values
 *
 * @param heartrate Heart rate in bpm (beats per minute)
 * @param spo2 SpO2 value (oxygen saturation)
 *
 * @return 0 on success, negative error code on failure
 */
int ble_manager_send_sensor_data(uint16_t heartrate, uint16_t spo2);

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

#endif /* BLE_MANAGER_H */
