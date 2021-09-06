#include "sd.h"
#include "uart.h"
#include "logging.h"
#include "color.h"

static unsigned int sd_spi_get_tx_fifo_left()
{
	unsigned long status = SD_REG_READ4(SD_STATUS);
	return (status >> 16) & 0xffff;
}

static unsigned char sd_spi_read_fifo_blocked()
{
	unsigned long result = SD_REG_READ4(SD_DATA);
	while((result & 0x80000000) == 0)
		result = SD_REG_READ4(SD_DATA);
	return result & 0xff;
}

static void sd_spi_wait_fifo()
{
	while(sd_spi_get_tx_fifo_left() == 0);
}

static void sd_spi_wait_fifo_empty()
{
	while(sd_spi_get_tx_fifo_left() != 0x20);
}

static void sd_spi_send_cmd(unsigned long cmd)
{
	sd_spi_wait_fifo();
	SD_REG_WRITE4(SD_DATA, cmd);
}

static void sd_spi_enable_ss()
{
	sd_spi_send_cmd(0x11000000);
}

static void sd_spi_disable_ss()
{
	sd_spi_send_cmd(0x10000000);
}

static void sd_spi_send_data(unsigned char data)
{
	sd_spi_send_cmd(0x00000000 | data);
}

static void sd_spi_push_read_fifo(unsigned char sendch)
{
	sd_spi_send_cmd(0x01000000 | sendch);
}

static void sd_spi_set_config(unsigned long config)
{
	SD_REG_WRITE4(SD_CONFIG, config);
}

static void sd_spi_set_clkdiv(unsigned long clkdiv)
{
	SD_REG_WRITE4(SD_CLKDIVIDER, clkdiv);
}

static void sd_spi_set_ss_setup(unsigned long setup)
{
	SD_REG_WRITE4(SD_SS_SETUP, setup);
}

static void sd_spi_set_ss_hold(unsigned long hold)
{
	SD_REG_WRITE4(SD_SS_HOLD, hold);
}

static void sd_spi_set_ss_disable(unsigned long disable)
{
	SD_REG_WRITE4(SD_SS_DISABLE, disable);
}

unsigned char sd_send_command_nodessart(unsigned char cmd, unsigned long arg, unsigned char crc)
{
	unsigned char r1;
	unsigned int Retry = 0;
	sd_spi_wait_fifo_empty();
	sd_spi_disable_ss();
	//发送8个时钟，提高兼容性
	sd_spi_send_data(0xff);	
	sd_spi_wait_fifo_empty();
	//选中SD卡
	sd_spi_enable_ss();		
	/*按照SD卡的命令序列开始发送命令 */
	//cmd参数的第二位为传输位，数值为1，所以或0x40  
	sd_spi_send_data(cmd | 0x40);    
	//参数段第24-31位数据[31..24]
	sd_spi_send_data((unsigned char)(arg >> 24));
	//参数段第16-23位数据[23..16]	
	sd_spi_send_data((unsigned char)(arg >> 16));
	//参数段第8-15位数据[15..8]	
	sd_spi_send_data((unsigned char)(arg >> 8));	
	//参数段第0-7位数据[7..0]
	sd_spi_send_data((unsigned char)arg);    
	sd_spi_send_data(crc);
	//等待响应或超时退出
	do
	{
		if(Retry > 800) break; 	//超时次数
		Retry++;
		sd_spi_push_read_fifo(0xff);
	}
	while((r1 = sd_spi_read_fifo_blocked()) == 0xFF);
	//返回状态值	
	return r1;		
}

unsigned char sd_send_command(unsigned char cmd, unsigned long arg, unsigned char crc)
{
	char r1 = sd_send_command_nodessart(cmd, arg, crc);
	//关闭片选
	sd_spi_disable_ss();		
	//在总线上额外发送8个时钟，让SD卡完成剩下的工作
	sd_spi_send_data(0xFF);
	sd_spi_wait_fifo_empty();
	return r1;
}

static void sd_spi_lowlevel_init()
{
	sd_spi_set_config(0);
	sd_spi_set_clkdiv(100);
	sd_spi_set_ss_setup(2);
	sd_spi_set_ss_hold(2);
	sd_spi_set_ss_disable(0);
}

