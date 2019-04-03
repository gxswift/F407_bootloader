#include "stm32f4xx_hal.h"
#include "usbh_core.h"
#include "usbh_msc.h" 
#include "ff.h"
#include "ff_gen_drv.h"

typedef enum {
  APPLICATION_IDLE = 0,  
  APPLICATION_READY,    
  APPLICATION_DISCONNECT,
}MSC_ApplicationTypeDef;

USBH_HandleTypeDef hUSBHost;
MSC_ApplicationTypeDef Appli_state = APPLICATION_IDLE;

char USBDISKPath[4];             /* 串行Flash逻辑设备路径 */
FATFS fs;													/* FatFs文件系统对象 */
FIL file;													/* 文件对象 */
FRESULT f_res;                    /* 文件操作结果 */
UINT fnum;            					  /* 文件成功读写数量 */

extern Diskio_drvTypeDef  USBH_Driver;
extern HCD_HandleTypeDef hhcd_USB_OTG_FS;
//-------------------------------------------------------------------------------------------------------------------------
RTC_HandleTypeDef hrtc;

void MX_RTC_Init(void)
{
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
  }
}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{
  if(rtcHandle->Instance==RTC)
  {
    __HAL_RCC_RTC_ENABLE();
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{
  if(rtcHandle->Instance==RTC)
  {
    __HAL_RCC_RTC_DISABLE();
  }
} 
//-------------------------------------------------------------------------------------------------------------------------
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
 
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;       
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  HAL_RCC_EnableCSS();                      
  
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000); 
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
//-------------------------------------------------------------------------------------------------------------------------
#define FLASH_APP1_ADDR		0x08010000

void iap_load_app(uint32_t appxaddr)
{
	if(((*(volatile uint32_t*)appxaddr)&0xFF000000)==0x20000000
		|| (((*(__IO uint32_t *) appxaddr) & 0xFF000000) ==0x10000000))
	{ 
    __set_MSP(*(volatile uint32_t*)appxaddr);
		((void(*)(void))*(volatile uint32_t*)(appxaddr+4))();
	}
}		

//-------------------------------------------------------------------------------------------------------------------------
static uint32_t GetSector(uint32_t Address)
{
  if(Address < ADDR_FLASH_SECTOR_1)return FLASH_SECTOR_0;  
  else if(Address < ADDR_FLASH_SECTOR_2)return FLASH_SECTOR_1;  
  else if(Address < ADDR_FLASH_SECTOR_3)return FLASH_SECTOR_2;  
  else if(Address < ADDR_FLASH_SECTOR_4)return FLASH_SECTOR_3;  
  else if(Address < ADDR_FLASH_SECTOR_5)return FLASH_SECTOR_4;  
  else if(Address < ADDR_FLASH_SECTOR_6)return FLASH_SECTOR_5;  
  else if(Address < ADDR_FLASH_SECTOR_7)return FLASH_SECTOR_6;  
  else if(Address < ADDR_FLASH_SECTOR_8)return FLASH_SECTOR_7;  
  else if(Address < ADDR_FLASH_SECTOR_9)return FLASH_SECTOR_8;  
  else if(Address < ADDR_FLASH_SECTOR_10)return FLASH_SECTOR_9;  
  else if(Address < ADDR_FLASH_SECTOR_11)return FLASH_SECTOR_10;  
  else return FLASH_SECTOR_11;  
}
	
void STMFLASH_Write(uint32_t WriteAddr,uint32_t *pBuffer,uint32_t NumToWrite)	
{ 
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t SectorError=0;
	uint32_t addrx=0;
	uint32_t endaddr=0;	
	if(WriteAddr<0x08000000||WriteAddr%4)return;	//非法地址
		
	HAL_FLASH_Unlock();             //解锁	
	addrx=WriteAddr;				//写入的起始地址
	endaddr=WriteAddr+NumToWrite*4;	//写入的结束地址
		
	if(addrx<0X1FFF0000)
	{
		while(addrx<endaddr)
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
		}
	}

	 while(WriteAddr<endaddr)//写数据
	 {
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,WriteAddr,*pBuffer)!=HAL_OK)//写入数据
		{ 
			break;	//写入异常
		}
		WriteAddr+=4;
		pBuffer++;
	}  
	HAL_FLASH_Lock();           //上锁
} 


uint32_t iapbuf[512]; 	//2K字节缓存 
void iap_write_appbin(uint32_t appxaddr,uint8_t *appbuf,uint16_t appsize)
{
	uint32_t t;
	uint16_t i=0;
	uint32_t temp;
	uint32_t fwaddr=appxaddr;//当前写入的地址
	uint8_t *dfu=appbuf;
	for(t=0;t<appsize;t+=4)
	{						   
		temp=(uint32_t)dfu[3]<<24;   
		temp|=(uint32_t)dfu[2]<<16;    
		temp|=(uint32_t)dfu[1]<<8;
		temp|=(uint32_t)dfu[0];	  
		dfu+=4;//偏移4个字节
		iapbuf[i++]=temp;	    
		if(i==512)
		{
			i=0; 
			STMFLASH_Write(fwaddr,iapbuf,512);
			fwaddr+=2048;//偏移2048  512*4=2048
		}   
	} 
	if(i)STMFLASH_Write(fwaddr,iapbuf,i);//将最后的一些内容字节写进去.  
}
//-------------------------------------------------------------------------------------------------------------------------
UART_HandleTypeDef huart1;

