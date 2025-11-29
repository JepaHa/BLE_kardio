#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "BLE/ble_init.h"
#include "BLE/ble_advertising.h"

LOG_MODULE_REGISTER(ble_init, CONFIG_LOG_DEFAULT_LEVEL);

int ble_init(void)
{
    int err;

    LOG_INF("Initializing BLE...");

    err = bt_enable(NULL);
    if (err) {
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }

    LOG_INF("Bluetooth initialized");

    ble_advertising_start();
    return 0;
}
