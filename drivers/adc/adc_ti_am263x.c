/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TI AM263x / AM263Px ADC driver (SAR ADC in CONTROLSS).
 *
 * Minimal bring-up: software-triggered one-shot conversions, polling EOC via
 * ADCINT1, 12-bit single-ended reads through the Zephyr ADC API. Enough to read
 * a channel from the shell and prove the peripheral (Phase 6 step 1).
 *
 * All register offsets / bit fields / sequences are taken from the MCU+ SDK
 * (no guessing — see CLAUDE.md Rule 1):
 *   - drivers/adc/v2/cslr_adc.h           (ADC config register map + fields)
 *   - drivers/adc/v2/cslr_adc_result.h    (ADCRESULTx)
 *   - drivers/adc/v2/adc.h                (ADC_CLK_DIV_4_0 = 6, sequences)
 *   - drivers/soc/am263px/soc.c           (SOC_enableAdcReference -> TOP_CTRL)
 *   - hw_include/am263px/cslr_soc_baseaddress.h, cslr_top_ctrl.h
 * Clock: rides the CONTROLSS source clock (enabled in soc.c); the per-ADC
 * ADC0_CLK_GATE is ungated at reset.
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/pinctrl.h>

#define DT_DRV_COMPAT ti_am263x_adc

/* ADC config-window register offsets (cslr_adc.h v2). */
#define ADC_ADCCTL1        0x00U   /* 16-bit */
#define ADC_ADCCTL2        0x02U   /* 16-bit */
#define ADC_ADCINTFLG      0x06U   /* 16-bit */
#define ADC_ADCINTFLGCLR   0x08U   /* 16-bit */
#define ADC_ADCINTSEL1N2   0x0EU   /* 16-bit */
#define ADC_ADCSOCFLG1     0x18U   /* 16-bit */
#define ADC_ADCSOCFRC1     0x1AU   /* 16-bit */
#define ADC_ADCSOC0CTL     0x20U   /* 32-bit, stride 4 per SOC */

/* Result window: ADCRESULT0 at +0x00, 16-bit, stride 2 (cslr_adc_result.h). */
#define ADC_RESULT0        0x00U

/* ADCCTL1 fields (cslr_adc.h). */
#define ADCCTL1_INTPULSEPOS  BIT(2)    /* 1 = INT pulse at end of conversion */
#define ADCCTL1_ADCPWDNZ     BIT(7)    /* 1 = analog powered up */
#define ADCCTL1_ADCBSY       BIT(13)

/* ADCCTL2.PRESCALE [3:0]; value 6 = ADC_CLK_DIV_4_0 (adc.h). */
#define ADCCTL2_PRESCALE_DIV4  6U

/* ADCSOCxCTL fields (32-bit) — cslr_adc.h. */
#define ADCSOCCTL_ACQPS_SHIFT    0U     /* [8:0]  acquisition window, ADCCLK cycles */
#define ADCSOCCTL_ACQPS_MASK     0x1FFU
#define ADCSOCCTL_CHSEL_SHIFT    15U    /* [19:15] channel select (AINx) */
#define ADCSOCCTL_CHSEL_MASK     0x1FU
#define ADCSOCCTL_TRIGSEL_SHIFT  20U    /* [26:20] trigger source; 0 = SW only */
#define ADCSOCCTL_TRIGSEL_SW     0U

/* ADCINTSEL1N2 fields (cslr_adc.h). */
#define ADCINTSEL_INT1SEL_SHIFT  0U     /* [4:0] EOC/SOC that sets INT1 */
#define ADCINTSEL_INT1E          BIT(5) /* enable INT1 */

/* Default sample window (SYSCLK cycles); ACQPS field = window - 1 (SDK adc.h). */
#define ADC_DEFAULT_SAMPLE_WINDOW  16U

/* ADCCTL2 fields (cslr_adc.h). */
#define ADCCTL2_PRESCALE_MASK    0x000FU
#define ADCCTL2_RESOLUTION_12BIT 0x0000U   /* bit6 = 0 */
#define ADCCTL2_SIGNALMODE_SE    0x0000U   /* bit7 = 0, single-ended */

