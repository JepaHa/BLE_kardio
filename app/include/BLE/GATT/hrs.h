/** @file
 *  @brief HRS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/bluetooth/gatt.h>

#ifdef __cplusplus
extern "C" {
#endif

void hrs_notify(void);
void hrs_send(uint16_t heartrate_value);
void hrs_set_connection(struct bt_conn *conn);
int hrs_service_register(uint8_t blsc);
int hrs_service_unregister(void);
struct bt_gatt_service *hrs_get_service(void);

#ifdef __cplusplus
}
#endif