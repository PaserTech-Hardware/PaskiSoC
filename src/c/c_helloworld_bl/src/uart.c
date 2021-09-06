#include "uart.h"

uint32_t uart_writeAvailability(Uart_Reg *reg){
	return (reg->STATUS >> 16) & 0xFF;
}

uint32_t uart_readOccupancy(Uart_Reg *reg){
	return reg->STATUS >> 24;
}

void uart_write(Uart_Reg *reg, char data){
	while(uart_writeAvailability(reg) == 0);
	reg->DATA = data;
}

void uart_writeStr(Uart_Reg *reg, char* str){
	while(*str) uart_write(reg, *str++);
}

char uart_read(Uart_Reg *reg){
	while(uart_readOccupancy(reg) == 0);
	return reg->DATA;
}

void uart_applyConfig(Uart_Reg *reg, Uart_Config *config){
	reg->CLOCK_DIVIDER = config->clockDivider;
	reg->FRAME_CONFIG = ((config->dataLength-1) << 0) | (config->parity << 8) | (config->stop << 16);
}