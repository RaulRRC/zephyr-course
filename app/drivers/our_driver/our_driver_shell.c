#include <zephyr/shell/shell.h>
#include <zephyr/drivers/sensor.h>

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
    SHELL_CMD_ARG(fetch, NULL, "Get channel of my/our driver", cmd_fetch_handler, 2, 0),
    SHELL_CMD_ARG(read, NULL, "Get channel of my/our driver", cmd_read_handler, 2, 0),
    SHELL_CMD_ARG(info, NULL, "Get channel of my/our driver", cmd_info_handler, 2, 0),
    SHELL_SUBCMD_SET_END,
);

SHELL_CMD_REGISTER(sensorroot, &sensorroot, "our driver set of commands", NULL);