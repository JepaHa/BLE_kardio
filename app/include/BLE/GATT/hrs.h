/** @file
 *  @brief HRS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hrs_init(uint8_t blsc);
void hrs_notify(void);
void hrs_set_connection(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif