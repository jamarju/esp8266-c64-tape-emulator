#ifndef _I2S_FREERTOS_H_
#define _I2S_FREERTOS_H_

//Parameters for the I2S DMA behaviour
#define I2SDMABUFCNT (4)			//Number of buffers in the I2S circular buffer
#define I2SDMABUFLEN (64)		    //Length of one buffer, in 32-bit words.

// Total memory used = I2SDMABUFCNT * I2SDMABUFLEN * 4 bytes


void ICACHE_FLASH_ATTR i2sInit();
void i2sSetRate(int rate);
void i2sPushSample(unsigned int sample);
long ICACHE_FLASH_ATTR i2sGetUnderrunCnt();


#endif