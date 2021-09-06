#ifndef __BL_HAL_UART__
#define __BL_HAL_UART__

typedef unsigned long uint32_t;

typedef struct
{
  volatile uint32_t DATA;
  volatile uint32_t STATUS;
  volatile uint32_t CLOCK_DIVIDER;
  volatile uint32_t FRAME_CONFIG;
} Uart_Reg;

enum UartDataLength {BITS_8 = 8};
enum UartParity {NONE = 0,EVEN = 1,ODD = 2};
enum UartStop {ONE = 0,TWO = 1};

typedef struct {
	enum UartDataLength dataLength;
	enum UartParity parity;
	enum UartStop stop;
	uint32_t clockDivider;
} Uart_Config;

#define UART_CONFIG_BASE 0xF0001000

uint32_t uart_writeAvailability(Uart_Reg *reg);
uint32_t uart_readOccupancy(Uart_Reg *reg);
void uart_write(Uart_Reg *reg, char data);
void uart_writeStr(Uart_Reg *reg, char* str);
char uart_read(Uart_Reg *reg);
void uart_applyConfig(Uart_Reg *reg, Uart_Config *config);

#endif