int sd_init()
{
	char buff[4];
	consoleLog(HMAG, "SDCard", "SDCard init start!\r\n");
	sd_spi_lowlevel_init();
	sd_spi_enable_ss();
	// Wait for sdcard init
	for(int i=0;i<10;i++)
		sd_spi_send_data(0xff);
	int retry;
	int r1;
	retry = 0;
	do
	{
		// CMD0 and CRC=0x95
		r1 = sd_send_command(SD_CMD0, 0, 0x95);		
		retry++;
	}
	while((r1 != 0x01) && (retry < 200));
	if(retry == 200)
	{
		consoleLog(HRED, "SDCard", "Init failed, CMD0 no response!\r\n");
		return -SD_ERR_TIMEOUT;
	}
	// 获取SD卡版本信息
	r1 = sd_send_command_nodessart(SD_CMD8, 0x1aa, 0x87);
	//下面是SD2.0卡的初始化		
	if(r1 == 0x01)	
	{
		// V2.0的卡，CMD8命令后会传回4字节的数据，要跳过再结束本命令
		sd_spi_push_read_fifo(0xff);
		sd_spi_push_read_fifo(0xff);
		sd_spi_push_read_fifo(0xff);
		sd_spi_push_read_fifo(0xff);
		buff[0] = sd_spi_read_fifo_blocked();  	
		buff[1] = sd_spi_read_fifo_blocked();  	
		buff[2] = sd_spi_read_fifo_blocked();  	
		buff[3] = sd_spi_read_fifo_blocked();  		    
		sd_spi_disable_ss();
		//多发8个时钟	  
		sd_spi_send_data(0xFF);		
		sd_spi_wait_fifo_empty();
		retry = 0;
		//发卡初始化指令CMD55+ACMD41
		do
		{
			r1 = sd_send_command(SD_CMD55, 0, 0);		
			//应返回0x01
			if(r1 != 0x01)		
			{
				consoleLog(HRED, "SDCard", "Init failed, CMD55 unexcepted response!\r\n");
				return -SD_ERR_ILLEGAL_RESPONSE;
			}   
			r1 = sd_send_command(SD_ACMD41, 0x40000000, 1);	
			retry++;
			if(retry>200)	
			{
				consoleLog(HRED, "SDCard", "Init failed, ACMD41 return 0x");
				printHex32(r1);
				uart_writeStr((Uart_Reg *)UART_CONFIG_BASE, "!\r\n");
				return -SD_ERR_ILLEGAL_RESPONSE;
			}
		}
		while(r1 != 0);
		//----------鉴别SD2.0卡版本开始-----------
		//读OCR指令
		r1 = sd_send_command_nodessart(SD_CMD58, 0, 0);		
		//如果命令没有返回正确应答，直接退出，返回应答
		if(r1 != 0x00)  
		{
			sd_spi_disable_ss();
			consoleLog(HRED, "SDCard", "Init failed, CMD58 unexcepted response!\r\n");
			return -SD_ERR_ILLEGAL_RESPONSE;
		}   		 
		//应答正确后，会回传4字节OCR信息
		sd_spi_push_read_fifo(0xff);
		sd_spi_push_read_fifo(0xff);
		sd_spi_push_read_fifo(0xff);
		sd_spi_push_read_fifo(0xff);
		buff[0] = sd_spi_read_fifo_blocked();  	
		buff[1] = sd_spi_read_fifo_blocked();  	
		buff[2] = sd_spi_read_fifo_blocked();  	
		buff[3] = sd_spi_read_fifo_blocked();  	
		//OCR接收完成，片选置高
		sd_spi_disable_ss();
		//多发8个时钟	  
		sd_spi_send_data(0xFF);
		sd_spi_wait_fifo_empty();
		//检查接收到的OCR中的bit30位（CSS），确定其为SD2.0还是SDHC
		//CCS=1：SDHC   CCS=0：SD2.0
		if(buff[0]&0x40)
		{
			consoleLog(HCYN, "SDCard", "SDCard is V2HC\r\n");
		}   	 
		else		
		{
			consoleLog(HCYN, "SDCard", "SDCard is V2\r\n");
		}	    
		//-----------鉴别SD2.0卡版本结束----------- 
		sd_spi_set_clkdiv(1); 		//设置SPI为高速模式
	}
	else
	{
		sd_spi_disable_ss();
		//多发8个时钟	  
		sd_spi_send_data(0xFF);		
		sd_spi_wait_fifo_empty();
		consoleLog(HCYN, "SDCard", "SDCard is V1\r\n");
	}
	//初始化指令发送完成，接下来获取OCR信息
	consoleLog(HGRN, "SDCard", "SDCard init OK!\r\n");
	return SD_ERR_SUCCESS;
}

int sd_spi_recvdata(char *buf, unsigned long len)
{
	unsigned long count = 0xF000; //等待次数
	sd_spi_enable_ss();
	while(count)
	{
		sd_spi_push_read_fifo(0xff);
		if(sd_spi_read_fifo_blocked() == 0xFE)
			break;
		count--;
	}
	if(count == 0)
	{
		sd_spi_disable_ss();
		return -SD_ERR_ILLEGAL_RESPONSE;
	}
	while(len--) //开始接收数据
	{
		sd_spi_push_read_fifo(0xff);
		*buf = sd_spi_read_fifo_blocked();
		buf++;
	}
	//下面是2个伪CRC（dummy CRC），假装接收了2个CRC
	sd_spi_send_data(0xFF);
	sd_spi_send_data(0xFF);
	sd_spi_wait_fifo_empty();
	sd_spi_disable_ss();
	return 0;//读取成功
}

int sd_read_data(void *buffer, unsigned long sector, unsigned long count)
{
	if(sd_send_command(SD_CMD18, sector, 0X01) != 0x00)
		return -SD_ERR_ILLEGAL_RESPONSE;
	char *buf = (char *)buffer;
	int r1;
	do
	{
		r1 = sd_spi_recvdata(buf, 512);
		buf += 512;
		count -= 1;
	}
	while(count && r1 == 0);
	sd_send_command(SD_CMD12, 0, 0X01);
	
	return SD_ERR_SUCCESS;
}