/* TOP_CTRL — ADC reference buffer (cslr_top_ctrl.h, base cslr_soc_baseaddress.h). */
#define TOP_CTRL_BASE             0x50D80000U
#define TOP_CTRL_ADC_REFBUF0_CTRL 0xC00U
#define TOP_CTRL_ADC_REFBUF1_CTRL 0xC04U
#define TOP_CTRL_ADC_REFBUF2_CTRL 0xC94U
#define TOP_CTRL_ADC_REF_COMP_CTRL 0xC08U
#define TOP_CTRL_MASK_ANA_ISO     0xC30U
#define TOP_CTRL_LOCK0_KICK0      0x1008U
#define TOP_CTRL_LOCK0_KICK1      0x100CU
#define TOP_CTRL_KICK0_UNLOCK     0x01234567U  /* soc.h KICK0_UNLOCK_VAL */
#define TOP_CTRL_KICK1_UNLOCK     0x0FEDCBA8U  /* soc.h KICK1_UNLOCK_VAL */
#define TOP_CTRL_REFBUF_ENABLE    0x7U       /* REFBUF*_CTRL enable field */

/* CONTROLSS_CTRL — per-ADC clock gate + reset (cslr_controlss_ctrl.h). */
#define CONTROLSS_CTRL_BASE       0x502F0000U
#define CTRL_ADC0_CLK_GATE        0x25CU
#define CTRL_ADC0_RST             0x45CU
#define CTRL_ADC_INST_STRIDE      0x4U
#define CTRL_LOCK0_KICK0          0x1008U
#define CTRL_LOCK0_KICK1          0x100CU
#define CTRL_KICK0_UNLOCK         0x01234567U
#define CTRL_KICK1_UNLOCK         0x0FEDCBA8U
#define CTRL_CLK_GATE_MASK        0x7U       /* 0 = ungated */
#define CTRL_ADC_RST_ASSERT       0x7U

/* ADC instance bases are 0x1000 apart starting at 0x502C0000 (CSL). */
#define ADC_INSTANCE_BASE    0x502C0000U
#define ADC_INSTANCE_STRIDE  0x1000U

struct adc_ti_am263x_config {
	DEVICE_MMIO_ROM;
	const struct pinctrl_dev_config *pcfg;
	uint32_t result_base;   /* absolute addr of result window (DT reg[1]) */
	uint8_t instance;       /* 0..n, for the reference-buffer enable */
};

struct adc_ti_am263x_data {
	DEVICE_MMIO_RAM;
};

static inline uintptr_t adc_cfg_base(const struct device *dev)
{
	return DEVICE_MMIO_GET(dev);
}

static void adc_ti_controlss_unlock(void)
{
	sys_write32(CTRL_KICK0_UNLOCK, CONTROLSS_CTRL_BASE + CTRL_LOCK0_KICK0);
	sys_write32(CTRL_KICK1_UNLOCK, CONTROLSS_CTRL_BASE + CTRL_LOCK0_KICK1);
}

static void adc_ti_controlss_lock(void)
{
	sys_write32(0U, CONTROLSS_CTRL_BASE + CTRL_LOCK0_KICK0);
}

/*
 * Ungate this ADC instance's clock in CONTROLSS_CTRL. Mirrors SDK
 * SOC_ungateAdcClock(). After a CCS "Restart" the gate can be set like ePWM
 * TBCLK — reads then look like a stuck full-scale (0xFFF).
 */
static void adc_ti_ungate_clock(uint8_t instance)
{
	uint32_t gate_off = CTRL_ADC0_CLK_GATE + ((uint32_t)instance * CTRL_ADC_INST_STRIDE);

	adc_ti_controlss_unlock();
	sys_write32(0U, CONTROLSS_CTRL_BASE + gate_off);
	adc_ti_controlss_lock();
}

/* Pulse ADC reset (SDK SOC_generateAdcReset) before reference + power-up. */
static void adc_ti_reset_instance(uint8_t instance)
{
	uint32_t rst_off = CTRL_ADC0_RST + ((uint32_t)instance * CTRL_ADC_INST_STRIDE);

	adc_ti_controlss_unlock();
	sys_write32(CTRL_ADC_RST_ASSERT, CONTROLSS_CTRL_BASE + rst_off);
	sys_write32(0U, CONTROLSS_CTRL_BASE + rst_off);
	adc_ti_controlss_lock();
}

/*
 * Enable the ADC reference buffer in TOP_CTRL (KICK-locked). Mirrors SDK
 * SOC_enableAdcReference() + the MASK_ANA_ISO sequence from
 * SOC_enableAdcInternalReference() (soc.c).
 */
