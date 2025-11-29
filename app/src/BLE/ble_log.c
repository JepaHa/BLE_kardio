/** @file
 *  @brief Bluetooth deferred logging module implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <stdarg.h>
#include <stdint.h>

#include "BLE/ble_log.h"

LOG_MODULE_REGISTER(ble_log, CONFIG_LOG_DEFAULT_LEVEL);

#define LOG_BUFFER_SIZE 128

static struct k_work_delayable log_work;
static char log_buffer[LOG_BUFFER_SIZE];
static enum {
    LOG_EVENT_CONNECTED,
    LOG_EVENT_DISCONNECTED,
    LOG_EVENT_SECURITY_CHANGED,
    LOG_EVENT_INFO,
    LOG_EVENT_ERROR
} log_event_type;

static void log_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    switch (log_event_type) {
    case LOG_EVENT_CONNECTED:
        LOG_INF("BLE Connected: %s", log_buffer);
        break;
    case LOG_EVENT_DISCONNECTED:
        LOG_INF("BLE Disconnected: %s", log_buffer);
        break;
    case LOG_EVENT_SECURITY_CHANGED:
        LOG_INF("BLE Security changed: %s", log_buffer);
        break;
    case LOG_EVENT_INFO:
        LOG_INF("%s", log_buffer);
        break;
    case LOG_EVENT_ERROR:
        LOG_ERR("%s", log_buffer);
        break;
    }
}

void ble_log_init(void)
{
    k_work_init_delayable(&log_work, log_work_handler);
}

void ble_log_connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    if (conn != NULL) {
        bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    } else {
        snprintk(addr, sizeof(addr), "unknown");
    }

    if (err) {
        snprintk(log_buffer, sizeof(log_buffer), "%s (err 0x%02x)", addr, err);
        log_event_type = LOG_EVENT_DISCONNECTED;
    } else {
        snprintk(log_buffer, sizeof(log_buffer), "%s", addr);
        log_event_type = LOG_EVENT_CONNECTED;
    }

    k_work_schedule(&log_work, K_NO_WAIT);
}

void ble_log_disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    if (conn != NULL) {
        bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    } else {
        snprintk(addr, sizeof(addr), "unknown");
    }

    snprintk(log_buffer, sizeof(log_buffer), "%s (reason 0x%02x)", addr,
             reason);
    log_event_type = LOG_EVENT_DISCONNECTED;
    k_work_schedule(&log_work, K_NO_WAIT);
}

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CLASSIC)
void ble_log_security_changed(struct bt_conn *conn, bt_security_t level,
                              enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    if (conn != NULL) {
        bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    } else {
        snprintk(addr, sizeof(addr), "unknown");
    }

    if (!err) {
        snprintk(log_buffer, sizeof(log_buffer), "%s level %u", addr, level);
        log_event_type = LOG_EVENT_SECURITY_CHANGED;
        k_work_schedule(&log_work, K_NO_WAIT);
    } else {
        snprintk(log_buffer, sizeof(log_buffer), "%s level %u err %d", addr,
                 level, err);
        log_event_type = LOG_EVENT_ERROR;
        k_work_schedule(&log_work, K_NO_WAIT);
    }
}
#endif

void ble_log_info(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintk(log_buffer, sizeof(log_buffer), format, args);
    va_end(args);

    log_event_type = LOG_EVENT_INFO;
    k_work_schedule(&log_work, K_NO_WAIT);
}

void ble_log_error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintk(log_buffer, sizeof(log_buffer), format, args);
    va_end(args);

    log_event_type = LOG_EVENT_ERROR;
    k_work_schedule(&log_work, K_NO_WAIT);
}
