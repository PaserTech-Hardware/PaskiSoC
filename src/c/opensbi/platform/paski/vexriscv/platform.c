/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Angelic47 <admin@angelic47.com>
 *
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/uart8250.h>
#include <sbi_utils/sys/clint.h>

/* clang-format off */

#define VEX_HART_COUNT         1
#define VEX_PLATFORM_FEATURES  (SBI_PLATFORM_HAS_TIMER_VALUE | SBI_PLATFORM_HAS_MFAULTS_DELEGATION)
#define VEX_HART_STACK_SIZE	   8192

/* clang-format on */

/* timer */
#define PASKI_TIMER_REG_WRITE4(x,y) *((volatile unsigned long *)(x)) = (y)
#define PASKI_TIMER_REG_READ4(x) (*((volatile unsigned long *)(x)))

#define PASKI_SOC_TIMER_BASE 0xF0003000
#define PASKI_SOC_TIMER_PRESCALER_BASE (PASKI_SOC_TIMER_BASE + 0x0)
#define PASKI_SOC_TIMER_INTCTRL (PASKI_SOC_TIMER_BASE + 0x10)
#define PASKI_SOC_TIMER_A_BASE (PASKI_SOC_TIMER_BASE + 0x40)
#define PASKI_SOC_TIMER_B_BASE (PASKI_SOC_TIMER_BASE + 0x60)

#define PASKI_SOC_TIMER_REG_CTMASK(x) (x + 0x0)
#define PASKI_SOC_TIMER_REG_LIMIT_H32(x) (x + 0x4)
#define PASKI_SOC_TIMER_REG_LIMIT_L32(x) (x + 0x8)
#define PASKI_SOC_TIMER_REG_SEND_CACHE(x) (x + 0xc)
#define PASKI_SOC_TIMER_REG_CACHE_VAL_H32(x) (x + 0x10)
#define PASKI_SOC_TIMER_REG_CACHE_VAL_L32(x) (x + 0x14)

#define PASKI_SOC_INTCTRL_REG_PENDINGS(x) (x + 0x0)
#define PASKI_SOC_INTCTRL_REG_MASKS(x) (x + 0x4)
/* end timer */

static int vex_final_init(bool cold_boot)
{
	return 0;
}

static u32 vex_pmp_region_count(u32 hartid)
{
	return 0;
}

static int vex_pmp_region_info(u32 hartid, u32 index, ulong *prot, ulong *addr,
				ulong *log2size)
{
	int ret = 0;

	switch (index) {
	default:
		ret = -1;
		break;
	};

	return ret;
}


extern void vex_putc(char ch);
extern int vex_getc(void);

static int vex_console_init(void)
{
	return 0;
}

static int vex_irqchip_init(bool cold_boot)
{
	return 0;
}

void vex_ipi_clear(u32 target_hart)
{
	
}

void vex_ipi_send(u32 target_hart)
{
	
}

static int vex_ipi_init(bool cold_boot)
{
	return 0;
}

static int vex_timer_init(bool cold_boot)
{
	// timer A is use to generate interrupt
	// timer A limit sets to max && auto reload
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_LIMIT_H32(PASKI_SOC_TIMER_A_BASE), 0xFFFFFFFF);
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_LIMIT_L32(PASKI_SOC_TIMER_A_BASE), 0xFFFFFFFF);
	// timer A tick by sysclk to enable, clears by none
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_CTMASK(PASKI_SOC_TIMER_A_BASE), 0x00000001);
	
	// timer B is use to get counter
	// timer B limit sets to max
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_LIMIT_H32(PASKI_SOC_TIMER_B_BASE), 0xFFFFFFFF);
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_LIMIT_L32(PASKI_SOC_TIMER_B_BASE), 0xFFFFFFFF);
	// timer B tick by sysclk to enable, clears by overflow
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_CTMASK(PASKI_SOC_TIMER_B_BASE), 0x00010001);
	return 0;
}

static int vex_system_reset(u32 type)
{
	/* Tell the "finisher" that the simulation
	 * was successful so that QEMU exits
	 */

	return 0;
}

u64 vex_timer_value(void)
{
	u32 hi, lo;
	
	// timer B get data to cache
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_SEND_CACHE(PASKI_SOC_TIMER_B_BASE), 0xFFFFFFFF);
	hi = PASKI_TIMER_REG_READ4(PASKI_SOC_TIMER_REG_CACHE_VAL_H32(PASKI_SOC_TIMER_B_BASE));
	lo = PASKI_TIMER_REG_READ4(PASKI_SOC_TIMER_REG_CACHE_VAL_L32(PASKI_SOC_TIMER_B_BASE));
	
	return (((u64)hi) << 32) | ((u64)lo);
}

void vex_timer_event_stop(void)
{
	// disable timer A
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_CTMASK(PASKI_SOC_TIMER_A_BASE), 0x00000000);
	// disable timer A interrupt
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_INTCTRL_REG_MASKS(PASKI_SOC_TIMER_INTCTRL), 0x00000000);
	// clear timer A interrupt pending
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_INTCTRL_REG_PENDINGS(PASKI_SOC_TIMER_INTCTRL), 0x00000001);
}

void vex_timer_event_start(u64 next_event)
{
	u64 next = next_event - vex_timer_value();
	
	// disable timer A
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_CTMASK(PASKI_SOC_TIMER_A_BASE), 0x00000000);
	
	// reload timer A limit
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_LIMIT_H32(PASKI_SOC_TIMER_A_BASE), ((u32)(next >> 32)));
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_LIMIT_L32(PASKI_SOC_TIMER_A_BASE), ((u32)(next & (u64)(0xffffffff))));
	
	// clear timer A interrupt pending
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_INTCTRL_REG_PENDINGS(PASKI_SOC_TIMER_INTCTRL), 0x00000001);
	// enable timer A interrupt
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_INTCTRL_REG_MASKS(PASKI_SOC_TIMER_INTCTRL), 0x00000001);
	
	// enable timer A
	// timer A tick by sysclk to enable, clears by none
	PASKI_TIMER_REG_WRITE4(PASKI_SOC_TIMER_REG_CTMASK(PASKI_SOC_TIMER_A_BASE), 0x00000001);
}

const struct sbi_platform_operations platform_ops = {
	.pmp_region_count	= vex_pmp_region_count,
	.pmp_region_info	= vex_pmp_region_info,
	.final_init		    = vex_final_init,
	.console_putc		= vex_putc,
	.console_getc		= vex_getc,
	.console_init		= vex_console_init,
	.irqchip_init		= vex_irqchip_init,
	.ipi_send		    = vex_ipi_send,
	.ipi_clear		    = vex_ipi_clear,
	.ipi_init		    = vex_ipi_init,
	.timer_value		= vex_timer_value,
	.timer_event_stop	= vex_timer_event_stop,
	.timer_event_start	= vex_timer_event_start,
	.timer_init			= vex_timer_init,
	.system_reset		= vex_system_reset
};

const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x01),
	.name			    = "Angelic47 / Paski",
	.features		    = VEX_PLATFORM_FEATURES,
	.hart_count		    = VEX_HART_COUNT,
	.hart_stack_size	= VEX_HART_STACK_SIZE,
	.platform_ops_addr	= (unsigned long)&platform_ops
};


