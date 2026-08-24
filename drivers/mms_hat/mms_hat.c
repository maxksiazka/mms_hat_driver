/* SPDX-License-Identifier: Apache-2.0 */

#define DT_DRV_COMPAT konar_mms_hat

#include <drivers/mms_chainbus.h>
#include <drivers/mms_hat/mms_hat_base.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mms_hat, CONFIG_LOG_DEFAULT_LEVEL);

struct mms_hat_config {
    bool is_static;
    int position;
    uint16_t type_id;
};

static int mms_hat_init(const struct device* dev) {
    const struct mms_hat_config* cfg = dev->config;
    struct mms_hat_data* data = dev->data;
    if (cfg->position < 0 && cfg->type_id == 0) {
        LOG_ERR("HAT '%s' must have either a 'reg' or 'type-id' property in Devicetree!",
                dev->name);
        return -EINVAL;
    }

    if (cfg->is_static) {
        data->slot = chainbus_claim_slot_static(cfg->position);
        if (data->slot < 0) {
            LOG_ERR("HAT '%s' failed to claim static Chainbus slot %d!", dev->name,
                    cfg->position);
            return -ENODEV;
        }
    } else {
        data->slot = chainbus_claim_slot_dynamic(cfg->type_id);
        if (data->slot < 0) {
            LOG_ERR("HAT '%s' failed to claim dynamic Chainbus slot for type-id %d!", dev->name,
                    cfg->type_id);
            return -ENODEV;
        }
    }

    if (data->slot < 0) {
        LOG_ERR("HAT '%s' failed to claim a valid Chainbus slot!", dev->name);
        return -ENODEV;
    }
    LOG_INF("HAT '%s' claimed Chainbus Slot %d", dev->name, data->slot);
    return 0;
}

#define MMS_HAT_INIT(inst)                                                                         \
    static const struct mms_hat_config mms_hat_cfg_##inst = {                                      \
        .is_static = DT_INST_NODE_HAS_PROP(inst, reg),                                             \
        .position = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, reg), (DT_INST_REG_ADDR(inst)), (-1)), \
        .type_id = DT_INST_PROP_OR(inst, type_id, 0),                                              \
    };                                                                                             \
    static struct mms_hat_data mms_hat_data_##inst = {.slot = -1};                                 \
    DEVICE_DT_INST_DEFINE(inst, mms_hat_init, NULL, &mms_hat_data_##inst, &mms_hat_cfg_##inst,     \
                          POST_KERNEL, CONFIG_MMS_HAT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MMS_HAT_INIT)
