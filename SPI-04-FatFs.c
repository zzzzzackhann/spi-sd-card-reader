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

    while(1){
        
    }
}
