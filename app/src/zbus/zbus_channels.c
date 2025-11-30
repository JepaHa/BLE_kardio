/** @file
 *  @brief Zbus channels implementation for BLE_Kardio
 *
 * This file implements zbus channels for communication between simulators
 * and BLE manager.
 */

/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>
#include <stdint.h>

#include "zbus/zbus_channels.h"

LOG_MODULE_REGISTER(zbus_channels, CONFIG_LOG_DEFAULT_LEVEL);

/* Register zbus listener for sensor data channel */
/* The handler function is defined in ble_manager.c */
ZBUS_LISTENER_DEFINE(ble_manager_sensor_listener,
                     ble_manager_sensor_data_handler);

/* Sensor data channel (SpO2 + Heart Rate) */
ZBUS_CHAN_DEFINE(sensor_data_chan,                            /* Channel name */
                 struct sensor_data,                          /* Message type */
                 NULL,                                        /* Validator */
                 NULL,                                        /* User data */
                 ZBUS_OBSERVERS(ble_manager_sensor_listener), /* Observers */
                 ZBUS_MSG_INIT(.pulse = 0, .spo2 = 0) /* Initial value */
);
