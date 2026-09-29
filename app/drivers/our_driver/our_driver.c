#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "our_driver.h"

#define DT_DRV_COMPAT our_driver

LOG_MODULE_REGISTER(our_driver, LOG_LEVEL_INF);

struct our_driver_config {
    struct gpio_dt_spec led;
};

struct our_driver_data {
    const struct device *dev;
    struct k_work_delayable blink_work;
    struct k_mutex lock;
    bool led_on;
    bool blink_enabled;
    uint32_t blink_period_ms;
};

static void blink_work_handler(struct k_work *work)
{
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    struct our_driver_data *data = CONTAINER_OF(dwork, struct our_driver_data, blink_work);
    const struct our_driver_config *cfg = data->dev->config;
    int ret;

    k_mutex_lock(&data->lock, K_FOREVER);

    /* Re-check under the lock: we may have been disabled while waiting for it */
    if (!data->blink_enabled) {
        k_mutex_unlock(&data->lock);
        return;
    }

    data->led_on = !data->led_on;
    ret = gpio_pin_set_dt(&cfg->led, data->led_on);
    if (ret < 0) {
        LOG_ERR("Failed to set LED (%d)", ret);
    }

    k_work_reschedule(&data->blink_work, K_MSEC(data->blink_period_ms / 2));
    k_mutex_unlock(&data->lock);
}

static int blink_set_enabled(const struct device *dev, bool enable)
{
    const struct our_driver_config *cfg = dev->config;
    struct our_driver_data *data = dev->data;
    int ret = 0;

    k_mutex_lock(&data->lock, K_FOREVER);

    if (enable == data->blink_enabled) {
        goto out;
    }
    data->blink_enabled = enable;

    if (enable) {
        k_work_reschedule(&data->blink_work, K_NO_WAIT);
    } else {
        /* Non-blocking cancel: safe under the lock. A handler already running
         * will see blink_enabled == false and bail out. */
        k_work_cancel_delayable(&data->blink_work);
        data->led_on = false;
        ret = gpio_pin_set_dt(&cfg->led, 0);
    }
out:
    k_mutex_unlock(&data->lock);
    return ret;
}

static int blink_set_period(const struct device *dev, int32_t period_ms)
{
    struct our_driver_data *data = dev->data;

    if (period_ms < OUR_DRIVER_BLINK_PERIOD_MIN_MS ||
        period_ms > OUR_DRIVER_BLINK_PERIOD_MAX_MS) {
        return -EINVAL;
    }

    k_mutex_lock(&data->lock, K_FOREVER);
    data->blink_period_ms = period_ms;
    if (data->blink_enabled) {
        /* Apply the new speed right away instead of after the pending delay */
        k_work_reschedule(&data->blink_work, K_MSEC(period_ms / 2));
    }
    k_mutex_unlock(&data->lock);
    return 0;
}

static int our_driver_attr_set(const struct device *dev,
                               enum sensor_channel chan,
                               enum sensor_attribute attr,
                               const struct sensor_value *val)
{
    if (chan != SENSOR_CHAN_PROX) {
        return -ENOTSUP;
    }

    switch ((int)attr) {
    case OUR_DRIVER_ATTR_BLINK_ENABLE:
        return blink_set_enabled(dev, val->val1 != 0);
    case OUR_DRIVER_ATTR_BLINK_PERIOD_MS:
        return blink_set_period(dev, val->val1);
    default:
        return -ENOTSUP;
    }
}

static int channel_get_my_impl(const struct device *dev,
                               enum sensor_channel chan,
                               struct sensor_value *val)
{
    struct our_driver_data *data = dev->data;

    if (chan != SENSOR_CHAN_PROX) {
        return -ENOTSUP;
    }

    k_mutex_lock(&data->lock, K_FOREVER);
    val->val1 = data->led_on ? 1 : 0;
    k_mutex_unlock(&data->lock);
    val->val2 = 0;

    LOG_INF("Hello From Channel Get, channel %d", chan);
    return 0;
}

static int channel_fetch_my_impl(const struct device *dev, enum sensor_channel chan)
{
    const struct our_driver_config *cfg = dev->config;
    struct our_driver_data *data = dev->data;
    int ret = 0;

    k_mutex_lock(&data->lock, K_FOREVER);

    /* While blinking, the work item owns the LED; fetch must not fight it */
    if (!data->blink_enabled) {
        data->led_on = !data->led_on;
        ret = gpio_pin_set_dt(&cfg->led, data->led_on);
        if (ret < 0) {
            LOG_ERR("Failed to set LED (%d)", ret);
        }
    }

    k_mutex_unlock(&data->lock);
    if (ret < 0) {
        return ret;
    }

    LOG_INF("Hello From Channel Fetch, channel %d", chan);
    return 0;
}

static DEVICE_API(sensor, api_iomico_lecture) = {
    .sample_fetch = channel_fetch_my_impl,
    .channel_get = channel_get_my_impl,
    .attr_set = our_driver_attr_set,
};

static int our_driver_init(const struct device *dev)
{
    const struct our_driver_config *cfg = dev->config;
    struct our_driver_data *data = dev->data;
    int ret;

    if (!gpio_is_ready_dt(&cfg->led)) {
        LOG_ERR("LED GPIO controller not ready");
        return -ENODEV;
    }
    ret = gpio_pin_configure_dt(&cfg->led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        return ret;
    }

    data->dev = dev;
    data->led_on = false;
    data->blink_enabled = false;
    data->blink_period_ms = OUR_DRIVER_BLINK_PERIOD_DEFAULT_MS;
    k_mutex_init(&data->lock);
    k_work_init_delayable(&data->blink_work, blink_work_handler);

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