#include "uart.h"

void consoleLog(const char *color, const char *title, const char *content)
{
	uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, color);
	uart_write((Uart_Reg *)UART_CONFIG_BASE, '[');
	uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, title);
	uart_write((Uart_Reg *)UART_CONFIG_BASE, ']');
	uart_write((Uart_Reg *)UART_CONFIG_BASE, ' ');
	uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, content);
}

void printHex32(unsigned long x)
{
	for(unsigned int i = 0; i < 8; i ++)
	{
		unsigned long data = x & 0xf0000000;
		unsigned char printdata = (data >> 28) & 0xff;
		x <<= 4;
		if(printdata > 9)
			printdata = printdata - 10 + 'A';
		else
			printdata += '0';
		uart_write((Uart_Reg *)UART_CONFIG_BASE, printdata);
	}
}