void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
  }
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{
  if(uartHandle->Instance==USART1)
  {
    __HAL_RCC_USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);
  }
} 

int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 50);
  return ch;
}

void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}
//-------------------------------------------------------------------------------------------------------------------------
#define ADDLOG	1

char * Week[]={
" ",
"Monday",
"Tuesday",
"Wednesday",
"Thursday",
"Friday",
"Saturday",
"Sunday",
};

void Log()
{
	RTC_TimeTypeDef RTC_Time;
	RTC_DateTypeDef RTC_Date;

	HAL_RTC_GetTime(&hrtc,&RTC_Time,RTC_FORMAT_BIN);			
	HAL_RTC_GetDate(&hrtc,&RTC_Date,RTC_FORMAT_BIN);
	printf("20%2d-%2d-%2d\t%2d:%2d:%2d\t%-9s\t\r\n",RTC_Date.Year,RTC_Date.Month,RTC_Date.Date,RTC_Time.Hours,RTC_Time.Minutes,RTC_Time.Seconds,Week[RTC_Date.WeekDay]);
	
	f_res = f_open(&file, "log.txt", FA_OPEN_ALWAYS | FA_READ |FA_WRITE);
	f_lseek(&file,f_size(&file));
	f_printf(&file,"\r\n20%2d-%2d-%2d\t%2d:%2d:%2d\t%-9s\tupgrade successful\r\n",RTC_Date.Year,RTC_Date.Month,RTC_Date.Date,RTC_Time.Hours,RTC_Time.Minutes,RTC_Time.Seconds,Week[RTC_Date.WeekDay]);
	f_close(&file);
	printf("add log\r\n");
}

uint8_t Receive_dat_buffer[2048];
void USB_IAP()
{
	volatile uint32_t addrx;
	uint32_t Read_data=0;
	uint16_t br;
	uint32_t file_size;
	f_res = f_mount(&fs,"0:",1);
	if (f_res != FR_OK)
	{
		printf("Mount failed\r\n");
		while(1);
	}
	f_res = f_open(&file, "updata.bin", FA_OPEN_EXISTING | FA_READ);
	if (f_res != FR_OK)
	{
		printf("Open failed:%d\r\n",f_res);
		while(1);
	}
	file_size=f_size(&file);
	printf("File size:%dByte\r\n",file_size);
	addrx = FLASH_APP1_ADDR;
	while(1)
	{
		f_res = f_read(&file, Receive_dat_buffer, 2048, (UINT*)&br);
		Read_data+=br;   //读取的总字节数
		if (f_res || br == 0) 
		{
			printf("Total:%dByte\r\n",Read_data);
			f_close(&file);
			break; 
		}
		iap_write_appbin(addrx,Receive_dat_buffer,br);
		printf("Write%4d%%\r\n",Read_data*100/file_size);
		addrx+=2048;//偏移2048  512*4=2048
	}
	//-----------------------------------
	if(((*(volatile uint32_t*)(FLASH_APP1_ADDR+4))&0xFF000000)==0x08000000)//判断是否为0X08XXXXXX.
	{	 
		printf("Upgrade successed ,jump to App\r\n");
		#if ADDLOG
			Log();
		#endif
		HAL_HCD_MspDeInit(&hhcd_USB_OTG_FS);
		HAL_UART_MspDeInit(&huart1);
		//NVIC_SystemReset();
		iap_load_app(FLASH_APP1_ADDR);//执行FLASH APP代码
	}else 
	{
			printf("Upgrade failed\r\n");	   
	}
}

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{
  switch(id)
  { 
    case HOST_USER_SELECT_CONFIGURATION:
      break;
		
    case HOST_USER_DISCONNECTION:
      Appli_state = APPLICATION_DISCONNECT;
      
      break;
      
    case HOST_USER_CLASS_ACTIVE:
      Appli_state = APPLICATION_READY;
      break;
      
    case HOST_USER_CONNECTION:
      break;

    default:
      break; 
  }
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
	MX_GPIO_Init();
	MX_USART1_UART_Init();
	
	if (HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_0) == 1)
	{
			printf("Jump to APP\r\n");
			iap_load_app(FLASH_APP1_ADDR);//执行FLASH APP代码
	}
	#if ADDLOG
			MX_RTC_Init();
	#endif

  if(FATFS_LinkDriver(&USBH_Driver, USBDISKPath) == 0)
  {
    if(USBH_Init(&hUSBHost, USBH_UserProcess, HOST_FS)==USBH_OK)
    {
    }
    USBH_RegisterClass(&hUSBHost, USBH_MSC_CLASS);
    USBH_Start(&hUSBHost);      
  }
	while(1)
	{
		USBH_Process(&hUSBHost);
		if(Appli_state==APPLICATION_READY)
		{
			USB_IAP();
			while(1);
		}
	}
}




