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
/* PINMUX --- AM263x IOMUX / pad config                               */
/* ================================================================== */
/*
 * Each pad's config register lives at PADCFG_BASE + <pad offset>.
 * Values verified against the TI MCU+ SDK pinmux.h for AM263Px and the
 * AM263Px datasheet (SPRSP81):
 *   PADCFG_BASE      = 0x53100000  (IOMUX pad-config block)
 *   EPWM7_B pad off  = 0xE8        (this pad muxes to GPIO58 in mode 7)
 *
 * Pad-config field layout (from pinmux.h):
 *   bits [3:0]   MUXMODE       7 = GPIO function
 *   bit  [8]     PULL_DISABLE  1 = internal pull disabled
 *   bits [17:16] GPIO owner    0 = R5SS0 core0 (our core)  -> PIN_GPIO_R5SS0_0
 * Output direction is then controlled by the GPIO DIR register, so we do
 * not need to force the output buffer here.
 */
#define PADCFG_BASE         0x53100000U

#define PAD_OFF_EPWM7_B     0x000000E8U   /* -> GPIO58 = USER_LED1 (LD7) */
#define PAD_OFF_LIN2_TXD    0x00000058U   /* -> GPIO22 = USER_LED0 (LD6) */
/*
 * Both verified in datasheet SPRSP81 Table 5-1 (Pin Attributes):
 *   ball A8 LIN2_TXD,  LIN2_TXD_CFG_REG @ 0x53100058, mode 7 = GPIO22
 *   ball ?? EPWM7_B,   EPWM7_B_CFG_REG  @ 0x531000E8, mode 7 = GPIO58
 * (The ControlCARD guide SPRUJ86 Table 2-12 has a typo naming GPIO22's pad
 *  "EPWM7_B"; the datasheet pin table is authoritative — it is LIN2_TXD.)
 */

#define PAD_GPIO_MODE7      0x7U
#define PAD_PULL_DISABLE    (0x1U << 8)
#define PAD_GPIO_R5SS0_0    (0x0U << 16)
#define PAD_CFG_GPIO_OUT    (PAD_GPIO_MODE7 | PAD_PULL_DISABLE | PAD_GPIO_R5SS0_0)

static void pinmux_led_pads(void)
{
	/* Route both LED pads to GPIO mode (mux 7), owned by R5SS0 core0. */
	sys_write32(PAD_CFG_GPIO_OUT, PADCFG_BASE + PAD_OFF_LIN2_TXD); /* LD6/GPIO22 */
	sys_write32(PAD_CFG_GPIO_OUT, PADCFG_BASE + PAD_OFF_EPWM7_B);  /* LD7/GPIO58 */
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
