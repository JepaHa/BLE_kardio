#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>

#include "BLE/ble_init.h"
#include "BLE/ble_advertising.h"
#include "BLE/ble_log.h"
#include "BLE/GATT/hrs.h"

LOG_MODULE_REGISTER(ble_init, CONFIG_LOG_DEFAULT_LEVEL);

static void connected(struct bt_conn *conn, uint8_t err)
{
    ble_log_connected(conn, err);

    if (!err) {
        /* Set connection for HRS service */
        hrs_set_connection(conn);

#if defined(CONFIG_BT_SMP)
        if (bt_conn_get_security(conn) < BT_SECURITY_L2) {
            err = bt_conn_set_security(conn, BT_SECURITY_L2);
            if (err) {
                ble_log_error("Failed to set security (err %d)", err);
            }
        }
#endif
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ble_log_disconnected(conn, reason);

    /* Clear connection for HRS service */
    hrs_set_connection(NULL);
}

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err)
{
    ble_log_security_changed(conn, level, err);
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
    .security_changed = security_changed,
#endif
};

int ble_init(void)
{
    int err;

    /* Initialize deferred logging */
    ble_log_init();

    LOG_INF("Initializing BLE...");

    err = bt_enable(NULL);
    if (err) {
        ble_log_error("Bluetooth init failed (err %d)", err);
        return err;
    }

    ble_log_info("Bluetooth initialized");
    hrs_init(0x01);
    ble_log_info("HRS initialized");
    ble_advertising_start();
    return 0;
}
