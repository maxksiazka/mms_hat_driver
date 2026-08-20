/* SPDX-License-Identifier: Apache-2.0 */

#define DT_DRV_COMPAT konar_mms_hat

#include <drivers/mms_chainbus.h>
#include <drivers/mms_hat/mms_hat_base.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mms_hat, CONFIG_LOG_DEFAULT_LEVEL);

struct mms_hat_config {
    int static_position;
    const char* compatible;
};

static int mms_hat_rtc_sd_init(const struct device* dev) {
    const struct mms_hat_config* cfg = dev->config;
    struct mms_hat_data* data = dev->data;

    /* 1. Claim slot from chainbus once for this entire HAT */
    data->slot = chainbus_claim_slot(cfg->compatible, cfg->static_position);
    if (data->slot < 0) {
        LOG_ERR("HAT '%s' failed to claim a valid Chainbus slot!", dev->name);
        return -ENODEV;
    }

    LOG_INF("HAT '%s' claimed Chainbus Slot %d", dev->name, data->slot);
    return 0;
}

#define MMS_HAT_INIT(inst)                                                                         \
    static const struct mms_hat_config mms_hat_cfg_##inst = {                                      \
        .static_position = DT_INST_NODE_HAS_PROP(inst, reg) ? DT_INST_REG_ADDR(inst) : -1,         \
        .compatible = DT_INST_PROP_BY_IDX(inst, compatible, 0),                                    \
    };                                                                                             \
    static struct mms_hat_data mms_hat_data_##inst = {.slot = -1};                                 \
    DEVICE_DT_INST_DEFINE(inst, mms_hat_rtc_sd_init, NULL, &mms_hat_data_##inst,                   \
                          &mms_hat_cfg_##inst, POST_KERNEL, CONFIG_MMS_HAT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MMS_HAT_INIT)
