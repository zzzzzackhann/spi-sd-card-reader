#include "registers.h"
#include <stdint.h>

/* SPI1 configured to mode 0 (CPOL = 0 and CPHA = 0), with PA4 acting as software-controlled CS*/
void spi_config(void) {
    GPIOA_MODER &= ~((3 << 8) | (3 << 10) | (3 << 12) | (3 << 14)); // clear bits at PA4, PA5, PA6, PA7
    GPIOA_MODER |= (1 << 8) | (2 << 10) | (2 << 12) | (2 << 14); // set PA4 to output mode, PA5, PA76, PA7 to AF mode
    GPIOA_ODR |= (1 << 4); // set PA4 high (SPIidle)
    GPIOA_AFRL &= ~((15 << 20) | (15 << 24) | (15 << 28)); // clear AF bits at PA5, PA6, PA7
    GPIOA_AFRL |= (5 << 20) | (5 << 24) | (5 << 28); // set PA5, PA6, PA7 to AF5 (SPI1)
    SPI_CR1 &= ~(7 << 3); // clear bits 5:3 (baud rate)
    /* set device as master, baud raute to AHB2_CLK/64, enable software slave management
    and set to 1 (high, not-enabled), leave CPOL and CPHA at 0 for mode 0, 
    leave LSBFIRST (least significant byte) at 0 (SD cards want MSB first), and finally enable SPI*/
    SPI_CR1 |= (1 << 2) | (5 << 3) | (1 << 9) | (1 << 8) |  (1 << 6);
}

uint8_t spi_transfer(uint8_t tx) {
    while(!(SPI_SR & (1 << 1))); // wait until TXE (transmit buffer empty) flag is set
    SPI_DR = tx; // write tx to SPI_DR
    while(!(SPI_SR & 1)); // wait until receive buffer NOT empty
    
    return SPI_DR; // read receive buffer to give received byte 
}

void cs_low(void) { GPIOA_ODR &= ~(1 << 4); } // select SPI device (CS low)
void cs_high(void) { GPIOA_ODR |= (1 << 4); } // deselect SPI device (CS high)

