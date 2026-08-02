#ifndef SPI_H
#define SPI_H

#include <stdint.h>

void spi_config(void);
uint8_t spi_transfer(uint8_t tx);
void cs_low(void);
void cs_high(void); 

#endif
