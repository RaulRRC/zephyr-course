#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

namespace{
    void test(){
        const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(our_driver0));
        struct sensor_value val;
        auto ret = sensor_channel_get(dev, SENSOR_CHAN_PROX, &val);
        LOG_INF("sensor_channel_get returned %d", ret);
    }
}

int main(void)
{
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(our_driver0));
    struct sensor_value val;
    test();
    bool led_state = true;

    if (!gpio_is_ready_dt(&led)) return 0;

    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) return 0;

    while (1) {
        if (gpio_pin_toggle_dt(&led) < 0) return 0;
        sensor_channel_get(dev, SENSOR_CHAN_PROX, &val);
        k_msleep(SLEEP_TIME_MS);
        sensor_sample_fetch(dev);
        k_msleep(SLEEP_TIME_MS);

    }
    return 0;
}