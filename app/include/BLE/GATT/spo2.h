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

#ifdef __cplusplus
extern "C" {
#endif

void spo2_init(void);
void spo2_notify(void);
void spo2_set_connection(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif
