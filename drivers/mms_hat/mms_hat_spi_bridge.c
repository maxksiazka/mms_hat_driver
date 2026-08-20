/* SPDX-License-Identifier: Apache-2.0 */

#define DT_DRV_COMPAT konar_mms_hat_spi_bridge

#include <drivers/mms_chainbus.h>
#include <drivers/mms_hat/mms_hat_base.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mms_hat_spi_bridge, CONFIG_SPI_LOG_LEVEL);

struct mms_hat_spi_config {
    const struct device* parent_spi;
    const struct device* hat_container;
};

struct mms_hat_spi_data {
    int slot;
};

static int mms_hat_spi_transceive(const struct device* dev, const struct spi_config* config,
                                  const struct spi_buf_set* tx_bufs,
                                  const struct spi_buf_set* rx_bufs) {
    const struct mms_hat_spi_config* cfg = dev->config;
    struct mms_hat_spi_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = spi_transceive(cfg->parent_spi, config, tx_bufs, rx_bufs);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_spi_release(const struct device* dev, const struct spi_config* config) {
    const struct mms_hat_spi_config* cfg = dev->config;
    return spi_release(cfg->parent_spi, config);
}

static const struct spi_driver_api mms_hat_spi_api = {
    .transceive = mms_hat_spi_transceive,
    .release = mms_hat_spi_release,
};

static int mms_hat_spi_init(const struct device* dev) {
    const struct mms_hat_spi_config* cfg = dev->config;
    struct mms_hat_spi_data* data = dev->data;

    if (!device_is_ready(cfg->parent_spi)) {
        LOG_ERR("Parent physical SPI bus not ready");
        return -ENODEV;
    }

    data->slot = mms_hat_get_slot(cfg->hat_container);
    if (data->slot < 0) {
        LOG_ERR("Bridge '%s' cannot find valid slot from parent HAT '%s'", dev->name,
                cfg->hat_container->name);
        return -ENODEV;
    }

    LOG_INF("SPI Bridge active on Slot %d -> HW Bus: %s", data->slot, cfg->parent_spi->name);
    return 0;
}

#define MMS_HAT_SPI_INIT(inst)                                                                     \
    static const struct mms_hat_spi_config mms_hat_spi_cfg_##inst = {                              \
        .parent_spi = DEVICE_DT_GET(DT_INST_PHANDLE(inst, spi_bus)),                               \
        .hat_container = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                      \
    };                                                                                             \
    static struct mms_hat_spi_data mms_hat_spi_dat_##inst;                                         \
    DEVICE_DT_INST_DEFINE(inst, mms_hat_spi_init, NULL, &mms_hat_spi_dat_##inst,                   \
                          &mms_hat_spi_cfg_##inst, POST_KERNEL,                                    \
                          CONFIG_MMS_HAT_SPI_BRIDGE_INIT_PRIORITY, &mms_hat_spi_api);

DT_INST_FOREACH_STATUS_OKAY(MMS_HAT_SPI_INIT)
