# MMS HAT I2C Bridge - Detailed Documentation

## Overview

The MMS HAT I2C Bridge (`mms_hat_i2c_bridge.c`) provides a virtual I2C controller that routes communication through the MMS Chainbus to a physical I2C controller on the baseboard. This allows I2C devices connected to HAT modules to communicate using standard Zephyr I2C APIs.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application                               │
│                  (Zephyr I2C API)                               │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                  MMS HAT I2C Bridge                             │
│  (mms_hat_i2c_bridge.c)                                         │
│  - Implements i2c_driver_api                                    │
│  - Manages slot CS via chainbus_assert_cs()                     │
│  - Delegates to parent I2C controller                           │
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
│                  Physical I2C Controller                        │
│  (e.g., STM32 I2C, &i2c0)                                       │
└─────────────────────────────────────────────────────────────────┘
```

## Device Tree Configuration

### Parent I2C Controller

First, ensure you have a physical I2C controller defined:

```dts
&i2c0 {
    status = "okay";
    clock-frequency = <I2C_BITRATE_FAST>;
    pinctrl-0 = <&i2c0_pins>;
    pinctrl-names = "default";
};
```

### HAT with I2C Bridge

```dts
/ {
    chainbus {
        compatible = "konar,mms-chainbus";
        cs-gpios = <&gpioa 4 GPIO_ACTIVE_LOW>,  /* CS for slot 0 */
                   <&gpioa 5 GPIO_ACTIVE_LOW>;  /* CS for slot 1 */

        my_hat: hat@0 {
            compatible = "konar,mms-hat";
            reg = <0>;
            label = "My I2C HAT";

            /* Virtual I2C Bridge for Slot 0 */
            hat_i2c: i2c-bridge {
                compatible = "konar,mms-hat-i2c-bridge";
                i2c-bus = <&i2c0>;  /* Physical I2C on baseboard */
            };
        };
    };
};
```

### Using the Virtual I2C Bus

Once configured, you can use the virtual I2C bus device for I2C devices connected to the HAT:

```dts
/* Reference the virtual I2C bus in I2C device nodes */
my_i2c_sensor: my_i2c_sensor@68 {
    compatible = "vendor,my-i2c-sensor";
    reg = <0x68>;
    status = "okay";
};
```

## Supported I2C APIs

| Function | Description |
|----------|-------------|
| `configure` | Configure I2C bus parameters (speed, addressing mode) |
| `transfer` | Perform I2C transactions (write, read, or combined) |

### Transfer Function

The `transfer` function is the main I/O operation:

```c
int i2c_transfer(const struct device* dev, struct i2c_msg* msgs, uint8_t num_msgs, uint16_t addr);
```

- `dev`: The virtual I2C bridge device
- `msgs`: Array of I2C messages (can be multiple for combined transactions)
- `num_msgs`: Number of messages in the array
- `addr`: 7-bit I2C target address

Each message describes a single direction transfer:
```c
struct i2c_msg {
    uint8_t* buf;       /* Data buffer */
    size_t len;         /* Length of buffer */
    uint16_t flags;     /* I2C_MSG_READ, I2C_MSG_WRITE, I2C_MSG_RESTART, I2C_MSG_STOP */
};
```

## Usage Example

### Basic I2C Sensor Communication

```c
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(i2c_example, LOG_LEVEL_INF);

/* Virtual I2C bridge device */
const struct device* i2c_dev = DEVICE_DT_GET(DT_NODELABEL(hat_i2c));

/* Target device address (7-bit) */
#define SENSOR_ADDR 0x68

void main(void) {
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C bridge device not ready");
        return;
    }

    int ret;
    uint8_t tx_buf[2];
    uint8_t rx_buf[2];

    /* Example: Write register address, then read 2 bytes */
    struct i2c_msg msgs[2] = {
        {
            .buf = tx_buf,
            .len = 1,
            .flags = I2C_MSG_WRITE,
        },
        {
            .buf = rx_buf,
            .len = 2,
            .flags = I2C_MSG_READ | I2C_MSG_STOP,
        },
    };

    while (1) {
        /* Write register address to read from */
        tx_buf[0] = 0x00;  /* Example register address */

        ret = i2c_transfer(i2c_dev, msgs, 2, SENSOR_ADDR);
        if (ret != 0) {
            LOG_ERR("I2C transfer failed: %d", ret);
        } else {
            LOG_INF("Read value: 0x%02x 0x%02x", rx_buf[0], rx_buf[1]);
        }

        k_sleep(K_MSEC(1000));
    }
}
```

### Simple Write/Read

```c
/* Write single byte to register */
uint8_t write_buf[] = { 0x01, 0x55 };  /* Register 0x01 = 0x55 */
ret = i2c_write(i2c_dev, write_buf, sizeof(write_buf), SENSOR_ADDR);

