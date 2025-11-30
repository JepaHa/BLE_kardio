/** @file
 *  @brief Zbus channels definitions for BLE_Kardio
 *
 * This file defines zbus channels for communication between simulators
 * and BLE manager.
 */

/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZBUS_CHANNELS_H
#define ZBUS_CHANNELS_H

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Combined sensor data structure (SpO2 + Heart Rate) */
struct sensor_data {
    uint16_t pulse; /* Пульс (Heart rate) in bpm */
    uint16_t spo2;  /* SpO2 value */
};

/* Forward declaration of handler function */
void ble_manager_sensor_data_handler(const struct zbus_channel *chan);

/* Zbus channel for sensor data (SpO2 and Heart Rate) */
ZBUS_CHAN_DECLARE(sensor_data_chan);

#ifdef __cplusplus
}
#endif

#endif /* ZBUS_CHANNELS_H */
