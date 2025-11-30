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

#include "BLE/ble_manager.h"
#include "simulator/spo2_simulator.h"

LOG_MODULE_REGISTER(spo2_simulator, CONFIG_LOG_DEFAULT_LEVEL);

#define SPO2_SIMULATOR_STACK_SIZE  1024
#define SPO2_SIMULATOR_PRIORITY    5
#define SPO2_SIMULATOR_INTERVAL_MS 60000 /* 60 seconds */

static uint8_t spo2_value = 98U; /* SpO2 value in percent (95-100 typical) */

static void spo2_simulator_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    LOG_INF("SpO2 simulator thread started");

    /* Wait a bit for system to initialize */
    k_msleep(2000);

    while (1) {
        /* Simulate realistic SpO2 variation (95-100%) */
        spo2_value++;
        if (spo2_value > 100U) {
            spo2_value = 95U;
        }

        /* Use high-level BLE Manager to send SpO2 data */
        /* This function handles all Bluetooth management automatically */
        int err = ble_manager_send_spo2(spo2_value);
        if (err) {
            LOG_ERR("Failed to send SpO2 data (err %d)", err);
        }

        /* Wait 60 seconds before next measurement */
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
