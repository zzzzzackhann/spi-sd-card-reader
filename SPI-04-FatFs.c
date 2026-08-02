#include "registers.h"
#include "uart.h"
#include "spi.h"
#include "sd_card.h"
#include "ff.h"
#include "diskio.h"


#define DEBUG 0

void clock_enable(void) {
    RCC_APB1ENR |= (1 << 17); // enable USART 2 clock
    RCC_AHB1ENR |= 1; // enable GPIOA clock
    RCC_APB2ENR |= (1 << 12); // enable SPI1 clock
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
