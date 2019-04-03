#include "diskio.h"
#include "ff_gen_drv.h"
#include "ff.h"

#if _USE_LFN != 0 

#if _CODE_PAGE == 936
#include "option\cc936.c"
#elif _CODE_PAGE == 950	
#include "option\cc950.c"
#else	
#include "option\ccsbcs.c"
#endif

#endif

extern Disk_drvTypeDef  disk;

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
  DSTATUS stat;
  
  stat = disk.drv[pdrv]->disk_status(disk.lun[pdrv]);
  return stat;
}

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
  DSTATUS stat = RES_OK;
  
  if(disk.is_initialized[pdrv] == 0)
  { 
    disk.is_initialized[pdrv] = 1;
    stat = disk.drv[pdrv]->disk_initialize(disk.lun[pdrv]);
  }
  return stat;
}

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	        /* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
  DRESULT res;
 
  res = disk.drv[pdrv]->disk_read(disk.lun[pdrv], buff, sector, count);
  return res;
}

#if _USE_WRITE == 1
DRESULT disk_write (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count        	/* Number of sectors to write */
)
{
  DRESULT res;
  
  res = disk.drv[pdrv]->disk_write(disk.lun[pdrv], buff, sector, count);
  return res;
}
#endif /* _USE_WRITE == 1 */


#if _USE_IOCTL == 1
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  DRESULT res;

  res = disk.drv[pdrv]->disk_ioctl(disk.lun[pdrv], cmd, buff);
  return res;
}
#endif /* _USE_IOCTL == 1 */


__weak DWORD get_fattime (void)
{
  return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

