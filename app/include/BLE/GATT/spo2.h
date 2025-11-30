/** @file
 *  @brief SpO2 Service sample
 */

/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#ifdef __cplusplus
extern "C" {
#endif

void spo2_send(uint8_t spo2_value, uint16_t pulse_rate);
void spo2_set_connection(struct bt_conn *conn);
int spo2_service_register(void);
int spo2_service_unregister(void);
struct bt_gatt_service *spo2_get_service(void);

#ifdef __cplusplus
}
#endif
