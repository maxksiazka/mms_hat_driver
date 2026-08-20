/* SPDX-License-Identifier: Apache-2.0 */

#define DT_DRV_COMPAT konar_mms_hat_i2c_bridge

#include <drivers/mms_chainbus.h>
#include <drivers/mms_hat/mms_hat_base.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mms_hat_i2c_bridge, CONFIG_I2C_LOG_LEVEL);

struct mms_hat_i2c_config {
    const struct device* parent_i2c;
    const struct device* hat_container; /* Reference to parent HAT device */
};

struct mms_hat_i2c_data {
    int slot;
};

static int mms_hat_i2c_configure(const struct device* dev, uint32_t dev_config) {
    const struct mms_hat_i2c_config* cfg = dev->config;
    return i2c_configure(cfg->parent_i2c, dev_config);
}

static int mms_hat_i2c_transfer(const struct device* dev, struct i2c_msg* msgs, uint8_t num_msgs,
                                uint16_t addr) {
    const struct mms_hat_i2c_config* cfg = dev->config;
    struct mms_hat_i2c_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    /* Toggle CS line via chainbus controller */
    chainbus_assert_cs(data->slot, true);
    ret = i2c_transfer(cfg->parent_i2c, msgs, num_msgs, addr);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static const struct i2c_driver_api mms_hat_i2c_api = {
    .configure = mms_hat_i2c_configure,
    .transfer = mms_hat_i2c_transfer,
};

static int mms_hat_i2c_init(const struct device* dev) {
    const struct mms_hat_i2c_config* cfg = dev->config;
    struct mms_hat_i2c_data* data = dev->data;

    if (!device_is_ready(cfg->parent_i2c)) {
        LOG_ERR("Parent physical I2C bus not ready");
        return -ENODEV;
    }

    /* Fetch slot index already claimed by parent HAT driver */
    data->slot = mms_hat_get_slot(cfg->hat_container);
    if (data->slot < 0) {
        LOG_ERR("Bridge '%s' cannot find valid slot from parent HAT '%s'", dev->name,
                cfg->hat_container->name);
        return -ENODEV;
    }

    LOG_INF("I2C Bridge active on Slot %d -> HW Bus: %s", data->slot, cfg->parent_i2c->name);
    return 0;
}

#define MMS_HAT_I2C_INIT(inst)                                                                     \
    static const struct mms_hat_i2c_config mms_hat_i2c_cfg_##inst = {                              \
        .parent_i2c = DEVICE_DT_GET(DT_INST_PHANDLE(inst, i2c_bus)),                               \
        .hat_container = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                      \
    };                                                                                             \
    static struct mms_hat_i2c_data mms_hat_i2c_dat_##inst;                                         \
    DEVICE_DT_INST_DEFINE(inst, mms_hat_i2c_init, NULL, &mms_hat_i2c_dat_##inst,                   \
                          &mms_hat_i2c_cfg_##inst, POST_KERNEL,                                    \
                          CONFIG_MMS_HAT_I2C_BRIDGE_INIT_PRIORITY, &mms_hat_i2c_api);

DT_INST_FOREACH_STATUS_OKAY(MMS_HAT_I2C_INIT)
