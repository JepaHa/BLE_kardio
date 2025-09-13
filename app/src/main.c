#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
    int ret = 0;
    bool led_state = true;
    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("%s is not ready! Exit.", led.port->name);
        return -1;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("%s: %s. Exit.", led.port->name, strerror(-ret));
        return -1;
    }

    LOG_INF("Run Blinky sample.");
    while (1) {
        ret = gpio_pin_toggle_dt(&led);
        if (ret < 0) {
            LOG_ERR("%s: %s. Exit.", led.port->name, strerror(-ret));
            return -1;
        }
        led_state = !led_state;
        LOG_INF("LED state: %s", led_state ? "ON" : "OFF");
        k_msleep(500);
    }
    return 0;
}
