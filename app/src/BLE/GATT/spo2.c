/** @file
 *  @brief SpO2 Service sample
 */

/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "BLE/ble_log.h"
#include "BLE/GATT/spo2.h"

LOG_MODULE_REGISTER(spo2, CONFIG_LOG_DEFAULT_LEVEL);

/* Oxygen Saturation Service UUID: 0x1822 */
#define BT_UUID_OSS_VAL 0x1822
#define BT_UUID_OSS     BT_UUID_DECLARE_16(BT_UUID_OSS_VAL)

/* SpO2 Measurement Characteristic UUID: 0x2A5F */
#define BT_UUID_SPO2_MEASUREMENT_VAL 0x2A5F
#define BT_UUID_SPO2_MEASUREMENT                                               \
    BT_UUID_DECLARE_16(BT_UUID_SPO2_MEASUREMENT_VAL)

/* Helper function to convert value to IEEE 11073 SFLOAT format
 * SFLOAT: 16-bit, mantissa (12 bits, signed) + exponent (4 bits, signed)
 * Format: value = mantissa * 10^exponent
 * For SpO2 (0-100%), we use exponent = 0
 */
static uint16_t float_to_sfloat(float value)
{
    int16_t mantissa;
    int8_t exponent = 0;

    /* Handle special cases */
    if (value == 0.0f) {
        return 0x07FF; /* NRes (Not at this Resolution) */
    }

    /* For values 0-100 (SpO2) and 0-300 (PR), use exponent 0 */
    /* Values fit in mantissa range (-2048 to 2047) with exponent 0 */
    mantissa = (int16_t)value;

    /* Clamp mantissa to valid range */
    if (mantissa < -2048) {
        mantissa = -2048;
    } else if (mantissa > 2047) {
        mantissa = 2047;
    }

    /* SFLOAT format: mantissa (12 bits, signed) + exponent (4 bits, signed) */
    /* Mantissa in bits 0-11, exponent in bits 12-15 */
    return (uint16_t)((mantissa & 0x0FFF) | ((exponent & 0x0F) << 12));
}

static bool notifications_enabled = false;
static struct bt_conn *current_conn;

static void spo2_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                 uint16_t value)
{
    ARG_UNUSED(attr);
    notifications_enabled = (value == BT_GATT_CCC_NOTIFY);
    ble_log_info("SpO2 notifications %s",
                 notifications_enabled ? "enabled" : "disabled");
}

/* Oxygen Saturation Service Declaration */
static struct bt_gatt_attr spo2_attrs[] = {
    BT_GATT_PRIMARY_SERVICE(BT_UUID_OSS),
    BT_GATT_CHARACTERISTIC(BT_UUID_SPO2_MEASUREMENT, BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(spo2_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static struct bt_gatt_service spo2_svc = {
    .attrs = NULL,
    .attr_count = 0,
};

void spo2_send(uint8_t spo2_value)
{
    /* PLX Continuous Measurement format according to spec:
     * Byte 0: Flags (1 octet)
     * Byte 1-2: SpO2PR-Normal SpO2 (SFLOAT, 2 octets, LSO..MSO)
     * Byte 3-4: Measurement Status (16-bit, 2 octets, LSO..MSO) - optional but
     * included
     */
    static uint8_t spo2_data[5];
    struct bt_conn *conn = current_conn;
    uint16_t spo2_sfloat;
    uint16_t measurement_status = 0x0001; /* Valid measurement */

    /* Check if notifications are enabled */
    if (!notifications_enabled) {
        return;
    }

    /* Convert to SFLOAT format */
    spo2_sfloat = float_to_sfloat((float)spo2_value);

    /* Format PLX Continuous Measurement:
     * Flags: bit 0 = SpO2PR-Normal present, bit 1 = Measurement Status present
     */
    spo2_data[0] =
        0x03; /* Flags: SpO2PR-Normal and Measurement Status present */

    /* SpO2 (SFLOAT, LSO..MSO) */
    sys_put_le16(spo2_sfloat, &spo2_data[1]);

    /* Measurement Status (16-bit, LSO..MSO)
     * Bit 0: Valid SpO2 (1 = valid, 0 = invalid)
     * Other bits: various status flags
     */
    sys_put_le16(measurement_status, &spo2_data[3]);

    /* Send notification to all connected devices */
    if (conn != NULL) {
        bt_gatt_notify(conn, &spo2_svc.attrs[1], &spo2_data, sizeof(spo2_data));
        ble_log_info("SpO2: %d%% sent", spo2_value);
    } else {
        /* If no specific connection, notify all */
        bt_gatt_notify(NULL, &spo2_svc.attrs[1], &spo2_data, sizeof(spo2_data));
        ble_log_info("SpO2: %d%% sent (broadcast)", spo2_value);
    }
}

void spo2_set_connection(struct bt_conn *conn)
{
    current_conn = conn;
}

int spo2_service_register(void)
{
    int err;

    /* Initialize service structure */
    spo2_svc.attrs = spo2_attrs;
    spo2_svc.attr_count = ARRAY_SIZE(spo2_attrs);

    /* Register SpO2 service */
    err = bt_gatt_service_register(&spo2_svc);
    if (err) {
        ble_log_error("SpO2 service registration failed (err %d)", err);
        return err;
    }

    ble_log_info("SpO2 service registered");
    return 0;
}

int spo2_service_unregister(void)
{
    int err;

    err = bt_gatt_service_unregister(&spo2_svc);
    if (err) {
        ble_log_error("SpO2 service unregistration failed (err %d)", err);
        return err;
    }

    ble_log_info("SpO2 service unregistered");
    return 0;
}

struct bt_gatt_service *spo2_get_service(void)
{
    return &spo2_svc;
}
