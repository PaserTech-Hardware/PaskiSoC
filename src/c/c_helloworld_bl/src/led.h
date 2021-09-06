#ifndef __BL_HAL_LED__
#define __BL_HAL_LED__

#define LED_BASE 0xF0000000
#define LED_OUTPUT (LED_BASE + 0x04)
#define LED_IO_DRIECTION (LED_BASE + 0x08)
#define LED_REG_WRITE4(x,y) *((volatile unsigned long *)(x)) = (y)

#define LED_SET_DATA(x) LED_REG_WRITE4(LED_OUTPUT, (x))
#define LED_SET_IO_DRIECTION(x) LED_REG_WRITE4(LED_IO_DRIECTION, (x))

#endif
