#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "BLE/ble_advertising.h"

LOG_MODULE_REGISTER(ble_advertising, CONFIG_LOG_DEFAULT_LEVEL);

/* Advertising data */
static const uint8_t heart_rate_uuid[] = {0x0D,
                                          0x18}; /* 0x180D in little-endian */
static const uint8_t pulse_oximeter_appearance[] = {
    0xC0, 0x03}; /* 0x03C0 in little-endian */

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    /* Heart Rate Service UUID (0x180D) */
    BT_DATA(BT_DATA_UUID16_SOME, heart_rate_uuid, sizeof(heart_rate_uuid)),
    /* Pulse Oximeter Appearance (0x03C0) */
    BT_DATA(BT_DATA_GAP_APPEARANCE, pulse_oximeter_appearance,
            sizeof(pulse_oximeter_appearance)),
    BT_DATA(BT_DATA_NAME_COMPLETE, "BLE_Kardio", 10),
};

static bool advertising_active = false;

int ble_advertising_start(void)
{
    int err;

    if (advertising_active) {
        LOG_WRN("Advertising already active");
        return 0;
    }

    LOG_INF("Starting BLE advertising...");

    struct bt_le_adv_param adv_param = {
        .id = BT_ID_DEFAULT,
        .sid = 0,
        .secondary_max_skip = 0,
        .options = BT_LE_ADV_OPT_CONN,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
        .peer = NULL,
    };

    err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err) {
        LOG_ERR("Advertising failed to start (err %d)", err);
        return err;
    }

    advertising_active = true;
    LOG_INF("Advertising started successfully");

    return 0;
}

int ble_advertising_stop(void)
{
    int err;

    if (!advertising_active) {
        /* Already stopped, but try to stop anyway to ensure clean state */
        bt_le_adv_stop();
        return 0;
    }

    LOG_INF("Stopping BLE advertising...");

    err = bt_le_adv_stop();
    /* Always reset flag, even if stop returns error (stack may have already
     * stopped it) */
    advertising_active = false;

    if (err) {
        LOG_WRN("Advertising stop returned error (err %d), but flag reset",
                err);
        /* Don't return error, as advertising is effectively stopped */
        return 0;
    }

    LOG_INF("Advertising stopped");
    return 0;
}
