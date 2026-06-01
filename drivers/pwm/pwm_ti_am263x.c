/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI AM263x / AM263Px ePWM driver (basic period + duty for LED/PWM output).
 *
 * Supports ePWMxA (channel 0) and ePWMxB (channel 1), up-count mode, with
 * normal or inverted output polarity. Sufficient for LED brightness; motor
 * control (deadband, sync, trip-zone, HRPWM) is out of scope here.
 *
 * IMPORTANT — this is the "etpwm" type-5 IP, NOT the legacy C2000 type-0 map.
 * Register offsets, the 32-bit CMPA/CMPB layout, and the time-base-clock
 * enable below are all taken from the MCU+ SDK AM263Px CSL headers (SDK
 * 26.00.00.06):
 *   - drivers/epwm/v1/cslr_etpwm.h           (register offsets + bit fields)
 *   - hw_include/am263px/cslr_controlss_ctrl.h (EPWM_CLKSYNC, KICK lock)
 *   - drivers/soc/am263px/soc.c  SOC_setEpwmTbClk()/SOC_controlModuleUnlockMMR()
 * Do not "simplify" these back to 0x0A/0x0E/0x16 — those are the type-0 map and
 * are wrong for this SoC.
 */

#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/sys_io.h>

#define DT_DRV_COMPAT ti_am263x_epwm

/* ePWM (etpwm type-5) register offsets — cslr_etpwm.h. */
#define EPWM_TBCTL    0x00U   /* 16-bit */
#define EPWM_TBCTR    0x08U   /* 16-bit */
#define EPWM_AQCTLA   0x80U   /* 16-bit */
#define EPWM_AQCTLB   0x84U   /* 16-bit */
#define EPWM_TBPRD    0xC6U   /* 16-bit */
#define EPWM_CMPA     0xD4U   /* 32-bit: [31:16]=CMPA, [15:0]=CMPAHR */
#define EPWM_CMPB     0xD8U   /* 32-bit: [31:16]=CMPB, [15:0]=CMPBHR */

/* TBCTL: CTRMODE[1:0] (0=up), FREE_SOFT[15:14] (2=free-run on emu halt). */
#define TBCTL_CTRMODE_UP   0x0U
#define TBCTL_FREE_SOFT    (0x2U << 14)

/*
 * Action-Qualifier action encoding (TRM SPRUJ55 §7.5.6.6, AQCTLx fields):
 *   0=disabled, 1=clear (force low), 2=set (force high), 3=toggle.
 * Field shifts (cslr_etpwm.h): ZRO[1:0], CAU[5:4] (chan A), CBU[9:8] (chan B).
 */
#define AQ_DISABLE  0x0U
#define AQ_CLEAR    0x1U
#define AQ_SET      0x2U

#define AQCTL_ZRO_SHIFT  0U
#define AQCTL_CAU_SHIFT  4U
#define AQCTL_CBU_SHIFT  8U

/* CONTROLSS_CTRL — time-base-clock enable (cslr_controlss_ctrl.h). */
#define CONTROLSS_CTRL_BASE   0x502F0000U
#define CTRL_EPWM_CLKSYNC     0x010U   /* bit per ePWM instance */
#define CTRL_LOCK0_KICK0      0x1008U
#define CTRL_LOCK0_KICK1      0x100CU
#define CTRL_KICK0_UNLOCK     0x01234567U  /* soc.h KICK0_UNLOCK_VAL */
#define CTRL_KICK1_UNLOCK     0x0FEDCBA8U  /* soc.h KICK1_UNLOCK_VAL */

/* ePWM instance bases are 0x1000 apart starting at 0x50000000 (CSL). */
#define EPWM_INSTANCE_BASE    0x50000000U
#define EPWM_INSTANCE_STRIDE  0x1000U

struct pwm_ti_am263x_config {
	DEVICE_MMIO_ROM;
	const struct pinctrl_dev_config *pcfg;
	uint8_t instance;   /* 0..31, for the EPWM_CLKSYNC bit */
};

struct pwm_ti_am263x_data {
	DEVICE_MMIO_RAM;
};

static inline uintptr_t pwm_ti_base(const struct device *dev)
{
	return DEVICE_MMIO_GET(dev);
}

/* Enable this instance's time-base clock in CONTROLSS_CTRL (partition is KICK
 * locked). Mirrors SOC_setEpwmTbClk() + SOC_controlModuleUnlockMMR().
 */
static void pwm_ti_enable_tbclk(uint8_t instance)
{
	sys_write32(CTRL_KICK0_UNLOCK, CONTROLSS_CTRL_BASE + CTRL_LOCK0_KICK0);
	sys_write32(CTRL_KICK1_UNLOCK, CONTROLSS_CTRL_BASE + CTRL_LOCK0_KICK1);

	uint32_t v = sys_read32(CONTROLSS_CTRL_BASE + CTRL_EPWM_CLKSYNC);

	v |= (1U << instance);
	sys_write32(v, CONTROLSS_CTRL_BASE + CTRL_EPWM_CLKSYNC);

	/* Re-lock the partition. */
	sys_write32(0U, CONTROLSS_CTRL_BASE + CTRL_LOCK0_KICK0);
}

