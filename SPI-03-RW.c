#include "registers.h"
#include "uart.h"

#define DEBUG 0

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

uint8_t sd_command(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t *resp, uint8_t resp_len, uint8_t hold_cs) { // if hold_cs is 1, the CS must be set high manually after data transfer 
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
    if (hold_cs == 0) cs_high();
    return resp[0];
}

uint8_t sd_init(void) {
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
    sd_command(0, 0, 0x95, r1, 1, 0); 

    #if DEBUG
        usart_send_string("CMD0: ");
        usart_send_num(r1[0]);
        usart_send_string("\n\r");
    #endif

    /* The second essential command, with CRC constant 0x87. 0x01 in arg is a voltage code. Second arg is a "check pattern"
    to see if SD card is v1 or v2. The resp has 5 bytes, so you need to send argument and then poll SD card to receive next 4 bytes. First should indicate idle (0x01), last two should echo 
    argument's low bytes, 0x01 and 0xAA to confirm arguments.   */
    
    uint8_t r7[5]; // response type 7 (constant for CMD8) returns 5-bytes 
    sd_command(8, 0x000001AA, 0x87, r7, 5, 0);

    #if DEBUG 
        usart_send_string("CMD8: ");
        usart_send_num(r7[0]); usart_send_char(' ');
        usart_send_num(r7[1]); usart_send_char(' ');
        usart_send_num(r7[2]); usart_send_char(' ');
        usart_send_num(r7[3]); usart_send_char(' ');
        usart_send_num(r7[4]); usart_send_string("\n\r");
    #endif

    /* ACMD41 (CMD41) MUST be sent after CMD55, which tell in "application command coming next."
    ACMD41 (application command 41) sets HCS (host capacity support) bit, telling card that chip
    can handle high-capacity SD cards (SDHCs). */
    uint16_t timeout = 1000;

    do {
        sd_command(55, 0, 0x01, r1, 1, 0); 
        sd_command(41, 0x40000000, 0x01, r1, 1, 0);
    } while (r1[0] != 0x00 && --timeout); // wait until ACMD41 returns 0x00, which means it is done initializing
    
    #if DEBUG
        usart_send_string("ACMD1 final (should be 0x00): ");
        usart_send_num(r1[0]);
        usart_send_string("   timeout left: ");
        usart_send_num(timeout);
        usart_send_string("\n\r");
    #endif

    cs_high();

    // /* after init at 250kHz, change baud rate so actual exchange happens much faster */    
    // SPI_CR1 |= (1 << 3); 

    return 0;
}

uint8_t sd_read_block(uint32_t block_addr, uint8_t *buffer) {
    uint8_t r1[1];
    uint16_t timeout = 1000;

    sd_command(17, block_addr, 0x01, r1, 1, 1); // READ_SINGLE_BLOCK, card responds with 0x00 byte (accepted)

    if (r1[0] != 0x00) {
        #if DEBUG
            usart_send_string("CMD17 (should be 0x00): ");
            usart_send_num(r1[0]);
            usart_send_string("\n\r");
        #endif
        return 1;
    }

    /* Wait for data-token, which will read 0xFE when the following bytes are the 512-byte data block plus 2 CRC bytes.
    Use "token" variable to avoiding calling function twice during loop. */
    timeout = 5000;
    uint8_t token = 0;
    do {token = spi_transfer(0xFF);
    } while (token != 0xFE && --timeout); 

    #if DEBUG
        usart_send_string("token (must be 0xFE): ");
        usart_send_num(token);
        usart_send_string("\n\r");
    #endif

    for (uint16_t i = 0; i < 512; i++) {
        buffer[i] = spi_transfer(0xFF); 
    }

    #if DEBUG
        usart_send_string("Signature: ");
        usart_send_num(buffer[510]);
        usart_send_char(' ');
        usart_send_num(buffer[511]);
        usart_send_string("\n\r");
    #endif

    /* Read/discard final 2 bits, which are CRC*/
    spi_transfer(0xFF); 
    spi_transfer(0xFF); 

    return 0;
}

uint8_t sd_write_block(uint32_t block_addr, uint8_t *buffer) { // return of 0 = success
        uint8_t r1[1];

        sd_command(24, block_addr, 0x01, r1, 1, 1); //WRITE_BLOCK, keep CS low...
            #if DEBUG 
                usart_send_string("CMD24 WRITE_BLOCK (should return 0x00): ");
                usart_send_num(r1[0]);
                usart_send_string("\n\r");
            #endif
        if(r1[0] != 0x00) {
            cs_high();
            return 1;
        }


        spi_transfer(0xFF); // send one dummy byte as a gap
        spi_transfer(0xFE); // send a data token. write uses 0xFE

        /* Read 512 bytes, plus the CRC bytes */
        for (uint16_t i = 0; i < 512; i++) {
            spi_transfer(buffer[i]); // send the data from the buffer to the card
        }

        spi_transfer(0xFF);
        spi_transfer(0xFF);

        /* Read data response byte and mask it with 0x1F. Response format is xxx0sss1, where bits 3:1 are status */
        r1[0] = spi_transfer(0xFF) & 0x1F;

        #if DEBUG 
            if (r1[0] == 0x05) usart_send_string("Data response byte all good.");
        #endif

        if (r1[0]!= 0x05) {
            cs_high();
            return 2;
            #if DEBUG
                if(r1[0] == 0x0B) usart_send_string("CRC Error in write function.");
                if(r1[0] == 0x0D) usart_send_string("Write error in write function.");
            #endif
        }
        
        /* The card (MISO) reads 0x00 while it's writing, so wait until that's done and then raise the CS line*/
        uint16_t timeout = 10000;
        while (spi_transfer(0xFF) == 0x00 && --timeout);

        cs_high();

        return 0;
}



int main(void) {
    clock_enable();
    spi_config();
    usart_init();
    sd_init();

    uint8_t write_buf[512];
    for (uint16_t i = 0; i < 512; i++) write_buf[i] = i & 0xFF; // pattern

    uint8_t write_validation = sd_write_block(1000, write_buf);

    uint8_t read_buf[512];
    uint8_t read_validation = sd_read_block(1000, read_buf);

   
        usart_send_string("Write validation (should be 0): ");
        usart_send_num(write_validation);
        usart_send_string("\n\r");

        usart_send_string("Read validation (should be 0): ");
        usart_send_num(read_validation);
        usart_send_string("\n\r\n\r");



    for(uint8_t i = 0; i < 10; i++) {
        usart_send_num(read_buf[i]);
        usart_send_char(' ');
    } 

    while(1){}
}
