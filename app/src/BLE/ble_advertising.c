#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "BLE/ble_advertising.h"

LOG_MODULE_REGISTER(ble_advertising, CONFIG_LOG_DEFAULT_LEVEL);

/* Advertising data */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
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
        LOG_WRN("Advertising not active");
        return 0;
    }

    LOG_INF("Stopping BLE advertising...");

    err = bt_le_adv_stop();
    if (err) {
        LOG_ERR("Failed to stop advertising (err %d)", err);
        return err;
    }

    advertising_active = false;
    LOG_INF("Advertising stopped");

    return 0;
}
