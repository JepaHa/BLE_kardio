/** @file
 *  @brief Bluetooth deferred logging module
 */

#ifndef BLE_LOG_H
#define BLE_LOG_H

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Bluetooth logging module
 */
void ble_log_init(void);

/**
 * @brief Log connection event (deferred)
 *
 * @param conn Connection object
 * @param err Error code (0 for success)
 */
void ble_log_connected(struct bt_conn *conn, uint8_t err);

/**
 * @brief Log disconnection event (deferred)
 *
 * @param conn Connection object
 * @param reason Disconnection reason
 */
void ble_log_disconnected(struct bt_conn *conn, uint8_t reason);

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
/**
 * @brief Log security change event (deferred)
 *
 * @param conn Connection object
 * @param level Security level
 * @param err Security error (0 for success)
 */
void ble_log_security_changed(struct bt_conn *conn, bt_security_t level,
                              enum bt_security_err err);
#endif

/**
 * @brief Log generic Bluetooth event (deferred)
 *
 * @param format Format string (like printf)
 * @param ... Arguments for format string
 */
void ble_log_info(const char *format, ...);

/**
 * @brief Log generic Bluetooth error (deferred)
 *
 * @param format Format string (like printf)
 * @param ... Arguments for format string
 */
void ble_log_error(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* BLE_LOG_H */
