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
	if(WriteAddr<0x08000000||WriteAddr%4)return;	//�Ƿ���ַ
		
	HAL_FLASH_Unlock();             //����	
	addrx=WriteAddr;				//д�����ʼ��ַ
	endaddr=WriteAddr+NumToWrite*4;	//д��Ľ�����ַ
		
	if(addrx<0X1FFF0000)
	{
		while(addrx<endaddr)		//ɨ��һ���ϰ�.(�Է�FFFFFFFF�ĵط�,�Ȳ���)
		{
			if (*(volatile uint32_t *)addrx!=0XFFFFFFFF)		//	 if(STMFLASH_ReadWord(addrx)!=0XFFFFFFFF)
			{   
				FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;       //�������ͣ��������� 
				FlashEraseInit.Sector=GetSector(addrx);   //Ҫ����������
				FlashEraseInit.NbSectors=1;                             //һ��ֻ����һ������
				FlashEraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;      //��ѹ��Χ��VCC=2.7~3.6V֮��!!
				if(HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError)!=HAL_OK) 
				{
					break;//����������	
				}
				}else addrx+=4;
				//FLASH_WaitForLastOperation(FLASH_WAITETIME);                //�ȴ��ϴβ������
		}
	}
	//FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);            //�ȴ��ϴβ������
	if(FlashStatus==HAL_OK)
	{
		 while(WriteAddr<endaddr)//д����
		 {
			if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,WriteAddr,*pBuffer)!=HAL_OK)//д������
			{ 
				break;	//д���쳣
			}
			WriteAddr+=4;
			pBuffer++;
		}  
	}
	HAL_FLASH_Lock();           //����
} 
		



