#ifndef __BL_HAL_SD__
#define __BL_HAL_SD__

#define SD_SPI_BASE 0xF0002000
#define SD_REG_WRITE4(x,y) *((volatile unsigned long *)(x)) = (y)
#define SD_REG_READ4(x) (*((volatile unsigned long *)(x)))

#define SPI_REG_DATA 0x00
#define SPI_REG_STATUS 0x04
#define SPI_REG_CONFIG 0x08
#define SPI_REG_CLKDIVIDER 0x0C
#define SPI_REG_SS_SETUP 0x10
#define SPI_REG_SS_HOLD 0x14
#define SPI_REG_SS_DISABLE 0x18

#define SD_DATA (SD_SPI_BASE + 0x00)
#define SD_STATUS (SD_SPI_BASE + 0x04)
#define SD_CONFIG (SD_SPI_BASE + 0x08)
#define SD_CLKDIVIDER (SD_SPI_BASE + 0x0C)
#define SD_SS_SETUP (SD_SPI_BASE + 0x10)
#define SD_SS_HOLD (SD_SPI_BASE + 0x14)
#define SD_SS_DISABLE (SD_SPI_BASE + 0x18)

#define SD_CMD0 0x40
#define SD_CMD1 0x41
#define SD_CMD8 0x48
#define SD_CMD12 0x4C
#define SD_CMD18 0x52
#define SD_CMD55 0x77
#define SD_CMD58 0x7a
#define SD_ACMD41 0x69

#define SD_ERR_SUCCESS 0
#define SD_ERR_TIMEOUT 1
#define SD_ERR_ILLEGAL_RESPONSE 2

int sd_init();
int sd_read_data(void *buffer, unsigned long sector, unsigned long count);

#endif
