/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Angelic47 <admin@angelic47.com>
 *
 */

#include <stdint.h>

typedef struct
{
  volatile unsigned long DATA;
  volatile unsigned long STATUS;
  volatile unsigned long CLOCK_DIVIDER;
  volatile unsigned long FRAME_CONFIG;
} Uart_Reg;

enum UartDataLength {BITS_8 = 8};
enum UartParity {NONE = 0,EVEN = 1,ODD = 2};
enum UartStop {ONE = 0,TWO = 1};

typedef struct {
	enum UartDataLength dataLength;
	enum UartParity parity;
	enum UartStop stop;
	unsigned long clockDivider;
} Uart_Config;

#define UART_CONFIG_BASE 0xF0001000

unsigned long uart_writeAvailability(Uart_Reg *reg){
	return (reg->STATUS >> 16) & 0xFF;
}

unsigned long uart_readOccupancy(Uart_Reg *reg){
	return reg->STATUS >> 24;
}

void uart_write(Uart_Reg *reg, char data){
	while(uart_writeAvailability(reg) == 0);
	reg->DATA = data;
}

void uart_writeStr(Uart_Reg *reg, char* str){
	while(*str) uart_write(reg, *str++);
}

int uart_read(Uart_Reg *reg){
	if(uart_readOccupancy(reg) == 0)
		return -1;
	return reg->DATA & 0xff;
}

void uart_applyConfig(Uart_Reg *reg, Uart_Config *config){
	reg->CLOCK_DIVIDER = config->clockDivider;
	reg->FRAME_CONFIG = ((config->dataLength-1) << 0) | (config->parity << 8) | (config->stop << 16);
}


void vex_putc(char c){
	uart_write((Uart_Reg *)UART_CONFIG_BASE, c);
}

int vex_getc(void){
	return uart_read((Uart_Reg *)UART_CONFIG_BASE);
}
