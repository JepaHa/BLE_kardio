/** @file
 *  @brief SpO2 Simulator implementation
 */

/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdint.h>

#include "BLE/GATT/spo2.h"
#include "BLE/GATT/gatt_services.h"
#include "simulator/spo2_simulator.h"

LOG_MODULE_REGISTER(spo2_simulator, CONFIG_LOG_DEFAULT_LEVEL);

#define SPO2_SIMULATOR_STACK_SIZE        1024
#define SPO2_SIMULATOR_PRIORITY          5
#define SPO2_SIMULATOR_INTERVAL_MS       5000
#define SERVICE_TEST_UNREGISTER_DELAY_MS 10000 /* 10 seconds unregistered */
#define SERVICE_TEST_REREGISTER_DELAY_MS                                       \
    5000 /* 5 seconds before re-register                                       \
          */

static uint8_t spo2_value = 98U; /* SpO2 value in percent (95-100 typical) */
static bool service_test_enabled = true;

static void spo2_simulator_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("SpO2 simulator thread started");

    /* Wait a bit for BLE to initialize */
    k_msleep(2000);

    while (1) {
        if (service_test_enabled) {
            /* Test: Unregister SpO2 service */
            LOG_INF("Test: Unregistering SpO2 service...");
            int err = gatt_service_unregister(GATT_SERVICE_SPO2);
            if (err) {
                LOG_ERR("Failed to unregister SpO2 service (err %d)", err);
            } else {
                LOG_INF("SpO2 service unregistered, waiting %d ms...",
                        SERVICE_TEST_UNREGISTER_DELAY_MS);
                k_msleep(SERVICE_TEST_UNREGISTER_DELAY_MS);

                /* Test: Re-register SpO2 service */
                LOG_INF("Test: Re-registering SpO2 service...");
                err = gatt_service_register(GATT_SERVICE_SPO2, 0);
                if (err) {
                    LOG_ERR("Failed to re-register SpO2 service (err %d)", err);
                } else {
                    LOG_INF("SpO2 service re-registered successfully");
                    k_msleep(SERVICE_TEST_REREGISTER_DELAY_MS);
                }
            }

            /* Disable test after first run */
            service_test_enabled = false;
        }

        /* Simulate realistic SpO2 variation (95-100%) */
        spo2_value++;
        if (spo2_value > 100U) {
            spo2_value = 95U;
        }

        /* Send SpO2 measurement */
        spo2_send(spo2_value);

        /* Wait 5 seconds before next measurement */
        k_msleep(SPO2_SIMULATOR_INTERVAL_MS);
    }
}

K_THREAD_DEFINE(spo2_simulator_thread_id, SPO2_SIMULATOR_STACK_SIZE,
                spo2_simulator_thread, NULL, NULL, NULL,
                SPO2_SIMULATOR_PRIORITY, 0, 0);

void spo2_simulator_init(void)
{
    LOG_INF("SpO2 simulator initialized");
    /* Thread is automatically started by K_THREAD_DEFINE */
}