static void adc_ti_enable_reference(uint8_t instance)
{
	uint32_t refbuf_off = TOP_CTRL_ADC_REFBUF0_CTRL;
	uint16_t comp_mask = TOP_CTRL_REFBUF_ENABLE;

	if (instance == 1U || instance == 2U) {
		refbuf_off = TOP_CTRL_ADC_REFBUF0_CTRL;
		comp_mask = TOP_CTRL_REFBUF_ENABLE << 4;
	} else if (instance == 3U || instance == 4U) {
		refbuf_off = TOP_CTRL_ADC_REFBUF1_CTRL;
		comp_mask = TOP_CTRL_REFBUF_ENABLE << 8;
	} else if (instance == 5U || instance == 6U) {
		refbuf_off = TOP_CTRL_ADC_REFBUF2_CTRL;
		comp_mask = TOP_CTRL_REFBUF_ENABLE << 12;
	}

	sys_write32(TOP_CTRL_KICK0_UNLOCK, TOP_CTRL_BASE + TOP_CTRL_LOCK0_KICK0);
	sys_write32(TOP_CTRL_KICK1_UNLOCK, TOP_CTRL_BASE + TOP_CTRL_LOCK0_KICK1);

	/* Mask HHV before enabling ref (SDK soc.c). */
	sys_write16(TOP_CTRL_REFBUF_ENABLE,
		    TOP_CTRL_BASE + TOP_CTRL_MASK_ANA_ISO);

	sys_write16(TOP_CTRL_REFBUF_ENABLE, TOP_CTRL_BASE + refbuf_off);
	uint16_t comp = sys_read16(TOP_CTRL_BASE + TOP_CTRL_ADC_REF_COMP_CTRL);

	sys_write16(comp | comp_mask, TOP_CTRL_BASE + TOP_CTRL_ADC_REF_COMP_CTRL);

	sys_write16(0U, TOP_CTRL_BASE + TOP_CTRL_MASK_ANA_ISO);

	sys_write32(0U, TOP_CTRL_BASE + TOP_CTRL_LOCK0_KICK0);
}

static int adc_ti_am263x_channel_setup(const struct device *dev,
				       const struct adc_channel_cfg *chan_cfg)
{
	uintptr_t base = adc_cfg_base(dev);
	uint8_t soc = chan_cfg->channel_id;   /* SOC number = channel id (0-15) */
	uint32_t acqps;

	if (soc > 15U) {
		return -EINVAL;
	}

	uint32_t sample_window = ADC_DEFAULT_SAMPLE_WINDOW;

	if (chan_cfg->acquisition_time == ADC_ACQ_TIME_DEFAULT) {
		sample_window = ADC_DEFAULT_SAMPLE_WINDOW;
	} else if (ADC_ACQ_TIME_UNIT(chan_cfg->acquisition_time) == ADC_ACQ_TIME_TICKS) {
		sample_window = ADC_ACQ_TIME_VALUE(chan_cfg->acquisition_time);
	}
	if (sample_window < 1U) {
		sample_window = 1U;
	}
	if (sample_window > (ADCSOCCTL_ACQPS_MASK + 1U)) {
		sample_window = ADCSOCCTL_ACQPS_MASK + 1U;
	}
	/* SDK ADC_setupSOC(): ACQPS register = sampleWindow - 1. */
	acqps = sample_window - 1U;

	/* SOCx: SW trigger, channel = SOC number (AINx = channel_id), acq window.
	 * (1:1 SOC<->channel keeps this independent of CONFIG_ADC_CONFIGURABLE_INPUTS.)
	 */
	uint32_t soc_ctl =
		((acqps & ADCSOCCTL_ACQPS_MASK) << ADCSOCCTL_ACQPS_SHIFT) |
		(((uint32_t)soc & ADCSOCCTL_CHSEL_MASK) << ADCSOCCTL_CHSEL_SHIFT) |
		((uint32_t)ADCSOCCTL_TRIGSEL_SW << ADCSOCCTL_TRIGSEL_SHIFT);

	sys_write32(soc_ctl, base + ADC_ADCSOC0CTL + ((uint32_t)soc * 4U));

	/* SDK ADC_setInterruptSource() + ADC_enableInterrupt(): map INT1 to this
	 * SOC's EOC and enable it. Done here (once) not per-read.
	 */
	sys_write16((uint16_t)(((uint32_t)soc << ADCINTSEL_INT1SEL_SHIFT) | ADCINTSEL_INT1E),
		    base + ADC_ADCINTSEL1N2);

	return 0;
}

