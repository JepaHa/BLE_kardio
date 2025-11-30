/** @file
 *  @brief GATT Services Management
 */

/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GATT_SERVICES_H
#define GATT_SERVICES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GATT Service types
 */
enum gatt_service_type {
    GATT_SERVICE_HRS = 0,
    GATT_SERVICE_SPO2,
    GATT_SERVICE_COUNT
};

/**
 * @brief Register all GATT services
 *
 * @return 0 on success, negative error code on failure
 */
int gatt_services_register_all(void);

/**
 * @brief Register a specific GATT service
 *
 * @param service_type Type of service to register (from enum gatt_service_type)
 * @param param Optional parameter (e.g., blsc for HRS service)
 * @return 0 on success, negative error code on failure
 */
int gatt_service_register(enum gatt_service_type service_type, uint8_t param);

/**
 * @brief Unregister a specific GATT service
 *
 * @param service_type Type of service to unregister (from enum
 * gatt_service_type)
 * @return 0 on success, negative error code on failure
 */
int gatt_service_unregister(enum gatt_service_type service_type);

#ifdef __cplusplus
}
#endif

#endif /* GATT_SERVICES_H */
