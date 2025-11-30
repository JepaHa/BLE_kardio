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
#include <zephyr/zbus/zbus.h>
#include <stdint.h>

#include "zbus/zbus_channels.h"
#include "simulator/spo2_simulator.h"

LOG_MODULE_REGISTER(spo2_simulator, CONFIG_LOG_DEFAULT_LEVEL);

#define SPO2_SIMULATOR_STACK_SIZE  1024
#define SPO2_SIMULATOR_PRIORITY    5
#define SPO2_SIMULATOR_INTERVAL_MS 10000 /* 10 seconds */

static uint8_t spo2_value = 98U;  /* SpO2 value in percent (95-100 typical) */
static uint16_t pulse_rate = 72U; /* Heart rate in bpm (60-100 typical) */

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

        /* Simulate realistic heart rate variation (60-100 bpm) */
        pulse_rate += 2;
        if (pulse_rate > 100U) {
            pulse_rate = 60U;
        }

        /* Publish sensor data to zbus channel */
        struct sensor_data data = {.pulse = pulse_rate, .spo2 = spo2_value};

        int err = zbus_chan_pub(&sensor_data_chan, &data, K_NO_WAIT);
        if (err) {
            LOG_ERR("Failed to publish sensor data to zbus (err %d)", err);
        } else {
            LOG_INF("Published sensor data to zbus: pulse=%d, spo2=%d",
                    pulse_rate, spo2_value);
        }

        /* Wait 10 seconds before next measurement */
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
