# MMS HAT SPI Bridge - Detailed Documentation

## Overview

The MMS HAT SPI Bridge (`mms_hat_spi_bridge.c`) provides a virtual SPI controller that routes communication through the MMS Chainbus to a physical SPI controller on the baseboard. This allows SPI devices connected to HAT modules to communicate using standard Zephyr SPI APIs.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application                               │
│                  (Zephyr SPI API)                               │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                  MMS HAT SPI Bridge                             │
│  (mms_hat_spi_bridge.c)                                         │
│  - Implements spi_driver_api                                    │
│  - Manages slot CS via chainbus_assert_cs()                     │
│  - Delegates to parent SPI controller                           │
└─────────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Chainbus Controller                         │
│  (mms_chainbus)                                                 │
│  - Asserts/deasserts CS for the HAT slot                        │
└────────────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                  Physical SPI Controller                        │
│  (e.g., STM32 SPI, &spi3)                                       │
└─────────────────────────────────────────────────────────────────┘
```

## Device Tree Configuration

### Parent SPI Controller

First, ensure you have a physical SPI controller defined:

```dts
&spi3 {
    status = "okay";
    pinctrl-0 = <&spi3_pins>;
    pinctrl-names = "default";
    cs-gpios = <&gpiob 12 GPIO_ACTIVE_LOW>;  /* Default CS if needed */
};
```

### HAT with SPI Bridge

```dts
/ {
    chainbus {
        compatible = "konar,mms-chainbus";
        cs-gpios = <&gpioa 4 GPIO_ACTIVE_LOW>,  /* CS for slot 0 */
                   <&gpioa 5 GPIO_ACTIVE_LOW>;  /* CS for slot 1 */

        my_hat: hat@0 {
            compatible = "konar,mms-hat";
            reg = <0>;
            label = "My SPI HAT";

            /* Virtual SPI Bridge for Slot 0 */
            hat_spi: spi-bridge {
                compatible = "konar,mms-hat-spi-bridge";
                spi-bus = <&spi3>;  /* Physical SPI on baseboard */
                /* Optional: cs-gpios for pin-driven CS on HAT */
                /* cs-gpios = <&gpiob 5 GPIO_ACTIVE_LOW>; */
            };
        };
    };
};
```

### Using the Virtual SPI Bus

Once configured, you can use the virtual SPI bus device for SPI devices connected to the HAT:

```dts
/* Reference the virtual SPI bus in SPI device nodes */
my_spi_device: my_spi_device@0 {
    compatible = "vendor,my-spi-device";
    reg = <0>;  /* Chip select index on the virtual SPI bus */
    spi-max-frequency = <4000000>;
    status = "okay";
};
```

## Supported SPI APIs

| Function | Description |
|----------|-------------|
| `transceive` | Full-duplex SPI transfer (simultaneous TX/RX) |
| `release` | Release SPI bus/CS (for multi-device sharing) |

### Transceive Function

The `transceive` function is the main I/O operation:

```c
int spi_transceive(const struct device* dev, const struct spi_config* config,
                   const struct spi_buf_set* tx_bufs,
                   const struct spi_buf_set* rx_bufs);
```

- `dev`: The virtual SPI bridge device
- `config`: SPI configuration (mode, frequency, word size, CS)
- `tx_bufs`: Transmit buffers (or NULL for RX-only)
- `rx_bufs`: Receive buffers (or NULL for TX-only)

### SPI Configuration

The `spi_config` structure defines the SPI transaction parameters:

```c
struct spi_config {
    uint16_t frequency;     /* SPI clock frequency in Hz */
    uint16_t operation;     /* SPI mode flags */
    uint8_t slave;          /* Slave/chip select number */
    uint8_t cs;             /* Chip select control */
};
```

Common operation flags:
- `SPI_OP_MODE_MASTER` - Master mode (required)
- `SPI_OP_MODE_SLAVE` - Slave mode
- `SPI_MODE_CPOL` - Clock polarity (CPOL=1)
- `SPI_MODE_CPHA` - Clock phase (CPHA=1)
- `SPI_MODE_LOOP` - Loopback mode
- `SPI_TRANSFER_MSB` - MSB first (default)
- `SPI_TRANSFER_LSB` - LSB first
- `SPI_WORD_SET(n)` - Word size in bits (default 8)

### Buffer Sets

SPI uses buffer sets for scatter-gather I/O:

```c
struct spi_buf {
    void* buf;      /* Data buffer */
    size_t len;     /* Buffer length */
};

