#include "stmflash/stm_flash.h"



static uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;
  if(Address < ADDR_FLASH_SECTOR_1)sector = FLASH_SECTOR_0;  
  else if(Address < ADDR_FLASH_SECTOR_2)sector = FLASH_SECTOR_1;  
  else if(Address < ADDR_FLASH_SECTOR_3)sector = FLASH_SECTOR_2;  
  else if(Address < ADDR_FLASH_SECTOR_4)sector = FLASH_SECTOR_3;  
  else if(Address < ADDR_FLASH_SECTOR_5)sector = FLASH_SECTOR_4;  
  else if(Address < ADDR_FLASH_SECTOR_6)sector = FLASH_SECTOR_5;  
  else if(Address < ADDR_FLASH_SECTOR_7)sector = FLASH_SECTOR_6;  
  else if(Address < ADDR_FLASH_SECTOR_8)sector = FLASH_SECTOR_7;  
  else if(Address < ADDR_FLASH_SECTOR_9)sector = FLASH_SECTOR_8;  
  else if(Address < ADDR_FLASH_SECTOR_10)sector = FLASH_SECTOR_9;  
  else if(Address < ADDR_FLASH_SECTOR_11) sector = FLASH_SECTOR_10;  
  else sector = FLASH_SECTOR_11;  
  return sector;
}



void STMFLASH_Write_NoCheck ( uint32_t WriteAddr, uint16_t * pBuffer, uint16_t NumToWrite )   
{ 			 		 
	uint16_t i;	
	for(i=0;i<NumToWrite;i++)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,WriteAddr,pBuffer[i]);
	  WriteAddr+=2; 
	}  
} 

		
void STMFLASH_Write(uint32_t WriteAddr,uint32_t *pBuffer,uint32_t NumToWrite)	
{ 
	FLASH_EraseInitTypeDef FlashEraseInit;
	HAL_StatusTypeDef FlashStatus=HAL_OK;
	uint32_t SectorError=0;
	uint32_t addrx=0;
	uint32_t endaddr=0;	
	if(WriteAddr<0x08000000||WriteAddr%4)return;	//非法地址
		
	HAL_FLASH_Unlock();             //解锁	
	addrx=WriteAddr;				//写入的起始地址
	endaddr=WriteAddr+NumToWrite*4;	//写入的结束地址
		
	if(addrx<0X1FFF0000)
	{
		while(addrx<endaddr)		//扫清一切障碍.(对非FFFFFFFF的地方,先擦除)
		{
			if (*(volatile uint32_t *)addrx!=0XFFFFFFFF)		//	 if(STMFLASH_ReadWord(addrx)!=0XFFFFFFFF)
			{   
				FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //擦除类型，扇区擦除 
				FlashEraseInit.Sector=GetSector(addrx);   //要擦除的扇区
				FlashEraseInit.NbSectors=1;                             //一次只擦除一个扇区
				FlashEraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;      //电压范围，VCC=2.7~3.6V之间!!
				if(HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError)!=HAL_OK) 
				{
					break;//发生错误了	
				}
				}else addrx+=4;
				//FLASH_WaitForLastOperation(FLASH_WAITETIME);                //等待上次操作完成
		}
	}
	//FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);            //等待上次操作完成
	if(FlashStatus==HAL_OK)
	{
		 while(WriteAddr<endaddr)//写数据
		 {
			if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,WriteAddr,*pBuffer)!=HAL_OK)//写入数据
			{ 
				break;	//写入异常
			}
			WriteAddr+=4;
			pBuffer++;
		}  
	}
	HAL_FLASH_Lock();           //上锁
} 
		



