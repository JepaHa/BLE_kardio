#ifndef BLE_INIT_H
#define BLE_INIT_H

#include <stdbool.h>

/**
 * @brief Initialize BLE stack
 *
 * @return 0 on success, negative error code on failure
 */
int ble_init(void);

/**
 * @brief Enable Bluetooth stack
 *
 * @return 0 on success, negative error code on failure
 */
int ble_enable_stack(void);

/**
 * @brief Disable Bluetooth stack
 *
 * @return 0 on success, negative error code on failure
 */
int ble_disable_stack(void);

/**
 * @brief Check if Bluetooth stack is enabled
 *
 * @return true if enabled, false otherwise
 */
bool ble_is_enabled(void);

/**
 * @brief Check if there are active Bluetooth connections
 *
 * @return true if there are active connections, false otherwise
 */
bool ble_has_active_connections(void);

#endif /* BLE_INIT_H */
