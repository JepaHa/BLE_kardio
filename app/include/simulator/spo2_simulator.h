/** @file
 *  @brief SpO2 Simulator header
 */

/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SPO2_SIMULATOR_H
#define SPO2_SIMULATOR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize SpO2 simulator
 *
 * Starts a thread that simulates SpO2 measurements and sends them
 * via spo2_send() every 5 seconds.
 */
void spo2_simulator_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SPO2_SIMULATOR_H */
