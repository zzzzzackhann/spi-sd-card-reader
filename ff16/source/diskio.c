/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"		/* Basic definitions of FatFs */	
#include "diskio.h"		/* Declarations FatFs MAI */
#include "sd_card.h"


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
/* pdrv is drive number if multiple storage drives */
DSTATUS disk_status(BYTE pdrv) { 
	return 0; /* assume ready once initialized */
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
/* A more robust version would check sd_init()'s return value */
DSTATUS disk_initialize (BYTE pdrv) {
	sd_init(); 
	return 0; // 0 = OK
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	for (UINT i = 0; i < count; i++) {
		if (sd_read_block(sector + i, buff + (i * 512)) != 0) 
		return RES_ERROR;
	}
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	for (UINT i = 0; i < count; i++) {
		if (sd_write_block(sector + i, (BYTE*)(buff + (i * 512))) != 0) {
			return RES_ERROR;
		}
	}
	return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions   	                                         */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    (void)pdrv;
    (void)cmd;
    (void)buff;
    return RES_OK;
}

DWORD get_fattime(void) {
    /* Return a fixed timestamp: 2025-01-01 00:00:00
       Format is a packed 32-bit FAT timestamp:
       bits 31:25 = year - 1980, 24:21 = month, 20:16 = day,
       15:11 = hour, 10:5 = minute, 4:0 = second/2 */
    return ((DWORD)(2025 - 1980) << 25)  /* year */
         | ((DWORD)1 << 21)              /* month */
         | ((DWORD)1 << 16)              /* day */
         | (0 << 11)                     /* hour */
         | (0 << 5)                      /* minute */
         | (0 << 0);                     /* second */
}
