# MMS HAT UART Bridge - Detailed Documentation

## Overview

The MMS HAT UART Bridge (`mms_hat_uart_bridge.c`) provides a virtual UART controller that routes communication through the MMS Chainbus to a physical UART controller on the baseboard. This allows UART devices connected to HAT modules to communicate using standard Zephyr UART APIs.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application                               │
│                  (Zephyr UART API)                               │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                  MMS HAT UART Bridge                            │
│  (mms_hat_uart_bridge.c)                                        │
│  - Implements uart_driver_api                                   │
│  - Manages slot CS via chainbus_assert_cs()                     │
│  - Delegates to parent UART controller                          │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Chainbus Controller                         │
│  (mms_chainbus)                                                 │
│  - Asserts/deasserts CS for the HAT slot                        │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                  Physical UART Controller                       │
│  (e.g., STM32 UART, &uart0)                                     │
└─────────────────────────────────────────────────────────────────┘
```

## Device Tree Configuration

### Parent UART Controller

First, ensure you have a physical UART controller defined:

```dts
&uart0 {
    status = "okay";
    current-speed = <115200>;
    pinctrl-0 = <&uart0_pins>;
    pinctrl-names = "default";
};
```

### HAT with UART Bridge

```dts
/ {
    chainbus {
        compatible = "konar,mms-chainbus";
        cs-gpios = <&gpioa 4 GPIO_ACTIVE_LOW>,  /* CS for slot 0 */
                   <&gpioa 5 GPIO_ACTIVE_LOW>;  /* CS for slot 1 */

        my_hat: hat@0 {
            compatible = "konar,mms-hat";
            reg = <0>;
            label = "My UART HAT";

            /* Virtual UART Bridge for Slot 0 */
            hat_uart: uart-bridge {
                compatible = "konar,mms-hat-uart-bridge";
                uart-bus = <&uart0>;  /* Physical UART on baseboard */
                current-speed = <115200>;
            };
        };
    };
};
```

### Using the Virtual UART

Once configured, you can use the virtual UART device in your application:

```dts
/* Reference the virtual UART in other nodes */
my_uart_device: my_uart_device {
    compatible = "vendor,my-uart-device";
    uart = <&hat_uart>;  /* References the virtual UART bridge */
    status = "okay";
};
```

## Supported UART APIs

### Polling Mode (Always Available)

| Function | Description |
|----------|-------------|
| `poll_in` | Read a single character (blocking) |
| `poll_out` | Write a single character (blocking) |
| `err_check` | Check for UART errors |

### Runtime Configuration (CONFIG_UART_USE_RUNTIME_CONFIGURE)

| Function | Description |
|----------|-------------|
| `configure` | Configure UART parameters (baud rate, parity, etc.) |
| `config_get` | Get current UART configuration |

### Interrupt-Driven Mode (CONFIG_UART_INTERRUPT_DRIVEN) - **DISABLED**

> **Important:** Interrupt-driven mode is currently **disabled** (`#if 0`) because the chainbus architecture requires the CS line to be asserted for the duration of each UART operation.
    This is incompatible with interrupt callbacks where the CS would need to remain asserted across multiple operations.

The following APIs are implemented but not exposed:

- `fifo_fill` / `fifo_read`
- `irq_tx_enable` / `irq_tx_disable` / `irq_tx_ready`
- `irq_rx_enable` / `irq_rx_disable` / `irq_rx_ready`
- `irq_tx_complete`
- `irq_err_enable` / `irq_err_disable`
- `irq_is_pending` / `irq_update`
- `irq_callback_user_data_set`

### Line Control (CONFIG_UART_LINE_CTRL) - **DISABLED**

Currently disabled due to the same chainbus CS constraints.

- `line_ctrl_set` / `line_ctrl_get`

### Driver Commands (CONFIG_UART_DRV_CMD) - **DISABLED**

- `drv_cmd`

### Async API (CONFIG_UART_ASYNC_API) - **DISABLED**

The async API is implemented but disabled due to CS management constraints:

