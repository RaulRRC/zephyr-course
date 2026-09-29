#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define SLEEP_TIME_MS 1000

#define LED_MODE DT_ALIAS(led2)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_MODE, gpios);

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);


int main(void){

    const struct device* driver = DEVICE_DT_GET(DT_NODELABEL(our_driver0));
    struct sensor_value val;
    auto ret = sensor_channel_get(driver, SENSOR_CHAN_AMBIENT_TEMP, &val);
    LOG_INF("Channel ret %d", ret);

    while(1){

        sensor_sample_fetch(driver); 
        sensor_channel_get(driver, SENSOR_CHAN_PROX, &val);

        LOG_INF("LED state: %d\n", val.val1);

        k_msleep(SLEEP_TIME_MS);
    }
    return 0;}