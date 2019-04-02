/**
  ******************************************************************************
  * �ļ�����: diskio.c 
  * ��    ��: ӲʯǶ��ʽ�����Ŷ�
  * ��    ��: V1.0
  * ��д����: 2017-03-30
  * ��    ��: FatFS�ļ�ϵͳ�洢�豸��������ӿ�ʵ��
  ******************************************************************************
  * ˵����
  * ����������Ӳʯstm32������YS-F4Proʹ�á�
  * 
  * �Ա���
  * ��̳��http://www.ing10bbs.com
  * ��Ȩ��ӲʯǶ��ʽ�����Ŷ����У��������á�
  ******************************************************************************
  */

/* ����ͷ�ļ� ----------------------------------------------------------------*/
#include "diskio.h"
#include "ff_gen_drv.h"
#include "ff.h"

#if _USE_LFN != 0   // ���ʹ�ܳ��ļ����������ؽ����ļ�

#if _CODE_PAGE == 936	/* �������ģ�GBK */
#include "option\cc936.c"
#elif _CODE_PAGE == 950	/* �������ģ�Big5 */
#include "option\cc950.c"
#else					/* Single Byte Character-Set */
#include "option\ccsbcs.c"
#endif

#endif

/* ˽�����Ͷ��� --------------------------------------------------------------*/
/* ˽�к궨�� ----------------------------------------------------------------*/
/* ˽�б��� ------------------------------------------------------------------*/
/* ��չ���� ------------------------------------------------------------------*/
extern Disk_drvTypeDef  disk;

/* ˽�к���ԭ�� --------------------------------------------------------------*/
/* ������ --------------------------------------------------------------------*/
/**
  * ��������: ��ȡ�����豸״̬ 
  * �������: pdrv�������豸���
  * �� �� ֵ: DSTATUS���������
  * ˵    ��: ��
  */
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
  DSTATUS stat;
  
  stat = disk.drv[pdrv]->disk_status(disk.lun[pdrv]);
  return stat;
}

/**
  * ��������: ��ʼ�������豸
  * �������: pdrv�������豸���
  * �� �� ֵ: DSTATUS���������
  * ˵    ��: ��
  */
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

/**
  * ��������: �������豸��ȡ���ݵ�������
  * �������: pdrv�������豸���
  *           buff����Ŵ�д�����ݵĻ�����ָ��
  *           sector��������ַ(LBA)
  *           count��������Ŀ(1..128)
  * �� �� ֵ: DSTATUS���������
  * ˵    ��: ��
  */
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

/**
  * ��������: ������������д�뵽�����豸��
  * �������: pdrv�������豸���
  *           buff����Ŵ�д�����ݵĻ�����ָ��
  *           sector��������ַ(LBA)
  *           count��������Ŀ
  * �� �� ֵ: DSTATUS���������
  * ˵    ��: SD��д����û��ʹ��DMA����
  */
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

/**
  * ��������: ����������Ʋ���(I/O control operation)
  * �������: pdrv�������豸���
  *           cmd����������
  *           buff����Ŵ�д����߶�ȡ���ݵĻ�����ָ��
  * �� �� ֵ: DSTATUS���������
  * ˵    ��: ��
  */
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

/**
  * ��������: ��ȡʵʱʱ��
  * �������: ��
  * �� �� ֵ: ʵʱʱ��(DWORD)
  * ˵    ��: ��
  */
__weak DWORD get_fattime (void)
{
  return 0;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

