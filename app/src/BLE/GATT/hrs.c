/** @file
 *  @brief HRS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "BLE/ble_log.h"

LOG_MODULE_REGISTER(hrs, CONFIG_LOG_DEFAULT_LEVEL);

static uint8_t simulate_hrm;
static uint8_t heartrate = 90U;
static uint8_t hrs_blsc;
static struct k_timer hrs_timer;
static struct bt_conn *current_conn;

static void hrs_timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    hrs_notify();
}

static void hrmc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                 uint16_t value)
{
    ARG_UNUSED(attr);
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    simulate_hrm = notif_enabled ? 1 : 0;
    ble_log_info("HRS notifications %s",
                 notif_enabled ? "enabled" : "disabled");

    if (notif_enabled) {
        /* Start timer for periodic heart rate measurements */
        /* Heart rate is typically measured every 1 second */
        k_timer_start(&hrs_timer, K_SECONDS(1), K_SECONDS(1));
        ble_log_info("HRS measurement timer started");
    } else {
        /* Stop timer when notifications are disabled */
        k_timer_stop(&hrs_timer);
        ble_log_info("HRS measurement timer stopped");
    }
}

static ssize_t read_blsc(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                         void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &hrs_blsc,
                             sizeof(hrs_blsc));
}

/* Heart Rate Service Declaration */
BT_GATT_SERVICE_DEFINE(
    hrs_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_HRS),
    BT_GATT_CHARACTERISTIC(BT_UUID_HRS_MEASUREMENT, BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(hrmc_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CHARACTERISTIC(BT_UUID_HRS_BODY_SENSOR, BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ, read_blsc, NULL, NULL),
    BT_GATT_CHARACTERISTIC(BT_UUID_HRS_CONTROL_POINT, BT_GATT_CHRC_WRITE,
                           BT_GATT_PERM_NONE, NULL, NULL, NULL), );

void hrs_init(uint8_t blsc)
{
    hrs_blsc = blsc;
    k_timer_init(&hrs_timer, hrs_timer_handler, NULL);
    ble_log_info("HRS initialized with body sensor location: 0x%02x", blsc);
}

void hrs_notify(void)
{
    static uint8_t hrm[2];
    struct bt_conn *conn = current_conn;

    /* Heartrate measurements simulation */
    if (!simulate_hrm) {
        return;
    }

    /* Simulate realistic heart rate variation (90-160 bpm) */
    heartrate++;
    if (heartrate > 160U) {
        heartrate = 90U;
    }

    /* Format heart rate measurement according to HRS spec:
     * Byte 0: Flags (bit 0: Heart Rate Value Format, bit 1: Sensor Contact
     * Status) 0x06 = Heart Rate Value Format is UINT8, Sensor Contact detected
     * Byte 1: Heart Rate Value (UINT8)
     */
    hrm[0] = 0x06; /* uint8, sensor contact detected */
    hrm[1] = heartrate;

    /* Send notification to all connected devices */
    if (conn != NULL) {
        bt_gatt_notify(conn, &hrs_svc.attrs[1], &hrm, sizeof(hrm));
        ble_log_info("HRS: Heartrate %d bpm sent", heartrate);
    } else {
        /* If no specific connection, notify all */
        bt_gatt_notify(NULL, &hrs_svc.attrs[1], &hrm, sizeof(hrm));
        ble_log_info("HRS: Heartrate %d bpm sent (broadcast)", heartrate);
    }
}

void hrs_set_connection(struct bt_conn *conn)
{
    current_conn = conn;
}