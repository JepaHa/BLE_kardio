/** @file
 *  @brief GATT Services Management implementation
 */

/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>

#include "BLE/ble_log.h"
#include "BLE/GATT/gatt_services.h"
#include "BLE/GATT/hrs.h"
#include "BLE/GATT/spo2.h"

LOG_MODULE_REGISTER(gatt_services, CONFIG_LOG_DEFAULT_LEVEL);

int gatt_service_register(enum gatt_service_type service_type, uint8_t param)
{
    int err;

    switch (service_type) {
    case GATT_SERVICE_HRS:
        err = hrs_service_register(param);
        if (err) {
            ble_log_error("Failed to register HRS service (err %d)", err);
            return err;
        }
        ble_log_info("HRS service registered");
        break;

    case GATT_SERVICE_SPO2:
        err = spo2_service_register();
        if (err) {
            ble_log_error("Failed to register SpO2 service (err %d)", err);
            return err;
        }
        ble_log_info("SpO2 service registered");
        break;

    default:
        ble_log_error("Unknown service type: %d", service_type);
        return -EINVAL;
    }

    return err;
}

int gatt_service_unregister(enum gatt_service_type service_type)
{
    int err;

    switch (service_type) {
    case GATT_SERVICE_HRS:
        err = hrs_service_unregister();
        if (err) {
            ble_log_error("Failed to unregister HRS service (err %d)", err);
            return err;
        }
        ble_log_info("HRS service unregistered");
        break;

    case GATT_SERVICE_SPO2:
        err = spo2_service_unregister();
        if (err) {
            ble_log_error("Failed to unregister SpO2 service (err %d)", err);
            return err;
        }
        ble_log_info("SpO2 service unregistered");
        break;

    default:
        ble_log_error("Unknown service type: %d", service_type);
        return -EINVAL;
    }

    return err;
}

int gatt_services_register_all(void)
{
    int err;

    ble_log_info("Registering all GATT services...");

    /* Register HRS service with body sensor location: Chest (0x01) */
    err = gatt_service_register(GATT_SERVICE_HRS, 0x01);
    if (err) {
        ble_log_error("Failed to register HRS service (err %d)", err);
        return err;
    }

    /* Register SpO2 service */
    err = gatt_service_register(GATT_SERVICE_SPO2, 0);
    if (err) {
        ble_log_error("Failed to register SpO2 service (err %d)", err);
        return err;
    }

    ble_log_info("All GATT services registered successfully");
    return 0;
}
