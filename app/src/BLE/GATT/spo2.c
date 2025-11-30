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
 *
 * IEEE 11073-10404 SFLOAT format:
 * - Bits 0-11: Mantissa (signed, -2048 to 2047) in two's complement
 * - Bits 12-15: Exponent (signed, -8 to 7) in two's complement
 * - Special values:
 *   0x07FF = NRes (Not at this Resolution)
 *   0x0800 = NaN (Not a Number)
 *   0x0801 = +INF (Positive Infinity)
 *   0x07FE = -INF (Negative Infinity)
 *
 * Important: Mantissa is signed 12-bit value in two's complement format.
 * For positive values (0-2047), bits 0-11 contain the value directly.
 * For negative values, use two's complement representation.
 */
static uint16_t float_to_sfloat(float value)
{
    int16_t mantissa;
    int8_t exponent = 0;

    /* Handle special cases */
    if (value == 0.0f) {
        return 0x07FF; /* NRes (Not at this Resolution) */
    }

    /* For SpO2 values 0-100%, use exponent 0 */
    /* Values fit in mantissa range (-2048 to 2047) with exponent 0 */
    mantissa = (int16_t)value;

    /* Clamp mantissa to valid range */
    if (mantissa < -2048) {
        mantissa = -2048;
    } else if (mantissa > 2047) {
        mantissa = 2047;
    }

    /* SFLOAT format: mantissa (12 bits, signed) + exponent (4 bits, signed) */
    /* Mantissa in bits 0-11 (signed two's complement), exponent in bits 12-15
     */
    /* For positive values, just mask to 12 bits */
    /* For negative values, two's complement is already correct when masked */
    uint16_t mantissa_u16 = (uint16_t)(mantissa & 0x0FFF);

    /* Exponent: signed 4-bit value in two's complement */
    /* For exponent 0, we use 0x0 (not 0x8) */
    uint16_t exponent_u16 = (uint16_t)((exponent & 0x0F) << 12);

    return mantissa_u16 | exponent_u16;
}

static bool notifications_enabled = false;
static bool indications_enabled = false;
static struct bt_conn *current_conn;

static void spo2_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                                 uint16_t value)
{
    ARG_UNUSED(attr);
    notifications_enabled = (value & BT_GATT_CCC_NOTIFY) != 0;
    indications_enabled = (value & BT_GATT_CCC_INDICATE) != 0;

    if (notifications_enabled && indications_enabled) {
        ble_log_info("SpO2 notifications and indications enabled");
    } else if (notifications_enabled) {
        ble_log_info("SpO2 notifications enabled");
    } else if (indications_enabled) {
        ble_log_info("SpO2 indications enabled");
    } else {
        ble_log_info("SpO2 notifications and indications disabled");
    }
}

/* Oxygen Saturation Service Declaration */
/* According to Bluetooth SIG spec, SpO2 Measurement supports both NOTIFY and
 * INDICATE */
