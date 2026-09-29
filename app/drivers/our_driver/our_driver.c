#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT our_driver

LOG_MODULE_REGISTER(our_driver, LOG_LEVEL_INF);



struct our_driver_config {
    struct gpio_dt_spec led;
};

struct our_driver_data {
    bool led_on;
};

static int channel_get_my_impl(const struct device *dev,
                               enum sensor_channel chan,
                               struct sensor_value *val){
    struct our_driver_data *data = dev->data;
        /* Arbitrary channel choice for a dummy driver */
    if (chan != SENSOR_CHAN_PROX) {
        return -ENOTSUP;
    }
    val->val1 = data->led_on ? 1 : 0;
    val->val2 = 0;
    LOG_INF("Hello From Channel Get, channel %d", chan);
    return 0;
}

static int channel_fetch_my_impl(const struct device *dev,
                                enum sensor_channel chan){
    
    const struct our_driver_config *cfg = dev->config;
    struct our_driver_data *data = dev->data;
    int ret;
    data->led_on = !data->led_on;
    ret = gpio_pin_set_dt(&cfg->led, data->led_on);
    if (ret < 0) {
        LOG_ERR("Failed to set LED (%d)", ret);
        return ret;
    }    

    LOG_INF("Hello From Channel Fetch, channel %d", chan);
    return 0;
}

static DEVICE_API(sensor, api_iomico_lecture) = {
    .sample_fetch = channel_fetch_my_impl,
    .channel_get = channel_get_my_impl,
    
};

static int our_driver_init(const struct device *dev){
    const struct our_driver_config *cfg = dev->config;
    int ret;
    if (!gpio_is_ready_dt(&cfg->led)) {
        LOG_ERR("LED GPIO controller not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&cfg->led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        return ret;
    }
    LOG_INF("Device Initialized!");
    return 0;
}

#define OUR_DRIVER_INIT(inst)                                          \
    static struct our_driver_data our_driver_data_##inst;              \
    static const struct our_driver_config our_driver_cfg_##inst = {    \
        .led = GPIO_DT_SPEC_GET(DT_INST_PHANDLE(inst, led), gpios),    \
    };                                                                 \
    DEVICE_DT_INST_DEFINE(inst, our_driver_init, NULL,                 \
                          &our_driver_data_##inst,                     \
                          &our_driver_cfg_##inst,                      \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,    \
                          &api_iomico_lecture);

DT_INST_FOREACH_STATUS_OKAY(OUR_DRIVER_INIT)