struct spi_buf_set {
    struct spi_buf* buffers;  /* Array of buffers */
    size_t count;             /* Number of buffers */
};
```

## Usage Example

### Basic SPI Communication

```c
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_example, LOG_LEVEL_INF);

/* Virtual SPI bridge device */
const struct device* spi_dev = DEVICE_DT_GET(DT_NODELABEL(hat_spi));

/* SPI configuration for our device */
struct spi_config spi_cfg = {
    .frequency = 4000000,      /* 4 MHz */
    .operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER,
    .slave = 0,                /* CS index 0 */
};

void main(void) {
    if (!device_is_ready(spi_dev)) {
        LOG_ERR("SPI bridge device not ready");
        return;
    }

    int ret;
    uint8_t tx_buf[] = { 0x9F };  /* Example: JEDEC ID command */
    uint8_t rx_buf[3];

    struct spi_buf tx_spi_buf = { .buf = tx_buf, .len = sizeof(tx_buf) };
    struct spi_buf rx_spi_buf = { .buf = rx_buf, .len = sizeof(rx_buf) };

    struct spi_buf_set tx_bufs = { .buffers = &tx_spi_buf, .count = 1 };
    struct spi_buf_set rx_bufs = { .buffers = &rx_spi_buf, .count = 1 };

    while (1) {
        ret = spi_transceive(spi_dev, &spi_cfg, &tx_bufs, &rx_bufs);
        if (ret != 0) {
            LOG_ERR("SPI transceive failed: %d", ret);
        } else {
            LOG_INF("JEDEC ID: 0x%02x 0x%02x 0x%02x", rx_buf[0], rx_buf[1], rx_buf[2]);
        }

        k_sleep(K_MSEC(1000));
    }
}
```

### Write-Only Transfer

```c
/* Write data without reading */
uint8_t tx_data[] = { 0x02, 0x00, 0x55 };  /* Write 0x55 to address 0x0002 */
struct spi_buf tx_spi_buf = { .buf = tx_data, .len = sizeof(tx_data) };
struct spi_buf_set tx_bufs = { .buffers = &tx_spi_buf, .count = 1 };

ret = spi_transceive(spi_dev, &spi_cfg, &tx_bufs, NULL);
```

### Read-Only Transfer

```c
/* Read data without writing (send dummy bytes) */
uint8_t tx_dummy[] = { 0x03, 0x00, 0x00 };  /* Read command + address */
uint8_t rx_data[2];

struct spi_buf tx_spi_buf = { .buf = tx_dummy, .len = sizeof(tx_dummy) };
struct spi_buf rx_spi_buf = { .buf = rx_data, .len = sizeof(rx_data) };

struct spi_buf_set tx_bufs = { .buffers = &tx_spi_buf, .count = 1 };
struct spi_buf_set rx_bufs = { .buffers = &rx_spi_buf, .count = 1 };

ret = spi_transceive(spi_dev, &spi_cfg, &tx_bufs, &rx_bufs);
```

### Multiple Buffers (Scatter-Gather)

```c
/* Complex transaction with multiple buffers */
uint8_t cmd = 0x01;
uint8_t addr[2] = { 0x00, 0x10 };
uint8_t tx_payload[4] = { 0x11, 0x22, 0x33, 0x44 };
uint8_t rx_result[4];

struct spi_buf tx_buffers[] = {
    { .buf = &cmd, .len = 1 },
    { .buf = addr, .len = 2 },
    { .buf = tx_payload, .len = 4 },
};

struct spi_buf rx_buffers[] = {
    { .buf = NULL, .len = 3 },  /* Skip command + address echo */
    { .buf = rx_result, .len = 4 },
};

struct spi_buf_set tx_bufs = { .buffers = tx_buffers, .count = 3 };
struct spi_buf_set rx_bufs = { .buffers = rx_buffers, .count = 2 };

ret = spi_transceive(spi_dev, &spi_cfg, &tx_bufs, &rx_bufs);
```

### Using SPI with Device Tree Device

```c
/* Get SPI device from device tree */
const struct device* spi_dev = DEVICE_DT_GET(DT_NODELABEL(my_spi_device));

/* Use spi_dt.h helper APIs */
#include <zephyr/drivers/spi_dt.h>

struct spi_dt_spec spi_spec = SPI_DT_SPEC_GET(DT_NODELABEL(my_spi_device),
                                               SPI_WORD_SET(8) | SPI_OP_MODE_MASTER,
                                               0);

