#ifndef BLE_ADVERTISING_H
#define BLE_ADVERTISING_H

/**
 * @brief Start BLE advertising
 *
 * @return 0 on success, negative error code on failure
 */
int ble_advertising_start(void);

/**
 * @brief Stop BLE advertising
 *
 * @return 0 on success, negative error code on failure
 */
int ble_advertising_stop(void);

#endif /* BLE_ADVERTISING_H */
