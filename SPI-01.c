#include "registers.h"
#include "uart.h"

void clock_enable(void) {
    RCC_APB1ENR |= (1 << 17); // enable USART 2 clock
    RCC_AHB1ENR |= 1; // enable GPIOA clock
    RCC_APB2ENR |= (1 << 12); // enable SPI1 clock
}

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

uint8_t sd_command(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t *resp, uint8_t resp_len) {
    cs_low();
    spi_transfer(0x40 | cmd); // masking with "or" to add command bits to start bit
    /* Mask each byte of the 32 byte argument and format to MSB first*/
    spi_transfer(arg >> 24);
    spi_transfer((arg >> 16) & 0xFF); // mask to only read the necessary bits
    spi_transfer((arg >> 8) & 0xFF);
    spi_transfer (arg & 0xFF);
    spi_transfer(crc);

    uint16_t timeout = 1000;

    /* Poll spi_transfer untill you get a resp byte whose MSB is 0 (NOT 1). This will poll 1000
    before timing out. If it timesout the function will return a byte with 0xFF, which you can 
    read as an invalid resp. */
    do {
        resp[0] = spi_transfer(0xFF); // set garbage byte to get first response from SD card
    } while ((resp[0] & 0x80) && --timeout);

    for (uint8_t i = 1; i < resp_len; i++) {
        resp[i] = spi_transfer(0xFF);
    }

    cs_high();
    return resp[0];
}

int main(void) {
    clock_enable();
    spi_config();
    usart_init();
    
    /* with CS high, send 10 bytes (80 clock ticks) to wake SD card */
    cs_high();
    for(uint8_t i = 0; i < 10; i++) {
        spi_transfer(0xFF);
    }

    /* There is some complicated algorithm for calculating the CRC value, but it DOESN'T MATTER for all but two commands.
    Those are CMD0 (CRC = 0x95) and CMD8 (CRC = 0x87). After that the CRC is arbitrary so we use 0xFF. You certainly can 
    implement the algorithm in the program if you wanted, but it's not necessary since the value is ignored other than 
    for those two commands. */

    /* A 0x01 is a success ("idle"). 0xFF would be a failure to initialize (an "invalid resp" that indicates the initial handshake failed) */

    uint8_t r1[1]; // response type 1 (CMD0) returns 1 byte. this is the reset command. 
    sd_command(0, 0, 0x95, r1, 1); 
    // usart_send_num(r1[0]);
    // usart_send_string("\n\r");

    /* The second essential command, with CRC constant 0x87. 0x01 in arg is a voltage code. Second arg is a "check pattern"
    to see if SD card is v1 or v2. The resp has 5 bytes, so you need to send argument and then poll SD card to receive next 4 bytes. First should indicate idle (0x01), last two should echo 
    argument's low bytes, 0x01 and 0xAA to confirm arguments.   */
    
    uint8_t r7[5]; // response type 7 (constant for CMD8) returns 5-bytes 
    sd_command(8, 0x000001AA, 0x87, r7, 5);
    // usart_send_num(r7[0]); usart_send_char(' ');
    // usart_send_num(r7[1]); usart_send_char(' ');
    // usart_send_num(r7[2]); usart_send_char(' ');
    // usart_send_num(r7[3]); usart_send_char(' ');
    // usart_send_num(r7[4]); usart_send_string("\n\r");

    /* ACMD41 (CMD41) MUST be send after CMD55, which tell in "application command coming next."
    ACMD41 (application command 41) sets HCS (host capacity support) bit, telling card that chip
    can handle high-capacity SD cards (SDHCs). */

    uint16_t timeout = 1000;

    do {
        sd_command(55, 0, 0x01, r1, 1); 
        sd_command(41, 0x40000000, 0x01, r1, 1);
    } while (r1[0] != 0x00 && --timeout); // wait until ACMD41 returns 0x00, which means it is done initializing

    while(1){}
}