/* Transceive using the spec (automatically handles config) */
ret = spi_transceive_dt(&spi_spec, &tx_bufs, &rx_bufs);

/* Or use spi_write_dt / spi_read_dt for simple cases */
ret = spi_write_dt(&spi_spec, &tx_bufs);
ret = spi_read_dt(&spi_spec, &rx_bufs);
```

## Kconfig Options

| Option | Description | Default |
|--------|-------------|---------|
| `CONFIG_MMS_HAT_SPI_BRIDGE` | Enable SPI bridge driver | `y` |
| `CONFIG_MMS_HAT_SPI_BRIDGE_INIT_PRIORITY` | Init priority (after HAT container) | `60` |

## Implementation Details

### Slot Management

Each SPI bridge instance is bound to a specific HAT slot:

```c
struct mms_hat_spi_data {
    int slot;  /* Assigned during init via mms_hat_get_slot() */
};
```

During initialization (`mms_hat_spi_init`):
1. Verifies parent SPI device is ready
2. Retrieves slot number from parent HAT container
3. Stores slot for use in all API calls

### CS Line Management

Every SPI `transceive` call wraps the parent SPI operation with CS management:

```c
static int mms_hat_spi_transceive(const struct device* dev, const struct spi_config* config,
                                  const struct spi_buf_set* tx_bufs,
                                  const struct spi_buf_set* rx_bufs) {
    const struct mms_hat_spi_config* cfg = dev->config;
    struct mms_hat_spi_data* data = dev->data;
    int ret;

    if (data->slot < 0) {
        return -ENODEV;
    }

    chainbus_assert_cs(data->slot, true);   /* Assert CS - activate HAT slot */
    ret = spi_transceive(cfg->parent_spi, config, tx_bufs, rx_bufs);  /* Call parent SPI */
    chainbus_assert_cs(data->slot, false);  /* Deassert CS - release HAT slot */

    return ret;
}
```

This ensures the HAT is only active on the chainbus during actual SPI transactions.

### Release Function

The `release` function delegates to the parent SPI controller for releasing the bus:

```c
static int mms_hat_spi_release(const struct device* dev, const struct spi_config* config) {
    const struct mms_hat_spi_config* cfg = dev->config;
    return spi_release(cfg->parent_spi, config);
}
```

This is used when multiple devices share the same SPI bus and need explicit CS management.

## Device Tree Binding Properties

| Property | Type | Required | Description |
|----------|------|----------|-------------|
| `spi-bus` | phandle | Yes | Physical parent SPI controller (e.g., `&spi3`) |
| `cs-gpios` | phandle-array | No | Chip select GPIO line for this slot (if pin-driven) |

## Limitations & Known Issues

### 1. No Concurrent Access Protection
The driver does not provide mutex protection. If multiple threads access the same virtual SPI bus, they must implement their own synchronization (e.g., using `k_mutex` or the SPI `release` API).

### 2. CS Overhead
Each SPI transceive toggles the chainbus CS line. The HAT slot CS is asserted for the entire duration of the SPI transaction.

### 3. No Asynchronous API
Currently only synchronous `transceive` is supported. Zephyr's async SPI API (`spi_transceive_async`) is not implemented.

### 4. Limited CS Per Slot
The chainbus itself has one CS line. It is recommended to use an I2C GPIO expander or similar if multiple CS lines are needed for devices on the same HAT.

### 5. Fixed SPI Mode
The bridge passes through all SPI configuration to the parent controller. Ensure the parent controller supports the requested mode/frequency.

## Future Improvements

1. **Async API**: Add support for `spi_transceive_async` with callback
2. **Multiple CS Support**: Allow multiple CS lines per virtual SPI bus
3. **DMA Support**: Bridge DMA operations through chainbus for large transfers
4. **SPI RTIO**: Support for Zephyr's RTIO-based SPI API

## Debugging

Enable debug logging for the SPI bridge:

```c
/* In prj.conf */
CONFIG_LOG=y
CONFIG_SPI_LOG_LEVEL_DBG=y
```

Or at runtime:
```bash
# In Zephyr shell
log enable mms_hat_spi_bridge
```

## See Also

- [Zephyr SPI API Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/spi.html)
- [MMS Chainbus Driver](https://github.com/maxksiazka/mms_chainbus_driver)
- [MMS HAT UART Bridge Documentation](uart_bridge.md)
- [MMS HAT I2C Bridge Documentation](i2c_bridge.md)
- [MMS HAT Main README](../README.md)