static int pwm_ti_am263x_init(const struct device *dev)
{
	const struct pwm_ti_am263x_config *cfg = dev->config;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	uintptr_t base = pwm_ti_base(dev);

	/* Up-count, free-run on emulation halt, no time-base prescale. */
	sys_write16(TBCTL_CTRMODE_UP | TBCTL_FREE_SOFT, base + EPWM_TBCTL);
	sys_write16(0, base + EPWM_TBCTR);
	sys_write16(0, base + EPWM_TBPRD);
	sys_write32(0, base + EPWM_CMPA);
	sys_write32(0, base + EPWM_CMPB);

	/* Start this instance's time-base clock (gated from reset). */
	pwm_ti_enable_tbclk(cfg->instance);

	return 0;
}

static int pwm_ti_am263x_set_cycles(const struct device *dev,
				    uint32_t channel,
				    uint32_t period_cycles,
				    uint32_t pulse_cycles,
				    pwm_flags_t flags)
{
	uintptr_t base = pwm_ti_base(dev);
	bool inverted = (flags & PWM_POLARITY_INVERTED) != 0;

	if (channel > 1) {
		return -EINVAL;
	}
	if (period_cycles == 0U || period_cycles > 0x10000U) {
		/* TBPRD is 16-bit; period maps to TBPRD = period - 1. */
		return -ENOTSUP;
	}
	if (pulse_cycles > period_cycles) {
		pulse_cycles = period_cycles;
	}

	/* Up-count: T_pwm = (TBPRD + 1) x TBCLK  (TRM §7.5.6.4.3). */
	sys_write16((uint16_t)(period_cycles - 1U), base + EPWM_TBPRD);

	/*
	 * Single-edge up-count PWM, always set-high at CTR=0 and clear-low at
	 * the compare-up event. This action set is well-behaved at the extremes:
	 *   CMP = 0            -> 0% high (output stays low all period)
	 *   CMP > TBPRD        -> 100% high (compare never matches)
	 * The ZRO=clear variant is NOT used because at CMP=0 it leaves the pin
	 * stuck low (which, on the active-LOW LEDs, looked like "always on").
	 *
	 * Polarity is handled by complementing the compare value, not by
	 * swapping the actions:
	 *   normal:   high-time = pulse           -> CMP = pulse
	 *   inverted: high-time = period - pulse  -> CMP = period - pulse
	 * (Which polarity a given LED/load needs is a board fact set in the DT
	 * pwms flags, not assumed here.) Compare value lives in CMPx[31:16].
	 */
	uint32_t compare = inverted ? (period_cycles - pulse_cycles) : pulse_cycles;
	uint16_t cmpr = (uint16_t)(compare > 0xFFFFU ? 0xFFFFU : compare);
	uint16_t aqctl = (AQ_SET << AQCTL_ZRO_SHIFT) |
			 (AQ_CLEAR << (channel == 0 ? AQCTL_CAU_SHIFT : AQCTL_CBU_SHIFT));

	if (channel == 0) {
		sys_write16(aqctl, base + EPWM_AQCTLA);
		sys_write32((uint32_t)cmpr << 16, base + EPWM_CMPA);
	} else {
		sys_write16(aqctl, base + EPWM_AQCTLB);
		sys_write32((uint32_t)cmpr << 16, base + EPWM_CMPB);
	}

	return 0;
}

static int pwm_ti_am263x_get_cycles_per_sec(const struct device *dev,
					    uint32_t channel,
					    uint64_t *cycles)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	/*
	 * TBCLK = EPWMCLK = 200 MHz (this driver leaves TBCTL CLKDIV/HSPCLKDIV
	 * at 0, i.e. no time-base prescale). Verified from TI sources:
	 *   - CCS GEL Program_CONTROLSS_Clocks(): the CONTROLSS (ePWM) subsystem
	 *     clock = 400 MHz.
	 *   - MCU+ SDK etpwm.h: "EPWMCLK is a scaled version of SYSCLK. At reset
	 *     EPWMCLK is half SYSCLK" -> 400 / 2 = 200 MHz.
	 *   - SDK example epwm_xcmp_dma.c: EPWM_TBCLK_FREQUENCY_IN_HZ = 200000000.
	 * Duty *ratio* is independent of this; only the carrier frequency uses it.
	 */
	*cycles = 200000000ULL;
	return 0;
}

static const struct pwm_driver_api pwm_ti_am263x_api = {
	.set_cycles = pwm_ti_am263x_set_cycles,
	.get_cycles_per_sec = pwm_ti_am263x_get_cycles_per_sec,
};

#define PWM_TI_AM263X_INIT(n) \
	PINCTRL_DT_INST_DEFINE(n); \
	static struct pwm_ti_am263x_data pwm_ti_am263x_data_##n; \
	static const struct pwm_ti_am263x_config pwm_ti_am263x_config_##n = { \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)), \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
		.instance = (uint8_t)((DT_INST_REG_ADDR(n) - EPWM_INSTANCE_BASE) \
				      / EPWM_INSTANCE_STRIDE), \
	}; \
	DEVICE_DT_INST_DEFINE(n, pwm_ti_am263x_init, NULL, \
			      &pwm_ti_am263x_data_##n, &pwm_ti_am263x_config_##n, \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, \
			      &pwm_ti_am263x_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_TI_AM263X_INIT)
