#ifndef OUR_DRIVER_H_
#define OUR_DRIVER_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

/* Custom attributes for the sensor_attr_set() extension */
enum our_driver_attribute {
    OUR_DRIVER_ATTR_BLINK_ENABLE = SENSOR_ATTR_PRIV_START,
    OUR_DRIVER_ATTR_BLINK_PERIOD_MS,
};

/* Full on+off cycle, 50% duty */
#define OUR_DRIVER_BLINK_PERIOD_MIN_MS      20
#define OUR_DRIVER_BLINK_PERIOD_MAX_MS      10000
#define OUR_DRIVER_BLINK_PERIOD_DEFAULT_MS  1000

/** Start (true) or stop (false) blinking. Stopping turns the LED off. */
static inline int our_driver_blink_enable(const struct device *dev, bool enable)
{
    struct sensor_value val;

    val.val1 = enable ? 1 : 0;
    val.val2 = 0;
    return sensor_attr_set(dev, SENSOR_CHAN_PROX,
                           (enum sensor_attribute)OUR_DRIVER_ATTR_BLINK_ENABLE, &val);
}

/** Set the blink period (one full on+off cycle) in ms. Takes effect immediately. */
static inline int our_driver_blink_set_period_ms(const struct device *dev, uint32_t period_ms)
{
    struct sensor_value val;

    val.val1 = (int32_t)period_ms;
    val.val2 = 0;
    return sensor_attr_set(dev, SENSOR_CHAN_PROX,
                           (enum sensor_attribute)OUR_DRIVER_ATTR_BLINK_PERIOD_MS, &val);
}

#endif /* OUR_DRIVER_H_ */