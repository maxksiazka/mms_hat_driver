/* SPDX-License-Identifier: Apache-2.0 */

#define DT_DRV_COMPAT konar_mms_hat_uart_bridge

#include <drivers/mms_chainbus.h>
#include <drivers/mms_hat/mms_hat_base.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mms_hat_uart_bridge, CONFIG_UART_LOG_LEVEL);

struct mms_hat_uart_config {
    const struct device* parent_uart;
    const struct device* hat_container;
};

struct mms_hat_uart_data {
    int slot;
};

static int mms_hat_uart_poll_in(const struct device* dev, unsigned char* p_char) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_poll_in(cfg->parent_uart, p_char);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static void mms_hat_uart_poll_out(const struct device* dev, unsigned char out_char) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;

    if (data->slot < 0) {
        return;
    }

    chainbus_assert_cs(data->slot, true);
    uart_poll_out(cfg->parent_uart, out_char);
    chainbus_assert_cs(data->slot, false);
}

static int mms_hat_uart_err_check(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_err_check(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int mms_hat_uart_configure(const struct device* dev, const struct uart_config* cfg) {
    const struct mms_hat_uart_config* mcfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_configure(mcfg->parent_uart, cfg);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_uart_config_get(const struct device* dev, struct uart_config* cfg) {
    const struct mms_hat_uart_config* mcfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_config_get(mcfg->parent_uart, cfg);
    chainbus_assert_cs(data->slot, false);

    return ret;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#if 0
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int mms_hat_uart_fifo_fill(const struct device* dev, const uint8_t* tx_data, int size) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_fifo_fill(cfg->parent_uart, tx_data, size);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_uart_fifo_read(const struct device* dev, uint8_t* rx_data, const int size) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_fifo_read(cfg->parent_uart, rx_data, size);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static void mms_hat_uart_irq_tx_enable(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;

    if (data->slot < 0) {
        return;
    }

    chainbus_assert_cs(data->slot, true);
    uart_irq_tx_enable(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);
}

static void mms_hat_uart_irq_tx_disable(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;

    if (data->slot < 0) {
        return;
    }

    chainbus_assert_cs(data->slot, true);
    uart_irq_tx_disable(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);
}

static int mms_hat_uart_irq_tx_ready(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_irq_tx_ready(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static void mms_hat_uart_irq_rx_enable(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;

    if (data->slot < 0) {
        return;
    }

    chainbus_assert_cs(data->slot, true);
    uart_irq_rx_enable(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);
}

static void mms_hat_uart_irq_rx_disable(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;

    if (data->slot < 0) {
        return;
    }

    chainbus_assert_cs(data->slot, true);
    uart_irq_rx_disable(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);
}

static int mms_hat_uart_irq_tx_complete(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_irq_tx_complete(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_uart_irq_rx_ready(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_irq_rx_ready(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static void mms_hat_uart_irq_err_enable(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;

    if (data->slot < 0) {
        return;
    }

    chainbus_assert_cs(data->slot, true);
    uart_irq_err_enable(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);
}

static void mms_hat_uart_irq_err_disable(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;

    if (data->slot < 0) {
        return;
    }

    chainbus_assert_cs(data->slot, true);
    uart_irq_err_disable(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);
}

static int mms_hat_uart_irq_is_pending(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_irq_is_pending(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static void mms_hat_uart_irq_update(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;

    if (data->slot < 0) {
        return;
    }

    chainbus_assert_cs(data->slot, true);
    uart_irq_update(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);
}

static void mms_hat_uart_irq_callback_user_data_set(const struct device* dev,
                                                    uart_irq_callback_user_data_t cb,
                                                    void* user_data) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;

    if (data->slot < 0) {
        return;
    }

    chainbus_assert_cs(data->slot, true);
    uart_irq_callback_user_data_set(cfg->parent_uart, cb, user_data);
    chainbus_assert_cs(data->slot, false);

}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_LINE_CTRL
static int mms_hat_uart_line_ctrl_set(const struct device* dev, uint32_t ctrl, uint32_t val) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_line_ctrl_set(cfg->parent_uart, ctrl, val);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_uart_line_ctrl_get(const struct device* dev, uint32_t ctrl, uint32_t* val) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_line_ctrl_get(cfg->parent_uart, ctrl, val);
    chainbus_assert_cs(data->slot, false);

    return ret;
}
#endif /* CONFIG_UART_LINE_CTRL */

#ifdef CONFIG_UART_DRV_CMD
static int mms_hat_uart_drv_cmd(const struct device* dev, uint32_t cmd, uint32_t p) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_drv_cmd(cfg->parent_uart, cmd, p);
    chainbus_assert_cs(data->slot, false);

    return ret;
}
#endif /* CONFIG_UART_DRV_CMD */

#ifdef CONFIG_UART_ASYNC_API
static int mms_hat_uart_callback_set(const struct device* dev, uart_callback_t callback,
                                      void* user_data) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_callback_set(cfg->parent_uart, callback, user_data);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_uart_tx(const struct device* dev, const uint8_t* buf, size_t len,
                            int32_t timeout) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_tx(cfg->parent_uart, buf, len, timeout);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_uart_tx_abort(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_tx_abort(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_uart_rx_enable(const struct device* dev, uint8_t* buf, size_t len,
                                   int32_t timeout) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_rx_enable(cfg->parent_uart, buf, len, timeout);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_uart_rx_buf_rsp(const struct device* dev, uint8_t* buf, size_t len) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_rx_buf_rsp(cfg->parent_uart, buf, len);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_uart_rx_disable(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_rx_disable(cfg->parent_uart);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

#ifdef CONFIG_UART_WIDE_DATA
static int mms_hat_uart_tx_u16(const struct device* dev, const uint16_t* buf, size_t len,
                                int32_t timeout) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_tx_u16(cfg->parent_uart, buf, len, timeout);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_uart_rx_enable_u16(const struct device* dev, uint16_t* buf, size_t len,
                                       int32_t timeout) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_rx_enable_u16(cfg->parent_uart, buf, len, timeout);
    chainbus_assert_cs(data->slot, false);

    return ret;
}

static int mms_hat_uart_rx_buf_rsp_u16(const struct device* dev, uint16_t* buf, size_t len) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);
    ret = uart_rx_buf_rsp_u16(cfg->parent_uart, buf, len);
    chainbus_assert_cs(data->slot, false);

    return ret;
}
#endif /* CONFIG_UART_WIDE_DATA */
#endif /* CONFIG_UART_ASYNC_API */
#endif

static const struct uart_driver_api mms_hat_uart_api = {
    .poll_in = mms_hat_uart_poll_in,
    .poll_out = mms_hat_uart_poll_out,
    .err_check = mms_hat_uart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
    .configure = mms_hat_uart_configure,
    .config_get = mms_hat_uart_config_get,
#endif
#if 0 // Currently, the chainbus architecture does not allow interrupt-driven or async UART operations
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    .fifo_fill = mms_hat_uart_fifo_fill,
    .fifo_read = mms_hat_uart_fifo_read,
    .irq_tx_enable = mms_hat_uart_irq_tx_enable,
    .irq_tx_disable = mms_hat_uart_irq_tx_disable,
    .irq_tx_ready = mms_hat_uart_irq_tx_ready,
    .irq_rx_enable = mms_hat_uart_irq_rx_enable,
    .irq_rx_disable = mms_hat_uart_irq_rx_disable,
    .irq_tx_complete = mms_hat_uart_irq_tx_complete,
    .irq_rx_ready = mms_hat_uart_irq_rx_ready,
    .irq_err_enable = mms_hat_uart_irq_err_enable,
    .irq_err_disable = mms_hat_uart_irq_err_disable,
    .irq_is_pending = mms_hat_uart_irq_is_pending,
    .irq_update = mms_hat_uart_irq_update,
    .irq_callback_set = mms_hat_uart_irq_callback_user_data_set,
#endif
#ifdef CONFIG_UART_LINE_CTRL
    .line_ctrl_set = mms_hat_uart_line_ctrl_set,
    .line_ctrl_get = mms_hat_uart_line_ctrl_get,
#endif
#ifdef CONFIG_UART_DRV_CMD
    .drv_cmd = mms_hat_uart_drv_cmd,
#endif
#ifdef CONFIG_UART_ASYNC_API
    .callback_set = mms_hat_uart_callback_set,
    .tx = mms_hat_uart_tx,
    .tx_abort = mms_hat_uart_tx_abort,
    .rx_enable = mms_hat_uart_rx_enable,
    .rx_buf_rsp = mms_hat_uart_rx_buf_rsp,
    .rx_disable = mms_hat_uart_rx_disable,
#ifdef CONFIG_UART_WIDE_DATA
    .tx_u16 = mms_hat_uart_tx_u16,
    .rx_enable_u16 = mms_hat_uart_rx_enable_u16,
    .rx_buf_rsp_u16 = mms_hat_uart_rx_buf_rsp_u16,
#endif
#endif
#endif
};

static int mms_hat_uart_init(const struct device* dev) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;

    if (!device_is_ready(cfg->parent_uart)) {
        LOG_ERR("Parent physical UART bus not ready");
        return -ENODEV;
    }

    data->slot = mms_hat_get_slot(cfg->hat_container);
    if (data->slot < 0) {
        LOG_ERR("Bridge '%s' cannot find valid slot from parent HAT '%s'", dev->name,
                cfg->hat_container->name);
        return -ENODEV;
    }

    LOG_INF("UART Bridge active on Slot %d -> HW Bus: %s", data->slot, cfg->parent_uart->name);
    return 0;
}

#define MMS_HAT_UART_INIT(inst)                                                                    \
    static const struct mms_hat_uart_config mms_hat_uart_cfg_##inst = {                            \
        .parent_uart = DEVICE_DT_GET(DT_INST_PHANDLE(inst, uart_bus)),                             \
        .hat_container = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                      \
    };                                                                                             \
    static struct mms_hat_uart_data mms_hat_uart_dat_##inst;                                       \
    DEVICE_DT_INST_DEFINE(inst, mms_hat_uart_init, NULL, &mms_hat_uart_dat_##inst,                 \
                          &mms_hat_uart_cfg_##inst, POST_KERNEL,                                   \
                          CONFIG_MMS_HAT_UART_BRIDGE_INIT_PRIORITY, &mms_hat_uart_api);

DT_INST_FOREACH_STATUS_OKAY(MMS_HAT_UART_INIT)
