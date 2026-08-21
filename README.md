# MMS HAT Driver Module

This Zephyr module provides support for the MMS HAT architecture, enabling a modular system where a parent HAT device manages slot allocation via a chainbus,
and child bridge devices (such as I2C and SPI bridges) inherit these slots to facilitate communication.

This driver is meant to be a _blank slate_, only serving as a framework to allow device drivers to attach to the virtualized I2C and SPI buses exposed by the HAT. It does not provide any specific device drivers for the child devices themselves.
As such, it handles **only** the following responsibilities:

- Claiming a slot from the chainbus during initialization and storing this configuration for other child devices of the `chainbus` node.
- Virtualizing the physical parent I2C and SPI buses to allow child devices to attach to them via the HAT interface.

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
            position = <0>;
            label = "Static RTC + SD HAT";

            /* Slot 0 Virtual I2C Bus Bridge */
            slot0_i2c: i2c-bridge {
                compatible = "konar,mms-hat-i2c-bridge";
                i2c-bus = <&i2c2>; /* Physical I2C Controller */
                #address-cells = <1>;
                #size-cells = <0>;

                /* Example: DS3231 RTC + PCA9555 GPIO Expander */
                /* Parent MFD node on the I2C bus */
                ds3231: ds3231@68 {
                    compatible = "maxim,ds3231-mfd";
                    reg = <0x68>;
                    status = "okay";

                    /* Child RTC node that binds to 'maxim_ds3231_rtc' */
                    rtc0: ds3231_rtc {
                        compatible = "maxim,ds3231-rtc";
                        isw-gpios = <&expander0 15 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
                        status = "okay";
                    };
                };
                expander0: gpio@20 {
                    compatible = "nxp,pca9555";
                    reg = <0x20>;
                    gpio-controller;
                    #gpio-cells = <2>;
                    ngpios = <16>;
                    status = "okay";
                };
            };

            /* Slot 0 Virtual SPI Bus Bridge */
            slot0_spi: spi-bridge {
                compatible = "konar,mms-hat-spi-bridge";
                spi-bus = <&chainbus_spi>; /* Physical SPI Controller */
                #address-cells = <1>;
                #size-cells = <0>;
            };
            leds {
                compatible = "gpio-leds";
                hat0_led0: led_0 {
                    label = "LED 0";
                    gpios = <&expander0 0 GPIO_ACTIVE_HIGH>;
                };
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
│   └── mms_hat_spi_bridge.c
├── dts/bindings/mms_hat
│   ├── i2c
│   │   └── konar,mms-hat-i2c-bridge.yaml
│   ├── konar,mms-hat.yaml
│   └── spi
│   └── konar,mms-hat-spi-bridge.yaml
├── include/drivers/mms_hat
│   └── mms_hat_base.h
├── Kconfig
└── zephyr/module.yml

```

### Source Files

- **MMS HAT Container (`mms_hat.c`)**: Acts as the primary controller for the HAT. It claims a slot from the chainbus during initialization and stores this configuration for child devices.
- **I2C Bridge (`mms_hat_i2c_bridge.c`)**: Exposes an I2C controller interface linked to a physical parent I2C bus and a parent MMS HAT container.
- **SPI Bridge (`mms_hat_spi_bridge.c`)**: Exposes an SPI controller interface linked to a physical parent SPI bus and a parent MMS HAT container.

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

## Kconfig Configuration Options

The module provides several Kconfig options to enable or disable components and configure initialization priorities:

- **`CONFIG_MMS_HAT`**: Enables the MMS HAT Generic Container Driver. (Default: `y`, dependency: `CONFIG_MMS_CHAINBUS`)
- **`CONFIG_MMS_HAT_INIT_PRIORITY`**: Initialization priority for HAT containers. Must run after Chainbus init (default: `55`) and before child bridges. (Default: `56`)
- **`CONFIG_MMS_HAT_I2C_BRIDGE`**: Enables the MMS HAT Virtual I2C Bridge Driver. (Default: `y`)

## Runtime Behavior

### Slot Allocation

The MMS HAT driver claims a slot from the chainbus during initialization. If the `position` property is specified in the device tree, it will attempt to claim that specific slot.
If not specified, it will automatically discover and claim the first available slot.

### I2C and SPI Virtualization

The I2C and SPI bridge drivers expose virtualized interfaces that allow child devices to communicate over the physical parent buses.
They wrap the calls to the underlying buses with `chainbus_assert_cs(int slot, bool active)` to manage the chip select lines of the chainbus, ensuring that the HAT is active on the bus at this time.

> **NOTE:** The HAT driver **does not** handle mutexing the chainbus for concurrent access. It is the responsibility of the programmer to ensure that only one HAT is active at a time when accessing the bus.

## See Also

- **[mms_board](https://github.com/maxksiazka/mms_f405_zephyr_board_def)**: Zephyr board definition for the MMS3 F405 platform
- **[mms_chainbus](https://github.com/maxksiazka/mms_chainbus_driver)**: Chainbus driver -- parent bus/MFD for HATs
- **[apps/1-blink](https://github.com/maxksiazka/mms_blink)**: Complete working example (overlay + app)

## License

This software is licensed under **Apache 2.0**.
See [LICENSE](LICENSE) for details.
