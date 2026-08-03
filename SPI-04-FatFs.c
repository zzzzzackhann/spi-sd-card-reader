#include "registers.h"
#include "uart.h"
#include "spi.h"
#include "sd_card.h"
#include "ff.h"

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

    FATFS fs; // filesystem work area - holds mount state
    FIL file; // file object - holds an open file's state
    UINT bytes_written;
    FRESULT res; //FatFs functions return this status enum

    /* 1. Mount the filesystem */
    res = f_mount(&fs, "", 1); // "" = default drive; 1 = mount now
    if (res!= FR_OK) {
        usart_send_string("Mount failed: ");
        usart_send_num(res);
        usart_send_string("\n\r");
        while(1){}
    }

    /* 2 Open (create) a file for writing */
    res = f_open(&file, "hello.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if (res!= FR_OK) {
        usart_send_string("Open failed: ");
        usart_send_num(res);
        usart_send_string("\n\r");
        while(1){}
    }

    /* 3. Write to it */
    const char *msg = "Yooooo this is bare-metal STM32 speaking!!! \n\r";
    res = f_write(&file, msg, 30, &bytes_written);

    /* 4. Close it (flushes to card) */
    f_close(&file);

    usart_send_string("Donnnneee these bytes are written: ");
    usart_send_num(bytes_written);
    usart_send_string("\n\r");

    while(1){}
}
