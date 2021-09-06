#include "led.h"
#include "uart.h"
#include "logging.h"
#include "color.h"
#include "sd.h"

//#define SIMULATE_DEBUG 1

#ifndef SIMULATE_DEBUG

void sleepms(int ms)
{
	for(volatile int i = 0; i < ms; i ++)
		for(volatile int j = 0; j < 5000; j ++);
}

#else

void sleepms(int ms)
{
	for(volatile int i = 0; i < ms / 100; i ++);
}

#endif

/*
void sdram_memscan()
{
	unsigned long a = 0xaabbccdd;
	unsigned long b = 0xaabbccdd;
	for(unsigned long i = 0x40000000; i < 0x42000000; i += 0x400000)
	{
		volatile unsigned long *ptr_1 = (unsigned long *)i;
		volatile unsigned long *ptr_2 = (unsigned long *)i;
		consoleLog(HMAG, "Boot-SelfCheck", "Checking SDRAM memory at 0x");
		printHex32((unsigned long)ptr_1);
		uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, ", size: 0x800000\r\n");
		consoleLog(HCYN, "Boot-SelfCheck", "Filling memory....\r\n");
		for(unsigned j = 0; j < 0x100000; j ++)
		{
			ptr_1[j] = a;
			a += 1;
		}
		consoleLog(HCYN, "Boot-SelfCheck", "Checking....\r\n");
		for(unsigned j = 0; j < 0x100000; j ++)
		{
			unsigned long result = ptr_2[j];
			if(result != b)
			{
				consoleLog(HRED, "Boot-SelfCheck", "SDRAM memory fail at 0x");
				printHex32((unsigned long)(&(ptr_2[j])));
				uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, ", excepted: 0x");
				printHex32(b);
				uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, ", but found: 0x");
				printHex32(result);
				uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, "\r\n");
				while(1);
			}
			b += 1;
		}
		consoleLog(HGRN, "Boot-SelfCheck", "Block check OK!\r\n");
	}
	consoleLog(HGRN, "Boot-SelfCheck", "SDRAM check finished, OK!\r\n\r\n");
}
*/

/*
void led_blinking()
{
	LED_SET_IO_DRIECTION(0xf);
	consoleLog(HGRN, "Apploader", "Now blinking 4 leds!\r\n");
	while(1)
	{
		char led = 0;
		for(int i = 0; i < 4; i ++)
		{
			led <<= 1;
			led |= 1;
			LED_SET_DATA(led);
			sleepms(125);
		}
		for(int i = 0; i < 4; i ++)
		{
			led <<= 1;
			LED_SET_DATA(led);
			sleepms(125);
		}
	}
}
*/

void sd_boot(unsigned long bootmem, unsigned long sector, int jumptoentry)
{
	static int sd_init_finish = 0;
	
	consoleLog(HMAG, "1stBoot", "Reading Image from SD!\r\n");
	char *boot_sdram = (char *)bootmem;
	
	if(!sd_init_finish)
	{
		if(sd_init())
		{
			consoleLog(HRED, "1stBoot", "No SD or SD error!\r\n");
			while(1);
		}
		sd_init_finish = 1;
	}
	
	int result = sd_read_data(boot_sdram, sector, 1);
	if(result)
	{
		consoleLog(HRED, "1stBoot", "Failed to read SD! (1)\r\n");
		while(1);
	}
	unsigned long magic = ((unsigned long *)boot_sdram)[0];
	unsigned long image_length = ((unsigned long *)boot_sdram)[1];
	unsigned long entryaddr = ((unsigned long *)boot_sdram)[2];
	unsigned long checksum_get = ((unsigned long *)boot_sdram)[3];
	if(magic != 0x4c424e41) // ANBL
	{
		consoleLog(HRED, "1stBoot", "Bad SD boot magic 0x");
		printHex32(magic);
		uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, "!\r\n");
		while(1);
	}
	unsigned long length = image_length;
	consoleLog(HCYN, "1stBoot", "Reading SD image (length: 0x");
	printHex32(length);
	uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, ")...\r\n");
	consoleLog(HWHT, "1stBoot", "Progress: ");
	if(length > 512 - 4)
	{
		length -= 512 - 4;
		unsigned int chunk = sector;
		while(length)
		{
			if(length > 512)
				length -= 512;
			else
				length = 0;
			chunk += 1;
			boot_sdram += 512;
			result = sd_read_data(boot_sdram, chunk, 1); 
			if(result)
			{
				uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, HRED " FAIL!\r\n");
				consoleLog(HRED, "1stBoot", "Failed to read SD! (2)\r\n");
				while(1);
			}
			uart_write((Uart_Reg *)UART_CONFIG_BASE, '.');
		}
	}
	uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, HGRN " OK!\r\n");
	asm(".word 0x500F");
	consoleLog(HCYN, "1stBoot", "Start verify SD Image!\r\n");
	consoleLog(HWHT, "1stBoot", "Progress: ");
	unsigned long checksum_calc = 0;
	unsigned char *checksum_start = (unsigned char *)(bootmem + 0x20);
	unsigned long cnt = 0;
	for(unsigned long i = 0; i < image_length; i ++)
	{
		checksum_calc += checksum_start[i];
		cnt += 1;
		if(cnt > 511) {
			cnt = 0;
			uart_write((Uart_Reg *)UART_CONFIG_BASE, '.');
		}
	}
	if(checksum_calc != checksum_get)
	{
		uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, HRED " FAIL!\r\n");
		consoleLog(HRED, "1stBoot", "CheckSum failed! Excepted: 0x");
		printHex32(checksum_get);
		uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, ", but calculated: 0x");
		printHex32(checksum_calc);
		uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, "!\r\n");
		while(1);
	}
	uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, HGRN " OK!\r\n");
	
	if(jumptoentry)
	{
		void (*payload)(void) = entryaddr;
		consoleLog(HGRN, "1stBoot", "Jumping to user entrypoint 0x");
		printHex32(entryaddr);
		uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, "!\r\n\r\n" COLOR_RESET);
		__asm__("fence.i\n\t");
		payload();
		consoleLog(HRED, "1stBoot", "Failed jumping to user code? This will never happen\r\n");
		while(1);
	}
}

void sd_boot_sbi()
{
	consoleLog(HMAG, "1stBoot", "Read second stage bootloader payload from SD\r\n");
	sd_boot(0x40000000, 4096, 0); // 4096 => 0x200000 => Second Stage Bootloader
	consoleLog(HMAG, "1stBoot", "Read & booting RISC-V SBI from SD\r\n");
	sd_boot(0x41F00000 - 0x20, 12288, 1); // 12288 => 0x600000 => SBI fw_jump
}

void cpu0_entry_start()
{	
	consoleLog(HCYN, "Bootloader", "** Welcome to Paski SOC On-Chip Bootloader! **\r\n");
	consoleLog(HCYN, "Bootloader", "Author: Angelic47\r\n\r\n");
	
	//sdram_memscan();
	sd_boot_sbi();
}
