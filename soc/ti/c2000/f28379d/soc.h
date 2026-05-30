/*
 * SPDX-License-Identifier: Apache-2.0
 * TI TMS320F28379D SoC definitions
 *
 * Memory map reference: TMS320F28379D Technical Reference Manual (SPRUI33)
 * All addresses are in 16-bit word units unless noted as byte addresses.
 */

#ifndef __SOC_TI_F28379D_H__
#define __SOC_TI_F28379D_H__

/* ---- CPU Timer registers (used for Zephyr system clock) ---- */
#define F28379D_CPUTIMER0_BASE  0x000C00U
#define F28379D_CPUTIMER1_BASE  0x000C08U
#define F28379D_CPUTIMER2_BASE  0x000C10U

/* CPU Timer register offsets (16-bit word offsets) */
#define CPUTIM_TIM      0x00U  /* Timer Counter Low */
#define CPUTIM_TIMH     0x01U  /* Timer Counter High */
#define CPUTIM_PRD      0x02U  /* Timer Period Low */
#define CPUTIM_PRDH     0x03U  /* Timer Period High */
#define CPUTIM_TCR      0x04U  /* Timer Control Register */
#define CPUTIM_TPR      0x06U  /* Timer Prescale Low */
#define CPUTIM_TPRH     0x07U  /* Timer Prescale High */

/* TCR bits */
#define CPUTIM_TCR_TIF  BIT(15) /* Timer Interrupt Flag */
#define CPUTIM_TCR_TIE  BIT(14) /* Timer Interrupt Enable */
#define CPUTIM_TCR_FREE BIT(11) /* Free run (debug) */
#define CPUTIM_TCR_SOFT BIT(10) /* Soft stop (debug) */
#define CPUTIM_TCR_TRB  BIT(5)  /* Timer Reload */
#define CPUTIM_TCR_TSS  BIT(4)  /* Timer Stop Status (1=stop) */

/* ---- PIE Controller ---- */
#define F28379D_PIE_BASE    0x000CE0U
#define F28379D_PIE_VECT    0x000D00U

/* ---- SCI (UART) ---- */
#define F28379D_SCIA_BASE   0x007200U
#define F28379D_SCIB_BASE   0x007210U
#define F28379D_SCIC_BASE   0x007220U  /* Only available if pinout supports it */
#define F28379D_SCID_BASE   0x007230U

/* ---- SPI ---- */
#define F28379D_SPIA_BASE   0x006100U
#define F28379D_SPIB_BASE   0x006110U
#define F28379D_SPIC_BASE   0x006120U

/* ---- I2C ---- */
#define F28379D_I2CA_BASE   0x007300U
#define F28379D_I2CB_BASE   0x007340U

/* ---- CAN ---- */
#define F28379D_CANA_BASE   0x048000U
#define F28379D_CANB_BASE   0x04A000U

/* ---- GPIO ---- */
#define F28379D_GPIO_CTRL_BASE  0x007C00U
#define F28379D_GPIO_DATA_BASE  0x007F00U

/* ---- System Control ---- */
#define F28379D_SYSCTRL_BASE    0x00D200U
#define F28379D_CLKCFG_BASE     0x005D200U  /* TODO: verify address */

/* ---- Watchdog ---- */
#define F28379D_WD_BASE         0x000900U
#define F28379D_WD_WDCR_OFFSET  0x01U
#define F28379D_WD_KEY          0x0055U     /* Write to WDKEY to service WDT */
#define F28379D_WD_DISABLE      0x0068U     /* WDCR value to disable WDT */

/* ---- Flash ---- */
#define F28379D_FLASH_BASE      0x080000U   /* Word address */
#define F28379D_FLASH_SIZE      (512U * 1024U)  /* 512KB = 256K words */

/* ---- RAM ---- */
#define F28379D_M0RAM_BASE      0x000000U   /* M0 local shared RAM */
#define F28379D_M0RAM_SIZE      (1U * 1024U)
#define F28379D_M1RAM_BASE      0x000400U   /* M1 local shared RAM */
#define F28379D_M1RAM_SIZE      (1U * 1024U)
#define F28379D_LSRAM_BASE      0x008000U   /* LS0-LS5 local shared RAM */
#define F28379D_LSRAM_SIZE      (12U * 1024U)
#define F28379D_GSRAM_BASE      0x00C000U   /* GS0-GS15 global shared RAM */
#define F28379D_GSRAM_SIZE      (32U * 1024U)

/* Total usable RAM (CPU1 view, conservative) */
#define F28379D_RAM_TOTAL       (F28379D_M0RAM_SIZE + F28379D_M1RAM_SIZE + \
				 F28379D_LSRAM_SIZE + F28379D_GSRAM_SIZE)

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

/* Disable the hardware watchdog — must be called early in SoC init */
static inline void f28379d_wd_disable(void)
{
	volatile uint16_t *wdcr =
		(volatile uint16_t *)(F28379D_WD_BASE + F28379D_WD_WDCR_OFFSET);

	/* EALLOW must be active to write to WD registers */
	/* TODO: wrap in EALLOW/EDIS — requires asm or TI DriverLib */
	*wdcr = F28379D_WD_DISABLE;
}

#endif /* _ASMLANGUAGE */

#endif /* __SOC_TI_F28379D_H__ */
