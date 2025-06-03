#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <time.h>
#include <iio.h>
#include "mcp3002.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static void pabort(const char *s)
{
	perror(s);
	//abort();
}


/***************************************************************************//**
 * @brief Reads the output value of an MCP3002 on a selected channel.
 *
 * @param data - channel to read. 0, 1 (or 2 for differential +ve, 3 for -ve)
 *
 * @return 0 - 1023 adc output
*******************************************************************************/
uint32_t mcp3002_value(uint32_t channel)
{
  int ret = 0;
  int fd;

  static const char *device = "/dev/spidev0.0";
  static uint32_t mode = 0;
  static uint8_t bits = 8;
  static uint32_t speed = 2000000;  // Probably the maximum.  Originally 50000
  static uint16_t delay = 5;

  fd = open(device, O_RDWR);
  if (fd < 0)
  {
    pabort("can't open device");
  }

  // Original code to set spi mode.  Not used

  //ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
  //if (ret == -1)
  //{
  //  pabort("can't set spi mode");
  //}

  //ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
  //if (ret == -1)
  //{
  //  pabort("can't get spi mode");

  //  Set the bits per word.  Write then read to check

  ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
  if (ret == -1)
  {
    pabort("can't set bits per word");
  }

//  ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
//  if (ret == -1)
//  {
//    pabort("can't get bits per word");
//  }

  // Set max speed hz, then read to check

  ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
  if (ret == -1)
  {
    pabort("can't set max speed hz");
  }

//  ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
//  if (ret == -1)
//  {
//    pabort("can't get max speed hz");
//  }

  //printf("spi mode: 0x%x\n", mode);
  //printf("bits per word: %d\n", bits);
  //printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

  // Set the input configuration
  uint8_t tx[] = {0xC0, 0x00, };

  switch(channel)
  {
    case 0:
      tx[0] = 0xC0;
      break;
    case 1:
      tx[0] = 0xE0;
      break;
    case 2:
      tx[0] = 0x80;
      break;
    case 3:
      tx[0] = 0x40;
      break;
  }

  uint8_t rx[ARRAY_SIZE(tx)] = {0, };

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = ARRAY_SIZE(tx),
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	if (mode & SPI_TX_QUAD)
		tr.tx_nbits = 4;
	else if (mode & SPI_TX_DUAL)
		tr.tx_nbits = 2;
	if (mode & SPI_RX_QUAD)
		tr.rx_nbits = 4;
	else if (mode & SPI_RX_DUAL)
		tr.rx_nbits = 2;

  if (!(mode & SPI_LOOP))
  {
    if (mode & (SPI_TX_QUAD | SPI_TX_DUAL))
    {
      tr.rx_buf = 0;
    }
    else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL))
    {
      tr.tx_buf = 0;
    }
  }

  ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
  if (ret < 1)
  {
    pabort("can't send spi message");
  }

  //printf("%.2X \n", rx[0]);
  //printf("%.2X \n", rx[1]);

  uint32_t voltage;
  voltage = rx[1] / 2 + 128 * rx[0];

//  if (channel == 0)
//  {
//    printf("\n%d", voltage);
//  }
//  else
//  {
 //   printf(" %d ", voltage);
 // }

  close(fd);

  return voltage;

}

