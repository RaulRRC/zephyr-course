#include <zephyr/shell/shell.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>
#include <errno.h>
#include "our_driver.h"

static int cmd_blink_handler(const struct shell * sh, int argc, char** argv)
{
    const struct device *dev = shell_device_get_binding(argv[1]);
    if(!dev){
        shell_error(sh, "could not find device %s", argv[1]);
        return -EFAULT;
    }

    char *end;
    long val;

    errno = 0;
    val = strtol(argv[2], &end, 10);

    if (errno != 0 || *end != '\0' || end == argv[2]) {
    shell_error(sh, "Invalid number: %s", argv[2]);
    return -EINVAL;
    }

    if (val < 20 || val > 1000){
    shell_error(sh, "Value outside of range (20 - 1000) ms: %s", argv[2]);
    return -EINVAL;
    }
    int ret = our_driver_blink_set_period_ms(dev, val);

    if (ret != 0){
        shell_error(sh, "Could not get channel, got %d", ret);
        return -EFAULT;
    }
    shell_info(sh, "Set blinking speed to %s", argv[2]);
    return 0;
}

static int cmd_set_handler(const struct shell * sh, int argc, char** argv)
{
    const struct device *dev = shell_device_get_binding(argv[1]);
    if(!dev){
        shell_error(sh, "could not find device %s", argv[1]);
        return -EFAULT;
    }

    char *end;
    long val;

    errno = 0;
    val = strtol(argv[2], &end, 10);

    if (errno != 0 || *end != '\0' || end == argv[2] || (val != 0 && val != 1)) {
    shell_error(sh, "Value must be 0 (False) or 1 (True) %s", argv[2]);
    return -EINVAL;
    }


    int ret = our_driver_blink_enable(dev, val);

    if (ret != 0){
        shell_error(sh, "Could not set device, got %d", ret);
        return -EFAULT;
    }

    shell_info(sh, "Device Set");
    return 0;
}

static int cmd_read_handler(const struct shell * sh, int argc, char** argv)
{
    const struct device *dev = shell_device_get_binding(argv[1]);
    if(!dev){
        shell_error(sh, "could not find device %s", argv[1]);
        return -EFAULT;
    }
    struct sensor_value val;

    int ret = sensor_channel_get(dev, SENSOR_CHAN_PROX, &val);
    if (ret != 0){
        shell_error(sh, "Could not get channel, got %d", ret);
        return -EFAULT;
    }
    shell_info(sh, "%d", val.val1);
    return 0;
}

static int cmd_fetch_handler(const struct shell * sh, int argc, char** argv)
{
    const struct device *dev = shell_device_get_binding(argv[1]);
    if(!dev){
        shell_error(sh, "could not find device %s", argv[1]);
        return -EFAULT;
    }

    int ret = sensor_sample_fetch(dev);

    if (ret != 0){
        shell_error(sh, "Could not fetch channel, got %d", ret);
        return -EFAULT;
    }

    shell_info(sh, "Channel fetched");
    return 0;
}

static int cmd_info_handler(const struct shell * sh, int argc, char** argv)
{
    const struct device *dev = shell_device_get_binding(argv[1]);
    if(!dev){
        shell_error(sh, "could not find device %s", argv[1]);
        return -EFAULT;
    }

    shell_info(sh, "Device %s is ready to use", argv[1]);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sensorroot, 
    SHELL_CMD_ARG(enable, NULL, "Enable blinking, 0 (False) or 1 (True)", cmd_set_handler, 3, 0),
    SHELL_CMD_ARG(blinkspeed, NULL, "Set Blinking Delay (20 - 1000) ms", cmd_blink_handler, 3, 0),
    SHELL_CMD_ARG(fetch, NULL, "Toggle Led", cmd_fetch_handler, 2, 0),
    SHELL_CMD_ARG(read, NULL, "Red current led status", cmd_read_handler, 2, 0),    
    SHELL_CMD_ARG(info, NULL, "Retrieve device status", cmd_info_handler, 2, 0),
    SHELL_SUBCMD_SET_END,
);

SHELL_CMD_REGISTER(sensorroot, &sensorroot, "our driver set of commands", NULL);