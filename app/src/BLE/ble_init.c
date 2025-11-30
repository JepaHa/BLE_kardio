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
#include "BLE/GATT/spo2.h"
#include "BLE/GATT/gatt_services.h"

LOG_MODULE_REGISTER(ble_init, CONFIG_LOG_DEFAULT_LEVEL);

static struct k_work_delayable adv_restart_work;

static void adv_restart_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    int err = ble_advertising_start();
    if (err) {
        ble_log_error("Failed to restart advertising (err %d), retrying...",
                      err);
        /* Retry after another delay if failed */
        k_work_schedule(&adv_restart_work, K_MSEC(500));
    } else {
        ble_log_info("Advertising restarted successfully");
    }
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    ble_log_connected(conn, err);

    if (!err) {
        /* Cancel any pending advertising restart */
        k_work_cancel_delayable(&adv_restart_work);

        /* Stop advertising when connected */
        ble_advertising_stop();
        ble_log_info("Advertising stopped (connected)");

        /* Set connection for HRS and SpO2 services */
        hrs_set_connection(conn);
        spo2_set_connection(conn);

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

    /* Clear connection for HRS and SpO2 services */
    hrs_set_connection(NULL);
    spo2_set_connection(NULL);

    /* Restart advertising when disconnected with delay to allow stack to free
     * resources */
    /* Delay of 100ms should be enough for stack to clean up */
    k_work_schedule(&adv_restart_work, K_MSEC(100));
    ble_log_info("Advertising restart scheduled (disconnected)");
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

    /* Initialize advertising restart work */
    k_work_init_delayable(&adv_restart_work, adv_restart_work_handler);

    LOG_INF("Initializing BLE...");

    /* Enable Bluetooth stack */
    err = bt_enable(NULL);
    if (err) {
        ble_log_error("Bluetooth init failed (err %d)", err);
        return err;
    }

    ble_log_info("Bluetooth initialized");

    /* Register all GATT services */
    err = gatt_services_register_all();
    if (err) {
        ble_log_error("Failed to register GATT services (err %d)", err);
        return err;
    }

    /* Start advertising */
    ble_advertising_start();
    return 0;
}