static int adc_ti_am263x_read(const struct device *dev,
			      const struct adc_sequence *seq)
{
	uintptr_t base = adc_cfg_base(dev);
	const struct adc_ti_am263x_config *cfg = dev->config;
	uint32_t channels = seq->channels;
	int soc = -1;

	if (seq->resolution != 12U) {
		return -ENOTSUP;
	}
	if (seq->buffer_size < sizeof(uint16_t)) {
		return -ENOMEM;
	}

	/* Minimal: exactly one channel per call. */
	for (int i = 0; i < 16; i++) {
		if (channels & (1U << i)) {
			if (soc >= 0) {
				return -ENOTSUP;
			}
			soc = i;
		}
	}
	if (soc < 0) {
		return -EINVAL;
	}

	/* Clear any stale INT1, force the SW SOC, wait for EOC (SDK poll). */
	sys_write16(0x1U, base + ADC_ADCINTFLGCLR);
	sys_write16((uint16_t)(1U << soc), base + ADC_ADCSOCFRC1);

	uint32_t timeout = 100000U;

	while (timeout-- != 0U) {
		if ((sys_read16(base + ADC_ADCINTFLG) & 0x1U) != 0U) {
			break;
		}
	}
	if (timeout == 0U) {
		return -ETIMEDOUT;
	}

	/* 12-bit right-justified result (stride 2 per SOC, cslr_adc_result.h). */
	uint16_t raw = sys_read16(cfg->result_base + ADC_RESULT0 + ((uint32_t)soc * 2U)) & 0x0FFFU;

	*(uint16_t *)seq->buffer = raw;

	sys_write16(0x1U, base + ADC_ADCINTFLGCLR);

	return 0;
}

static int adc_ti_am263x_init(const struct device *dev)
{
	const struct adc_ti_am263x_config *cfg = dev->config;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	adc_ti_ungate_clock(cfg->instance);
	adc_ti_reset_instance(cfg->instance);
	adc_ti_enable_reference(cfg->instance);

	uintptr_t base = adc_cfg_base(dev);

	/* 12-bit single-ended + ADCCLK prescale (/4, SDK ADC_CLK_DIV_4_0 = 6). */
	uint16_t ctl2 = sys_read16(base + ADC_ADCCTL2);

	ctl2 &= ~(ADCCTL2_PRESCALE_MASK |
		  (0x0040U) | (0x0080U)); /* RESOLUTION + SIGNALMODE */
	ctl2 |= (ADCCTL2_PRESCALE_DIV4 & ADCCTL2_PRESCALE_MASK);
	ctl2 |= ADCCTL2_RESOLUTION_12BIT | ADCCTL2_SIGNALMODE_SE;
	sys_write16(ctl2, base + ADC_ADCCTL2);

	/* INT pulse at end of conversion, then power up the analog core. */
	uint16_t ctl1 = sys_read16(base + ADC_ADCCTL1);

	ctl1 |= ADCCTL1_INTPULSEPOS;
	ctl1 |= ADCCTL1_ADCPWDNZ;
	sys_write16(ctl1, base + ADC_ADCCTL1);

	sys_write16(0xFFFFU, base + ADC_ADCINTFLGCLR);

	/* SDK adc.h: allow >=500 us after power-up before the first sample. */
	k_busy_wait(500);

	return 0;
}

static const struct adc_driver_api adc_ti_am263x_api = {
	.channel_setup = adc_ti_am263x_channel_setup,
	.read = adc_ti_am263x_read,
};

#define ADC_TI_AM263X_INIT(n) \
	PINCTRL_DT_INST_DEFINE(n); \
	static struct adc_ti_am263x_data adc_ti_am263x_data_##n; \
	static const struct adc_ti_am263x_config adc_ti_am263x_config_##n = { \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)), \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
		.result_base = DT_INST_REG_ADDR_BY_IDX(n, 1), \
		.instance = (uint8_t)((DT_INST_REG_ADDR_BY_IDX(n, 0) - ADC_INSTANCE_BASE) \
				      / ADC_INSTANCE_STRIDE), \
	}; \
	DEVICE_DT_INST_DEFINE(n, adc_ti_am263x_init, NULL, \
			      &adc_ti_am263x_data_##n, &adc_ti_am263x_config_##n, \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, \
			      &adc_ti_am263x_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_TI_AM263X_INIT)
