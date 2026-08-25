# MMS HAT Driver Module

This Zephyr module provides support for the MMS HAT architecture, enabling a modular system where a parent HAT device manages slot allocation via a chainbus,
and child bridge devices (such as I2C, SPI, and UART bridges) inherit these slots to facilitate communication.

This driver is meant to be a _blank slate_, only serving as a framework to allow device drivers to attach to the virtualized I2C, SPI, and UART buses exposed by the HAT. It does not provide any specific device drivers for the child devices themselves.
As such, it handles **only** the following responsibilities:

- Claiming a slot from the chainbus during initialization and storing this configuration for other child devices of the `chainbus` node.
- Virtualizing the physical parent I2C, SPI, and UART buses to allow child devices to attach to them via the HAT interface.

## Example Usage

### Device Tree Overlay Example

See [mms_blinky](https://github.com/maxksiazka/mms_blink) for a more complete example of how to use this module in a Zephyr application.

```dts
/ {
    chainbus {
        compatible = "konar,mms-chainbus";
        cs-gpios = /* chip select GPIO phandle array for control of the chainbus */;
        hat_rtc_sd: hat@0 {
            compatible = "konar,mms-hat";
            reg = <0>;
            label = "Static RTC + SD HAT";

            /* Slot 0 Virtual I2C Bus Bridge */
            slot0_i2c: i2c-bridge {
                compatible = "konar,mms-hat-i2c-bridge";
                i2c-bus = <&chainbus_i2c>; /* Physical I2C Controller */
                #address-cells = <1>;
                #size-cells = <0>;

                /* Example child device on the virtual I2C bus */
            };

            /* Slot 0 Virtual SPI Bus Bridge */
            slot0_spi: spi-bridge {
                compatible = "konar,mms-hat-spi-bridge";
                spi-bus = <&chainbus_spi>; /* Physical SPI Controller */
                #address-cells = <1>;
                #size-cells = <0>;
                /* Example child device on the virtual SPI bus */
            };

            /* Slot 0 Virtual UART Bus Bridge */
            slot0_uart: uart-bridge {
                compatible = "konar,mms-hat-uart-bridge";
                uart-bus = <&chainbus_uart>; /* Physical UART Controller */
                current-speed = <115200>;
            };
        };
    };
};
```

## Architecture Overview

### Module Structure

```

modules/mms_hat
├── CMakeLists.txt
├── drivers
│   ├── CMakeLists.txt
│   └── mms_hat
│   ├── CMakeLists.txt
│   ├── mms_hat.c
│   ├── mms_hat_i2c_bridge.c
│   ├── mms_hat_spi_bridge.c
│   └── mms_hat_uart_bridge.c
├── dts/bindings/mms_hat
│   ├── i2c
│   │   └── konar,mms-hat-i2c-bridge.yaml
│   ├── konar,mms-hat.yaml
│   ├── spi
│   │   └── konar,mms-hat-spi-bridge.yaml
│   └── uart
│       └── konar,mms-hat-uart-bridge.yaml
├── include/drivers/mms_hat
│   └── mms_hat_base.h
├── Kconfig
└── zephyr/module.yml

```

### Source Files

- **MMS HAT Container (`mms_hat.c`)**: Acts as the primary controller for the HAT. It claims a slot from the chainbus during initialization and stores this configuration for child devices.
- **I2C Bridge (`mms_hat_i2c_bridge.c`)**: Exposes an I2C controller interface linked to a physical parent I2C bus and a parent MMS HAT container.
- **SPI Bridge (`mms_hat_spi_bridge.c`)**: Exposes an SPI controller interface linked to a physical parent SPI bus and a parent MMS HAT container.
- **UART Bridge (`mms_hat_uart_bridge.c`)**: Exposes a UART controller interface linked to a physical parent UART bus and a parent MMS HAT container. Supports both polling and interrupt-driven modes (when enabled via Kconfig), with line control, async API, and wide data support.

> **NOTE:** The UART bridge currently operates in **polling mode only**. The interrupt-driven and async APIs are implemented in the code but disabled (`#if 0`) because the chainbus architecture does not currently support interrupt-driven or async UART operations across slot boundaries. The chainbus CS (chip select) lines must be asserted for the duration of each UART operation, which is incompatible with the asynchronous callback model where the CS would need to remain asserted across multiple operations.

---

## Device Tree (DT) Bindings Documentation

### 1. MMS HAT Container

- **Binding File:** `dts/bindings/mms_hat/konar,mms-hat.yaml`
- **Description:** Abstract base schema for all MMS3 HAT expansion modules.
- **Properties:**
  - `compatible`: Must be set to `"konar,mms-hat"`.
  - `reg`: Optional array property.
  - `position`: Optional integer specifying explicit physical slot index (0..7). If omitted, slot index is resolved at boot via runtime auto-discovery.

### 2. MMS HAT I2C Bridge

- **Binding File:** `dts/bindings/mms_hat/i2c/konar,mms-hat-i2c-bridge.yaml`
- **Description:** MMS3 Slot I2C Controller Bridge.
- **Properties:**
  - `compatible`: Must be set to `"konar,mms-hat-i2c-bridge"`.
  - `i2c-bus`: Phandle to the physical parent I2C controller.

### 3. MMS HAT SPI Bridge

- **Binding File:** `dts/bindings/mms_hat/spi/konar,mms-hat-spi-bridge.yaml`
- **Description:** MMS3 Virtual SPI Slot Bridge Controller.
- **Properties:**
  - `compatible`: Must be set to `"konar,mms-hat-spi-bridge"`.
  - `spi-bus`: Phandle to the physical parent SPI controller (e.g. `&spi3`).
  - `cs-gpios`: Optional phandle-array for chip select GPIO lines.

### 4. MMS HAT UART Bridge

- **Binding File:** `dts/bindings/mms_hat/uart/konar,mms-hat-uart-bridge.yaml`
- **Description:** MMS3 Slot UART Controller Bridge.
- **Properties:**
  - `compatible`: Must be set to `"konar,mms-hat-uart-bridge"`.
  - `uart-bus`: Phandle to the physical parent UART controller (e.g. `&uart0`).

## Kconfig Configuration Options

The module provides several Kconfig options to enable or disable components and configure initialization priorities:

- **`CONFIG_MMS_HAT`**: Enables the MMS HAT Generic Container Driver. (Default: `y`, dependency: `CONFIG_MMS_CHAINBUS`)
- **`CONFIG_MMS_HAT_INIT_PRIORITY`**: Initialization priority for HAT containers. Must run after Chainbus init (default: `55`) and before child bridges. (Default: `56`)
- **`CONFIG_MMS_HAT_I2C_BRIDGE`**: Enables the MMS HAT Virtual I2C Bridge Driver. (Default: `y`)
- **`CONFIG_MMS_HAT_I2C_BRIDGE_INIT_PRIORITY`**: Initialization priority for HAT I2C bridges. Must run AFTER HAT container init (50). (Default: `60`)
- **`CONFIG_MMS_HAT_SPI_BRIDGE`**: Enables the MMS HAT Virtual SPI Bridge Driver. (Default: `y`)
- **`CONFIG_MMS_HAT_SPI_BRIDGE_INIT_PRIORITY`**: Initialization priority for HAT SPI bridges. Must run AFTER HAT container init (50). (Default: `60`)
- **`CONFIG_MMS_HAT_UART_BRIDGE`**: Enables the MMS HAT Virtual UART Bridge Driver. (Default: `y`)
- **`CONFIG_MMS_HAT_UART_BRIDGE_INIT_PRIORITY`**: Initialization priority for HAT UART bridges. Must run AFTER HAT container init (50). (Default: `60`)

> **NOTE:** It is imperative that any on-bus device drivers that are children of the HAT bridges are initialized **after** the HAT bridge drivers themselves. 
    This is typically achieved by setting their `init_priority` to a value greater-or-equal than the bridge's `init_priority` (default: 60).

## Runtime Behavior

### Slot Allocation

The MMS HAT driver claims a slot from the chainbus during initialization. If the `position` property is specified in the device tree, it will attempt to claim that specific slot.
If not specified, it will automatically discover and claim the first available slot.

### I2C, SPI, and UART Virtualization

The I2C, SPI, and UART bridge drivers expose virtualized interfaces that allow child devices to communicate over the physical parent buses.
They wrap the calls to the underlying buses with `chainbus_assert_cs(int slot, bool active)` to manage the chip select lines of the chainbus, ensuring that the HAT is active on the bus at this time.

> **NOTE:** The HAT driver **does not** handle mutexing the chainbus for concurrent access. It is the responsibility of the programmer to ensure that only one HAT is active at a time when accessing the bus.

> **NOTE on UART:** The UART bridge currently operates in **polling mode only**. The interrupt-driven and async APIs are implemented in the code but disabled (`#if 0`) because the chainbus architecture does not currently support interrupt-driven or async UART operations across slot boundaries. The chainbus CS (chip select) lines must be asserted for the duration of each UART operation, which is incompatible with the asynchronous callback model where the CS would need to remain asserted across multiple operations.

## See Also

- **[mms_board](https://github.com/maxksiazka/mms_f405_zephyr_board_def)**: Zephyr board definition for the MMS3 F405 platform
- **[mms_chainbus](https://github.com/maxksiazka/mms_chainbus_driver)**: Chainbus driver -- parent bus/MFD for HATs
- **[apps/1-blink](https://github.com/maxksiazka/mms_blink)**: Complete working example (overlay + app)
- **[UART Bridge Detailed Documentation](docs/uart_bridge.md)**: In-depth guide for the UART bridge driver

## License

This software is licensed under **Apache 2.0**.
See [LICENSE](LICENSE) for details.
