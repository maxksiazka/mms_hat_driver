#ifndef ZEPHYR_INCLUDE_DRIVERS_MMS_HAT_H_
#define ZEPHYR_INCLUDE_DRIVERS_MMS_HAT_H_
/* SPDX-License-Identifier: Apache-2.0 */

#include <zephyr/device.h>

struct mms_hat_data {
    int slot;
};

/**
 * @brief Get the physical Chainbus slot index claimed by this HAT parent container.
 */
static inline int mms_hat_get_slot(const struct device* hat_dev) {
    if (!hat_dev || !hat_dev->data) {
        return -EINVAL;
    }
    struct mms_hat_data* data = hat_dev->data;
    return data->slot;
}

#endif /* ZEPHYR_INCLUDE_DRIVERS_MMS_HAT_H_ */
