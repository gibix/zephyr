/*
 * Copyright (c) 2026 Arduino SA
 * SPDX-License-Identifier: Apache-2.0
 *
 * Read-only driver for the calibration/OTP fuse row (FUSES_CALOTP) of
 * Microchip PIC32CK SG/GC SoCs. The row is provisioned during manufacturing,
 * so programming is not supported.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT microchip_pic32ck_fuses

struct otp_mchp_fuses_config {
	mem_addr_t base;
	size_t size;
};

static int otp_mchp_fuses_read(const struct device *dev, off_t offset, void *buf, size_t len)
{
	const struct otp_mchp_fuses_config *config = dev->config;
	size_t start = (size_t)offset;
	uint8_t *dst = buf;
	size_t end;

	if (offset < 0 || size_add_overflow(start, len, &end) || end > config->size) {
		return -EINVAL;
	}

	/*
	 * The row only answers 32-bit accesses, so walk the aligned words
	 * covering the range instead of using memcpy().
	 */
	while (start < end) {
		size_t word_start = ROUND_DOWN(start, sizeof(uint32_t));
		size_t skip = start - word_start;
		size_t chunk = MIN(end - start, sizeof(uint32_t) - skip);
		uint8_t word[sizeof(uint32_t)];

		sys_put_le32(sys_read32(config->base + word_start), word);
		memcpy(dst, &word[skip], chunk);

		start += chunk;
		dst += chunk;
	}

	return 0;
}

static DEVICE_API(otp, otp_mchp_fuses_api) = {
	.read = otp_mchp_fuses_read,
};

#define OTP_MCHP_FUSES_INIT(inst)                                                                  \
	static const struct otp_mchp_fuses_config otp_mchp_fuses_config_##inst = {                  \
		.base = (mem_addr_t)DT_INST_REG_ADDR(inst),                                        \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, &otp_mchp_fuses_config_##inst,                \
			      PRE_KERNEL_1, CONFIG_OTP_INIT_PRIORITY, &otp_mchp_fuses_api);

DT_INST_FOREACH_STATUS_OKAY(OTP_MCHP_FUSES_INIT)