- `callback_set`
- `tx` / `tx_abort`
- `rx_enable` / `rx_buf_rsp` / `rx_disable`
- Wide data variants (CONFIG_UART_WIDE_DATA): `tx_u16`, `rx_enable_u16`, `rx_buf_rsp_u16`

## Usage Example

### Basic Polling Usage

```c
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(hat_uart));

void main(void) {
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return;
    }

    uint8_t tx_buf[] = "Hello from MMS HAT UART!\r\n";
    uint8_t rx_buf;

    /* Transmit using polling */
    for (size_t i = 0; i < sizeof(tx_buf); i++) {
        uart_poll_out(uart_dev, tx_buf[i]);
    }

    /* Receive using polling */
    while (1) {
        if (uart_poll_in(uart_dev, &rx_buf) == 0) {
            uart_poll_out(uart_dev, rx_buf);  /* Echo back */
        }
        k_msleep(10);
    }
}
```

### Using with UART Shell / Console

I have no clue why would you want to do this, but if you want to use the virtual UART as the Zephyr shell or console, you can set it in the `chosen` node:

```dts
/ {
    chosen {
        zephyr,console = &hat_uart;
        zephyr,shell-uart = &hat_uart;
    };
};
```

## Kconfig Options

| Option | Description | Default |
|--------|-------------|---------|
| `CONFIG_MMS_HAT_UART_BRIDGE` | Enable UART bridge driver | `y` |
| `CONFIG_MMS_HAT_UART_BRIDGE_INIT_PRIORITY` | Init priority (after HAT container) | `60` |

## Implementation Details

### Slot Management

Each UART bridge instance is bound to a specific HAT slot:

```c
struct mms_hat_uart_data {
    int slot;  /* Assigned during init via mms_hat_get_slot() */
};
```

During initialization (`mms_hat_uart_init`):
1. Verifies parent UART device is ready
2. Retrieves slot number from parent HAT container
3. Stores slot for use in all API calls

### CS Line Management

Every UART API call wraps the parent UART operation with CS management:

```c
static int mms_hat_uart_poll_in(const struct device* dev, unsigned char* p_char) {
    const struct mms_hat_uart_config* cfg = dev->config;
    struct mms_hat_uart_data* data = dev->data;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);   /* Assert CS - activate HAT slot */
    ret = uart_poll_in(cfg->parent_uart, p_char);  /* Call parent UART */
    chainbus_assert_cs(data->slot, false);  /* Deassert CS - release HAT slot */

    return ret;
}
```

This ensures the HAT is only active on the chainbus during actual UART transactions.

## Limitations & Known Issues

### 1. Polling Mode Only
Currently only polling APIs are enabled. Interrupt-driven and async modes are implemented but disabled due to chainbus CS constraints.

### 2. No Concurrent Access Protection
The driver does not provide mutex protection. If multiple threads access the same virtual UART, they must implement their own synchronization.

### 3. CS Overhead
Each character read/write in polling mode toggles the CS line, which adds latency. For high-throughput applications, consider using a DMA-based approach on the physical UART (not currently supported through the bridge).

### 4. Baud Rate Configuration
Runtime baud rate changes work through `uart_configure()`, but the CS is toggled during the configure call.

## Future Improvements

1. **Interrupt Support**: Requires chainbus architecture changes to support CS hold during ISR
2. **Async API**: Would need chainbus support for async CS management
3. **DMA Support**: Bridge DMA operations through chainbus
4. **Flow Control**: Hardware flow control (RTS/CTS) passthrough

## Debugging

Enable debug logging for the UART bridge:

```c
/* In prj.conf */
CONFIG_LOG=y
CONFIG_UART_LOG_LEVEL_DBG=y
```

Or at runtime:
```bash
# In Zephyr shell
log enable mms_hat_uart_bridge
```

## See Also

- [Zephyr UART API Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/uart.html)
- [MMS Chainbus Driver](https://github.com/maxksiazka/mms_chainbus_driver)
- [MMS HAT Main README](../README.md)
