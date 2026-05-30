/*
 * SPDX-License-Identifier: Apache-2.0
 * TI C28x interrupt management
 *
 * The C2000 interrupt architecture has two layers:
 *   CPU level : 16 CPU interrupt lines (INT1-INT12, DLOGINT, RTOSINT, NMI, RESET)
 *   PIE level : 96 peripheral interrupts multiplexed onto INT1-INT12 via the
 *               Peripheral Interrupt Expansion (PIE) controller
 *
 * Reference: TMS320F28379D Technical Reference Manual (SPRUI33)
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>

/* PIE base address — all C2000 devices */
#define PIE_CTRL_BASE   0x000CE0U
#define PIE_VECT_BASE   0x000D00U

/* PIE control register */
#define PIECTRL         (*(volatile uint16_t *)(PIE_CTRL_BASE + 0x00))
#define PIEACK          (*(volatile uint16_t *)(PIE_CTRL_BASE + 0x01))

/* PIECTRL bits */
#define PIECTRL_ENPIE   BIT(0)   /* Enable PIE */

/*
 * PIE interrupt number layout:
 *   irq = (group - 1) * 8 + (line - 1)
 *   group: 1-12 (INT1-INT12)
 *   line:  1-8
 *
 * CPU interrupt n maps to PIE group n (n = 1..12).
 */
#define PIE_GROUP(irq)  (((irq) >> 3) + 1)
#define PIE_LINE(irq)   (((irq) & 0x7) + 1)

/* CPU IER register (peripheral access, not a memory-mapped register —
 * accessed via assembly in the C28x architecture). Managed in swap.S. */

void arch_irq_enable(unsigned int irq)
{
	uint32_t key = irq_lock();

	if (irq < 96U) {
		/* PIE-level enable: set bit in PIEIERx register.
		 * PIEIERx is at PIE_CTRL_BASE + 0x02 + (group-1)*2 */
		unsigned int group = PIE_GROUP(irq);
		unsigned int line  = PIE_LINE(irq);
		volatile uint16_t *pieier =
			(volatile uint16_t *)(PIE_CTRL_BASE + 0x02U +
					      (group - 1U) * 2U);

		*pieier |= (uint16_t)BIT(line - 1U);

		/* Also enable the corresponding CPU-level IER bit.
		 * TODO: IER is a CPU register accessed via assembly — wrap
		 * in a small asm helper or use EALLOW/EDIS pattern. */
	}

	irq_unlock(key);
}

void arch_irq_disable(unsigned int irq)
{
	uint32_t key = irq_lock();

	if (irq < 96U) {
		unsigned int group = PIE_GROUP(irq);
		unsigned int line  = PIE_LINE(irq);
		volatile uint16_t *pieier =
			(volatile uint16_t *)(PIE_CTRL_BASE + 0x02U +
					      (group - 1U) * 2U);

		*pieier &= (uint16_t)~BIT(line - 1U);
	}

	irq_unlock(key);
}

int arch_irq_is_enabled(unsigned int irq)
{
	if (irq >= 96U) {
		return 0;
	}

	unsigned int group = PIE_GROUP(irq);
	unsigned int line  = PIE_LINE(irq);
	volatile uint16_t *pieier =
		(volatile uint16_t *)(PIE_CTRL_BASE + 0x02U +
				      (group - 1U) * 2U);

	return (*pieier >> (line - 1U)) & 0x1U;
}

/*
 * arch_irq_init — called once during kernel startup to enable the PIE.
 * Called from soc.c (SoC layer) before interrupts are unmasked.
 */
void arch_irq_init(void)
{
	PIECTRL |= PIECTRL_ENPIE;
}

/*
 * z_c28x_irq_dispatch — called from the PIE interrupt handler stub in swap.S.
 * Looks up the Zephyr ISR table entry for the given IRQ and invokes it.
 */
void z_c28x_irq_dispatch(unsigned int irq)
{
	struct _isr_table_entry *entry;

	if (irq >= CONFIG_NUM_IRQS) {
		z_c28x_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
		return;
	}

	entry = &_sw_isr_table[irq];
	entry->isr(entry->arg);

	/* Acknowledge the PIE group so further interrupts in this group
	 * can be taken. The PIEACK bit for the group is group number - 1. */
	PIEACK |= (uint16_t)BIT(PIE_GROUP(irq) - 1U);
}