/* Read single byte from register */
uint8_t reg = 0x01;
uint8_t read_val;
ret = i2c_write_read(i2c_dev, SENSOR_ADDR, &reg, 1, &read_val, 1);
```

### Configure I2C Speed

```c
/* Set I2C bus speed to 400kHz (Fast Mode) */
ret = i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_FAST));

/* Or use standard 100kHz */
ret = i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_STANDARD));
```

## Kconfig Options

| Option | Description | Default |
|--------|-------------|---------|
| `CONFIG_MMS_HAT_I2C_BRIDGE` | Enable I2C bridge driver | `y` |
| `CONFIG_MMS_HAT_I2C_BRIDGE_INIT_PRIORITY` | Init priority (after HAT container) | `60` |

## Implementation Details

### Slot Management

Each I2C bridge instance is bound to a specific HAT slot:

```c
struct mms_hat_i2c_data {
    int slot;  /* Assigned during init via mms_hat_get_slot() */
};
```

During initialization (`mms_hat_i2c_init`):
1. Verifies parent I2C device is ready
2. Retrieves slot number from parent HAT container
3. Stores slot for use in all API calls

### CS Line Management

Every I2C API call wraps the parent I2C operation with CS management:

```c
static int mms_hat_i2c_transfer(const struct device* dev, struct i2c_msg* msgs, uint8_t num_msgs,
                                uint16_t addr) {
    const struct mms_hat_i2c_config* cfg = dev->config;
    struct mms_hat_i2c_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    /* Toggle CS line via chainbus controller */
    chainbus_assert_cs(data->slot, true);   /* Assert CS - activate HAT slot */
    ret = i2c_transfer(cfg->parent_i2c, msgs, num_msgs, addr);  /* Call parent I2C */
    chainbus_assert_cs(data->slot, false);  /* Deassert CS - release HAT slot */

    return ret;
}
```

This ensures the HAT is only active on the chainbus during actual I2C transactions.

### Configure Function

The configure function simply delegates to the parent I2C controller:

```c
static int mms_hat_i2c_configure(const struct device* dev, uint32_t dev_config) {
    const struct mms_hat_i2c_config* cfg = dev->config;
    return i2c_configure(cfg->parent_i2c, dev_config);
}
```

Note: CS is NOT toggled during configure, as this typically only updates internal controller registers.

## Limitations & Known Issues

### 1. No Concurrent Access Protection
The driver does not provide mutex protection. If multiple threads access the same virtual I2C bus, they must implement their own synchronization (e.g., using `k_mutex`).

### 2. CS Overhead
Each I2C transfer toggles the CS line. For multi-message transactions, the CS remains asserted for the entire duration of `i2c_transfer()`.

### 3. No Bus Recovery
The bridge does not implement I2C bus recovery. If the physical bus gets stuck, recovery must be handled at the parent controller level.

### 5. Address Limitations
Only 7-bit addressing is supported (standard I2C). 10-bit addressing would require parent controller support.

## Future Improvements

1. **Bus Recovery**: Add support for I2C bus recovery procedures
2. **10-bit Addressing**: Support for 10-bit I2C addresses if parent controller allows
3. **SMBus/PMBus APIs**: Add support for SMBus-specific operations
4. **DMA Support**: Bridge DMA operations through chainbus for large transfers

## Debugging

Enable debug logging for the I2C bridge:

```c
/* In prj.conf */
CONFIG_LOG=y
CONFIG_I2C_LOG_LEVEL_DBG=y
```

Or at runtime:
```bash
# In Zephyr shell
log enable mms_hat_i2c_bridge
```

## See Also

- [Zephyr I2C API Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/i2c.html)
- [MMS Chainbus Driver](https://github.com/maxksiazka/mms_chainbus_driver)
- [MMS HAT UART Bridge Documentation](uart_bridge.md)
- [MMS HAT SPI Bridge Documentation](spi_bridge.md)
- [MMS HAT Main README](../README.md)
