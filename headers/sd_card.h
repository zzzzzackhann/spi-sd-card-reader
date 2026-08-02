#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdint.h> 

uint8_t sd_command(uint8_t cmd, uint32_t arg, uint8_t crc, uint8_t *resp, uint8_t resp_len, uint8_t hold_cs);
uint8_t sd_init(void);
uint8_t sd_read_block(uint32_t block_addr, uint8_t *buffer);
uint8_t sd_write_block(uint32_t block_addr, uint8_t *buffer);




#endif
