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
 * For SpO2 (0-100%) and PR (0-300 bpm), we use exponent = 0
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

static uint8_t simulate_spo2;
static uint8_t spo2_value = 98U;  /* SpO2 value in percent (95-100 typical) */
static uint16_t pulse_rate = 90U; /* Pulse rate in bpm */
static uint8_t valid_spo2 = 1U;   /* Valid SpO2 flag (1 = valid, 0 = invalid) */
static uint16_t measurement_status = 0U; /* Measurement Status (16-bit) */
static struct k_timer spo2_timer;
static struct bt_conn *current_conn;

static void spo2_timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);
    spo2_notify();
}

static void spo2_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                 uint16_t value)
{
    ARG_UNUSED(attr);
    bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
    simulate_spo2 = notif_enabled ? 1 : 0;
    ble_log_info("SpO2 notifications %s",
                 notif_enabled ? "enabled" : "disabled");

    if (notif_enabled) {
        /* Start timer for periodic SpO2 measurements */
        /* SpO2 is typically measured every 1 second */
        k_timer_start(&spo2_timer, K_SECONDS(1), K_SECONDS(1));
        ble_log_info("SpO2 measurement timer started");
    } else {
        /* Stop timer when notifications are disabled */
        k_timer_stop(&spo2_timer);
        ble_log_info("SpO2 measurement timer stopped");
    }
}

/* Oxygen Saturation Service Declaration */
BT_GATT_SERVICE_DEFINE(spo2_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_OSS),
                       BT_GATT_CHARACTERISTIC(BT_UUID_SPO2_MEASUREMENT,
                                              BT_GATT_CHRC_NOTIFY,
                                              BT_GATT_PERM_NONE, NULL, NULL,
                                              NULL),
                       BT_GATT_CCC(spo2_ccc_cfg_changed,
                                   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), );

void spo2_init(void)
{
    k_timer_init(&spo2_timer, spo2_timer_handler, NULL);
    ble_log_info("SpO2 service initialized");
}

void spo2_notify(void)
{
    /* PLX Continuous Measurement format according to spec:
     * Byte 0: Flags (1 octet)
     * Byte 1-2: SpO2PR-Normal SpO2 (SFLOAT, 2 octets, LSO..MSO)
     * Byte 3-4: SpO2PR-Normal PR (SFLOAT, 2 octets, LSO..MSO)
     * Byte 5-6: Measurement Status (16-bit, 2 octets, LSO..MSO) - optional but
     * included
     */
    static uint8_t spo2_data[7];
    struct bt_conn *conn = current_conn;
    uint16_t spo2_sfloat;
    uint16_t pr_sfloat;

    /* SpO2 measurements simulation */
    if (!simulate_spo2) {
        return;
    }

    /* Simulate realistic SpO2 variation (95-100%) */
    spo2_value++;
    if (spo2_value > 100U) {
        spo2_value = 95U;
    }

    /* Simulate pulse rate (90-100 bpm) */
    pulse_rate++;
    if (pulse_rate > 100U) {
        pulse_rate = 90U;
    }

    /* Convert to SFLOAT format */
    spo2_sfloat = float_to_sfloat((float)spo2_value);
    pr_sfloat = float_to_sfloat((float)pulse_rate);

    /* Format PLX Continuous Measurement:
     * Flags: bit 0 = SpO2PR-Normal present, bit 1 = Measurement Status present
     */
    spo2_data[0] =
        0x03; /* Flags: SpO2PR-Normal and Measurement Status present */

    /* SpO2 (SFLOAT, LSO..MSO) */
    sys_put_le16(spo2_sfloat, &spo2_data[1]);

    /* PR - Pulse Rate (SFLOAT, LSO..MSO) */
    sys_put_le16(pr_sfloat, &spo2_data[3]);

    /* Measurement Status (16-bit, LSO..MSO)
     * Bit 0: Valid SpO2 (1 = valid, 0 = invalid)
     * Other bits: various status flags
     */
    measurement_status = valid_spo2 ? 0x0001 : 0x0000;
    sys_put_le16(measurement_status, &spo2_data[5]);

    /* Send notification to all connected devices */
    if (conn != NULL) {
        bt_gatt_notify(conn, &spo2_svc.attrs[1], &spo2_data, sizeof(spo2_data));
        ble_log_info("SpO2: %d%% PR: %d bpm (valid=%d) sent", spo2_value,
                     pulse_rate, valid_spo2);
    } else {
        /* If no specific connection, notify all */
        bt_gatt_notify(NULL, &spo2_svc.attrs[1], &spo2_data, sizeof(spo2_data));
        ble_log_info("SpO2: %d%% PR: %d bpm (valid=%d) sent (broadcast)",
                     spo2_value, pulse_rate, valid_spo2);
    }
}

void spo2_set_connection(struct bt_conn *conn)
{
    current_conn = conn;
}
