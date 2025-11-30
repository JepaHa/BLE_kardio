#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "BLE/ble_init.h"
#include "simulator/spo2_simulator.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

int main(void)
{
    /* Initialize BLE (includes LED initialization) */
    ble_init();

    /* Initialize SpO2 simulator - starts working immediately */
    /* Connection will be established automatically when first data is sent */
    spo2_simulator_init();

    /* Main loop - just sleep, LED is controlled by Bluetooth state */
    while (1) {
        k_msleep(1000);
    }
    return 0;
}
