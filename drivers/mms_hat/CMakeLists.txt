# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# Base HAT container logic
zephyr_library_sources_ifdef(CONFIG_MMS_HAT mms_hat.c)

# Virtual Bridges (only compiled if their respective Kconfig symbol is 'y')
zephyr_library_sources_ifdef(CONFIG_MMS_HAT_I2C_BRIDGE mms_hat_i2c_bridge.c)
zephyr_library_sources_ifdef(CONFIG_MMS_HAT_SPI_BRIDGE mms_hat_spi_bridge.c)
zephyr_library_sources_ifdef(CONFIG_MMS_HAT_UART_BRIDGE mms_hat_uart_bridge.c)
