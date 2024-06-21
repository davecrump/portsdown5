#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

//static char *spiDevice = "/dev/spidev0.1";
static char *spiDevice = "/dev/spidev1.1";     // bus 1, device 1 (SR)?
static uint8_t spiBPW = 8;
static uint32_t spiSpeed = 5000000;
static uint16_t spiDelay = 0;
uint8_t SR0;                                   // LED switching word
uint8_t SR1;                                   // LNA, PA and fan switching word

/*
SR0 (LEDs):

0 - RPI_LED1_R (Active Low)
1 - RPI_LED1_G (Active Low)
2 - RPI_LED2_R (Active Low)
3 - RPI_LED2_G (Active Low)
4 - RPI_LED3_R (Active Low)
5 - RPI_LED3_G (Active Low)
6 - RPI_LED4_R (Active Low)
7 - RPI_LED4_G (Active Low)

SR1 (PAs, LNAs, FAN):

0 - EN_TXA_PA (Active High)
1 - EN_RXA_LNA (Active High)
2 - EN_TXB_PA (Active High)
3 - EN_RXB_LNA (Active High)
4 - (Not connected)
5 - (Not connected)
6 - (Not connected)
7 - FAN_CTRL (Active High. !!!Always keep at high!!!)

SR0 = 0b01111111 = 127
SR1 = 0b10000011 = 131

*/

uint8_t SR0 = 127;                                   // LED switching word
//uint8_t SR1 = 0;                                   // LNA, PA and fan switching word (all off)
uint8_t SR1 = 131;                                   // LNA, PA and fan switching word (fan and LNA1, PA1 on)

char spi_fd;

int spiOpen(char* dev)
{
  if((spi_fd = open(dev, O_RDWR)) < 0)
  {
    printf("error opening %s\n",dev);
    return -1;
  }
  return 0;
}


void writeLMNBytes(uint8_t leds, uint8_t amps)
{
  uint8_t spiBufTx [2];
  uint8_t spiBufRx [2];
  struct spi_ioc_transfer spi;
  memset (&spi, 0, sizeof(spi));
  spiBufTx [0] = leds;
  spiBufTx [1] = amps;
  spi.tx_buf =(unsigned long)spiBufTx;
  spi.rx_buf =(unsigned long)spiBufRx;
  spi.len = 2;
  spi.delay_usecs = spiDelay;
  spi.speed_hz = spiSpeed;
  spi.bits_per_word = spiBPW;
  ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi);
}


void LMNspi()
{
  spiOpen(spiDevice);

  writeLMNBytes(SR1, SR0);
}

