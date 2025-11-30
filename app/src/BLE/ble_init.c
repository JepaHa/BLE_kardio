#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <errno.h>

#include "BLE/ble_init.h"
#include "BLE/ble_advertising.h"
#include "BLE/ble_log.h"
#include "BLE/ble_manager.h"
#include "BLE/GATT/hrs.h"
#include "BLE/GATT/spo2.h"
#include "BLE/GATT/gatt_services.h"

LOG_MODULE_REGISTER(ble_init, CONFIG_LOG_DEFAULT_LEVEL);

static struct k_work_delayable adv_restart_work;
static bool bt_stack_enabled = false;
static bool has_connections = false;

/* LED GPIO for indicating Bluetooth state */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static void ble_led_set(bool state)
{
    if (gpio_is_ready_dt(&led)) {
        gpio_pin_set_dt(&led, state ? 1 : 0);
    }
}

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
        has_connections = true;

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

    /* Mark as disconnected - we'll update has_connections based on actual state
     */
    /* Note: In a multi-connection scenario, we'd need to check all connections,
     * but for simplicity, we assume single connection and set to false */
    has_connections = false;

    /* Cancel any pending advertising restart */
    k_work_cancel_delayable(&adv_restart_work);

    /* Notify manager about disconnection - it will disable Bluetooth */
    ble_manager_on_disconnected();
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
    int ret;
    int err;
    /* Initialize deferred logging */
    ble_log_init();

    /* Initialize advertising restart work */
    k_work_init_delayable(&adv_restart_work, adv_restart_work_handler);

    /* Initialize LED GPIO */
    if (gpio_is_ready_dt(&led)) {
        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure LED GPIO: %d", ret);
        } else {
            LOG_INF("LED GPIO initialized");
            /* LED starts off (Bluetooth not enabled yet) */
            ble_led_set(false);
        }
    } else {
        LOG_WRN("LED GPIO not ready");
    }

    /* Register all GATT services */
    err = gatt_services_register_all();
    if (err) {
        ble_log_error("Failed to register GATT services (err %d)", err);
        /* Turn off LED on error */
        ble_led_set(false);
        return err;
    }

    LOG_INF("BLE initialization structures ready");
    LOG_INF(
        "Note: Bluetooth stack will be enabled by SpO2 simulator when needed");

    /* Don't enable Bluetooth automatically - let SpO2 simulator manage it */
    /* GATT services will be registered when Bluetooth is enabled */
    return 0;
}

int ble_enable_stack(void)
{
    int err;

    if (bt_stack_enabled) {
        ble_log_info("Bluetooth stack already enabled");
        /* Ensure LED is on if already enabled */
        ble_led_set(true);
        return 0;
    }

    ble_log_info("Enabling Bluetooth stack...");

    /* Enable Bluetooth stack */
    err = bt_enable(NULL);
    if (err) {
        ble_log_error("Bluetooth enable failed (err %d)", err);
        /* Keep LED off on error */
        ble_led_set(false);
        return err;
    }

    ble_log_info("Bluetooth stack enabled");
    /* Turn on LED immediately after successful bt_enable */
    ble_led_set(true);

    /* Start advertising */
    err = ble_advertising_start();
    if (err) {
        ble_log_error("Failed to start advertising (err %d)", err);
        /* Turn off LED on error */
        ble_led_set(false);
        return err;
    }

    bt_stack_enabled = true;
    ble_log_info("Bluetooth stack fully enabled and advertising");
    /* LED is already on from bt_enable success */

    return 0;
}

int ble_disable_stack(void)
{
    int err;

    if (!bt_stack_enabled) {
        ble_log_info("Bluetooth stack already disabled");
        /* Ensure LED is off if already disabled */
        ble_led_set(false);
        return 0;
    }

    /* Check if there are active connections */
    if (has_connections) {
        ble_log_info("Cannot disable Bluetooth: active connections exist");
        /* Keep LED on if connections exist */
        ble_led_set(true);
        return -EBUSY;
    }

    ble_log_info("Disabling Bluetooth stack...");
    /* Turn off LED immediately when starting disable process */
    ble_led_set(false);

    /* Stop advertising */
    ble_advertising_stop();

    /* Disable Bluetooth stack */
    err = bt_disable();
    if (err) {
        ble_log_error("Bluetooth disable failed (err %d)", err);
        /* LED is already off */
        return err;
    }

    bt_stack_enabled = false;
    ble_log_info("Bluetooth stack disabled");
    /* LED is already off from start of disable process */

    return 0;
}

bool ble_is_enabled(void)
{
    return bt_stack_enabled;
}

bool ble_has_active_connections(void)
{
    return has_connections;
}
