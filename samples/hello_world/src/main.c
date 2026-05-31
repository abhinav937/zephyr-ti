/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal LED blink for bring-up on the TMDSCNCD263P AM263Px Control Card.
 *
 * Toggles the two USER LEDs (active-low), confirmed against the EVM User's
 * Guide SPRUJ86E, Table 2-12 "GPIO Mapping":
 *
 *   USER_LED0 = GPIO22, pad EPWM7_A   (active LOW)
 *   USER_LED1 = GPIO58, pad EPWM7_B   (active LOW)
 *
 * IMPORTANT: both pads power up muxed to their EPWM7 function, NOT to GPIO.
 * The GPIO controller cannot drive the physical ball until the pad's
 * PADCONFIG mux mode is switched to GPIO. That is what pinmux_led_pads()
 * below does. Without it, the GPIO register writes do nothing visible.
 *
 * This version intentionally uses a busy-wait (not k_msleep) so the blink
 * does not depend on the RTI system-tick driver during first bring-up.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

/* ------------------------------------------------------------------ */
/* GPIO controller (Davinci-style banked registers, base 0x52000000). */
/*                                                                     */
/* Banked layout: pins 0-31 live in the "...01" register set,         */
/* pins 32-63 in the "...23" set. So:                                  */
/*   GPIO22 -> "01" set, bit 22                                        */
/*   GPIO58 -> "23" set, bit (58 - 32) = 26                            */
/* ------------------------------------------------------------------ */
#define GPIO0_BASE          0x52000000U

/* Bank 0/1 (GPIO 0..31) */
#define GPIO_DIR01          (GPIO0_BASE + 0x10)
#define GPIO_OUT_DATA01     (GPIO0_BASE + 0x14)
#define GPIO_SET_DATA01     (GPIO0_BASE + 0x18)
#define GPIO_CLR_DATA01     (GPIO0_BASE + 0x1C)

/* Bank 2/3 (GPIO 32..63) */
#define GPIO_DIR23          (GPIO0_BASE + 0x38)
#define GPIO_OUT_DATA23     (GPIO0_BASE + 0x3C)
#define GPIO_SET_DATA23     (GPIO0_BASE + 0x40)
#define GPIO_CLR_DATA23     (GPIO0_BASE + 0x44)

#define LED0_BIT            22U   /* USER_LED0 = GPIO22, bank "01" */
#define LED1_BIT            26U   /* USER_LED1 = GPIO58, bank "23" (58-32) */

/* ================================================================== */
/* PINMUX --- FILL THESE 4 VALUES FROM SysConfig (or the AM263Px TRM) */
/* ================================================================== */
/*
 * How to get them (MCU+ SDK SysConfig, ~2 min):
 *   1. Open/Make a SysConfig file, add the GPIO module, add two outputs.
 *   2. Assign one to ball EPWM7_A (USER_LED0/GPIO22) and one to EPWM7_B
 *      (USER_LED1/GPIO58).
 *   3. Generate, then open ti_drivers_config.c -> Pinmux_init(). You'll see
 *      two lines like:  Pinmux_config(PIN_EPWM7_A, PIN_MODE(n) | ...);
 *   4. From pinmux.h: PADCFG_BASE is the IOMUX base used by Pinmux_config;
 *      PIN_EPWM7_A / PIN_EPWM7_B are the per-pad offsets; n is the GPIO mode.
 *
 * Leave PADCFG_BASE as 0 to SKIP pinmux (LEDs will NOT light, but the rest
 * of the program still runs so you can prove execution via a breakpoint).
 */
#define PADCFG_BASE         0x00000000U   /* <-- IOMUX/pad-config base addr   */
#define PADCFG_OFF_LED0     0x00000000U   /* <-- offset for EPWM7_A (GPIO22)  */
#define PADCFG_OFF_LED1     0x00000000U   /* <-- offset for EPWM7_B (GPIO58)  */
#define PAD_GPIO_MODE       7U            /* <-- GPIO mux mode (often 7)      */

/*
 * Typical AM26x PADCONFIG field layout (verify in the TRM IOMUX chapter):
 *   bits [3:0]  MUXMODE   - function select (use PAD_GPIO_MODE)
 *   bit  [16]   RXACTIVE  - 1 = input buffer enabled (set so we can read back)
 *   bit  [8]    PULLUDEN  - 1 = internal pull DISABLED (active-low enable)
 * For a push-pull output driving an LED this is a safe starting value.
 */
#define PAD_RXACTIVE        (1U << 16)
#define PAD_PULL_DISABLE    (1U << 8)

static void pinmux_led_pads(void)
{
	if (PADCFG_BASE == 0U) {
		return; /* pinmux not configured yet -- see block above */
	}

	uint32_t cfg = (PAD_GPIO_MODE & 0xFU) | PAD_RXACTIVE | PAD_PULL_DISABLE;

	sys_write32(cfg, PADCFG_BASE + PADCFG_OFF_LED0);
	sys_write32(cfg, PADCFG_BASE + PADCFG_OFF_LED1);
}

/* ------------------------------------------------------------------ */
/* LED helpers. LEDs are active-LOW: clearing the bit drives 0 = ON.  */
/* ------------------------------------------------------------------ */
static inline void led0_on(void)  { sys_write32(1U << LED0_BIT, GPIO_CLR_DATA01); }
static inline void led0_off(void) { sys_write32(1U << LED0_BIT, GPIO_SET_DATA01); }
static inline void led1_on(void)  { sys_write32(1U << LED1_BIT, GPIO_CLR_DATA23); }
static inline void led1_off(void) { sys_write32(1U << LED1_BIT, GPIO_SET_DATA23); }

static void busy_delay(void)
{
	/* ~ a few hundred ms at 400 MHz; tune to taste. volatile = not optimized away. */
	for (volatile uint32_t i = 0; i < 8000000U; i++) {
		__asm__ volatile("nop");
	}
}

int main(void)
{
	/* 1. Route the two pads to GPIO (the step that actually matters). */
	pinmux_led_pads();

	/* 2. Configure both GPIOs as outputs (DIR bit = 0 means output). */
	sys_write32(sys_read32(GPIO_DIR01) & ~(1U << LED0_BIT), GPIO_DIR01);
	sys_write32(sys_read32(GPIO_DIR23) & ~(1U << LED1_BIT), GPIO_DIR23);

	/* 3. Start with both off. */
	led0_off();
	led1_off();

	while (1) {
		/* Alternate: USER_LED0 then USER_LED1. */
		led0_on();
		led1_off();
		busy_delay();

		led0_off();
		led1_on();
		busy_delay();
	}

	return 0;
}
