#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/random/random.h>

/* Override weak bt_rand implementation for ESP32 */
int bt_rand(void *buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return -EINVAL;
    }

    return sys_csrand_get(buf, len);
}