static struct bt_gatt_attr spo2_attrs[] = {
    BT_GATT_PRIMARY_SERVICE(BT_UUID_OSS),
    BT_GATT_CHARACTERISTIC(BT_UUID_SPO2_MEASUREMENT,
                           BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE,
                           BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(spo2_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};

static struct bt_gatt_service spo2_svc = {
    .attrs = NULL,
    .attr_count = 0,
};

void spo2_send(uint8_t spo2_value, uint16_t pulse_rate)
{
    /* PLX Continuous Measurement format according to Bluetooth SIG PLX Service
     * spec: According to PLX Service spec (0x1822), PLX Continuous Measurement
     * format: Byte 0: Flags (1 octet)
     *   - Bit 0: SpO2PR-Normal present (SpO2 and Pulse Rate both present)
     *   - Bit 1: Measurement Status present
     *   - Bit 2: Device and Sensor Status present
     *   - Bit 3: Pulse Amplitude Index present
     *   - Bit 4: Device Clock present
     * Byte 1-2: SpO2 (SFLOAT, 2 octets, little-endian) - if bit 0 set
     * Byte 3-4: Pulse Rate (SFLOAT, 2 octets, little-endian) - if bit 0 set
     * Byte 5-6: Measurement Status (16-bit, 2 octets, little-endian) - if bit 1
     * set
     *
     * Note: According to spec, if bit 0 is set, BOTH SpO2 and PR must be
     * present. For our implementation, we send:
     * - Flags: 0x03 (SpO2PR-Normal present, Measurement Status present)
     * - SpO2 SFLOAT (2 bytes) - value in percent
     * - Pulse Rate SFLOAT (2 bytes) - heart rate in bpm
     * - Measurement Status (2 bytes)
     * Total: 7 bytes
     */
    static uint8_t spo2_data[7];
    struct bt_conn *conn = current_conn;
    uint16_t spo2_sfloat;
    uint16_t pr_sfloat;
    uint16_t measurement_status =
        0x0001; /* Bit 0: Valid SpO2-PR Normal measurement */

    /* Check if notifications or indications are enabled */
    if (!notifications_enabled && !indications_enabled) {
        return;
    }

    /* Validate SpO2 value range (0-100%) */
    if (spo2_value > 100) {
        ble_log_error("Invalid SpO2 value: %d%%, clamping to 100%%",
                      spo2_value);
        spo2_value = 100;
    }

    /* Validate Pulse Rate range (0-300 bpm typical, but allow up to 255 for
     * uint8_t compatibility) */
    if (pulse_rate > 300) {
        ble_log_error("Invalid pulse rate: %d bpm, clamping to 300 bpm",
                      pulse_rate);
        pulse_rate = 300;
    }

    /* Convert to SFLOAT format */
    spo2_sfloat = float_to_sfloat((float)spo2_value);
    pr_sfloat = float_to_sfloat((float)pulse_rate);

    /* Format PLX Continuous Measurement:
     * Flags byte:
     *   Bit 0 = 1: SpO2PR-Normal present (SpO2 AND Pulse Rate both present)
     *   Bit 1 = 1: Measurement Status present
     *   Bits 2-7 = 0: Other fields not present
     */
    spo2_data[0] =
        0x03; /* 0b00000011: SpO2PR-Normal and Measurement Status present */

    /* SpO2 value in SFLOAT format (little-endian: LSO, MSO) */
    sys_put_le16(spo2_sfloat, &spo2_data[1]);

    /* Pulse Rate in SFLOAT format (little-endian: LSO, MSO)
     * Heart rate in beats per minute (bpm)
     */
    sys_put_le16(pr_sfloat, &spo2_data[3]);

    /* Measurement Status (16-bit, little-endian: LSO, MSO)
     * According to PLX spec, Measurement Status format:
     * Bit 0: Valid SpO2-PR Normal (1 = valid, 0 = invalid)
     * Bit 1: Valid SpO2-PR Fast (1 = valid, 0 = invalid)
     * Bit 2: Valid SpO2-PR Slow (1 = valid, 0 = invalid)
     * Bits 3-15: Reserved for future use
     *
     * Since we only send SpO2PR-Normal, we set bit 0 = 1
     */
    sys_put_le16(measurement_status, &spo2_data[5]);

    /* Debug: Log data format for verification (show raw bytes) */
    ble_log_info("SpO2 data: Flags=0x%02X, SpO2=0x%04X (%d%%), PR=0x%04X "
                 "(%d bpm), Status=0x%04X",
                 spo2_data[0], spo2_sfloat, spo2_value, pr_sfloat, pulse_rate,
                 measurement_status);
    ble_log_info(
        "SpO2 raw bytes: [0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X]",
        spo2_data[0], spo2_data[1], spo2_data[2], spo2_data[3], spo2_data[4],
        spo2_data[5], spo2_data[6]);

    /* Send data using indicate (preferred) or notify */
    if (conn != NULL) {
        if (indications_enabled) {
            /* Use indicate for guaranteed delivery (requires confirmation) */
            /* In Zephyr, bt_gatt_indicate uses bt_gatt_indicate_params
             * structure */
            struct bt_gatt_indicate_params indicate_params = {
                .attr = &spo2_svc.attrs[1],
                .data = spo2_data,
                .len = sizeof(spo2_data),
            };
            bt_gatt_indicate(conn, &indicate_params);
            ble_log_info("SpO2: %d%%, PR: %d bpm sent via indicate", spo2_value,
                         pulse_rate);
        } else if (notifications_enabled) {
            /* Fallback to notify if indicate not enabled */
            bt_gatt_notify(conn, &spo2_svc.attrs[1], spo2_data,
                           sizeof(spo2_data));
            ble_log_info("SpO2: %d%%, PR: %d bpm sent via notify", spo2_value,
                         pulse_rate);
        }
    } else {
        /* If no specific connection, try to send to all */
        if (indications_enabled) {
            /* Indicate requires connection, fallback to notify for broadcast */
            bt_gatt_notify(NULL, &spo2_svc.attrs[1], spo2_data,
                           sizeof(spo2_data));
            ble_log_info(
                "SpO2: %d%%, PR: %d bpm sent via notify (broadcast, indicate "
                "requires connection)",
                spo2_value, pulse_rate);
        } else if (notifications_enabled) {
            bt_gatt_notify(NULL, &spo2_svc.attrs[1], spo2_data,
                           sizeof(spo2_data));
            ble_log_info("SpO2: %d%%, PR: %d bpm sent via notify (broadcast)",
                         spo2_value, pulse_rate);
        }
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
