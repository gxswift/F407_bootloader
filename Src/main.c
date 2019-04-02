#include "stm32f4xx_hal.h"
#include "usart/bsp_debug_usart.h"
#include "led/bsp_led.h"
#include "usbh_core.h"
#include "usbh_msc.h" 
#include "ff.h"
#include "ff_gen_drv.h"
#include "key/bsp_key.h"
#include "stmflash/stm_flash.h"
#include "rtc.h"

typedef enum {
  APPLICATION_IDLE = 0,  
  APPLICATION_READY,    
  APPLICATION_DISCONNECT,
}MSC_ApplicationTypeDef;

USBH_HandleTypeDef hUSBHost;
MSC_ApplicationTypeDef Appli_state = APPLICATION_IDLE;

char USBDISKPath[4];             /* ����Flash�߼��豸·�� */
FATFS fs;													/* FatFs�ļ�ϵͳ���� */
FIL file;													/* �ļ����� */
FRESULT f_res;                    /* �ļ�������� */
UINT fnum;            					  /* �ļ��ɹ���д���� */

extern Diskio_drvTypeDef  USBH_Driver;

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id);


void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
 
  __HAL_RCC_PWR_CLK_ENABLE();                                     //ʹ��PWRʱ��

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);  //���õ�ѹ�������ѹ����1

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;      // �ⲿ����8MHz
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;                        //��HSE 
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;                    //��PLL
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;            //PLLʱ��Դѡ��HSE
  RCC_OscInitStruct.PLL.PLLM = 8;                                 //8��ƵMHz
  RCC_OscInitStruct.PLL.PLLN = 336;                               //336��Ƶ
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;                     //2��Ƶ���õ�168MHz��ʱ��
  RCC_OscInitStruct.PLL.PLLQ = 7;                                 //USB/SDIO/������������ȵ���PLL��Ƶϵ��
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;       // ϵͳʱ�ӣ�168MHz
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;              // AHBʱ�ӣ� 168MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;               // APB1ʱ�ӣ�42MHz
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;               // APB2ʱ�ӣ�84MHz
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  HAL_RCC_EnableCSS();                                            // ʹ��CSS���ܣ�����ʹ���ⲿ�����ڲ�ʱ��ԴΪ����
  
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000); 
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
//-------------------------------------------------------------------------------------------------------------------------
typedef  void (*iapfun)(void);				//����һ���������͵Ĳ���.   
#define FLASH_APP1_ADDR		0x08010000  	//��һ��Ӧ�ó�����ʼ��ַ(�����FLASH)


iapfun jump2app; 
uint32_t iapbuf[512]; 	//2K�ֽڻ��� 


void iap_write_appbin(uint32_t appxaddr,uint8_t *appbuf,uint16_t appsize)
{
	uint32_t t;
	uint16_t i=0;
	uint32_t temp;
	uint32_t fwaddr=appxaddr;//��ǰд��ĵ�ַ
	uint8_t *dfu=appbuf;
	for(t=0;t<appsize;t+=4)
	{						   
		temp=(uint32_t)dfu[3]<<24;   
		temp|=(uint32_t)dfu[2]<<16;    
		temp|=(uint32_t)dfu[1]<<8;
		temp|=(uint32_t)dfu[0];	  
		dfu+=4;//ƫ��4���ֽ�
		iapbuf[i++]=temp;	    
		if(i==512)
		{
			i=0; 
			STMFLASH_Write(fwaddr,iapbuf,512);
			fwaddr+=2048;//ƫ��2048  512*4=2048
		}
        
	} 
	if(i)STMFLASH_Write(fwaddr,iapbuf,i);//������һЩ�����ֽ�д��ȥ.  
}




//��ת��Ӧ�ó����
//appxaddr:�û�������ʼ��ַ.
//����ջ����ַ
//addr:ջ����ַ
__asm void MSR_MSP(uint32_t addr) 
{
	MSR MSP, r0 			//set Main Stack value
	BX r14
}


void iap_load_app(uint32_t appxaddr)
{
	if(((*(volatile uint32_t*)appxaddr)&0xFF000000)==0x20000000
		|| (((*(__IO uint32_t *) appxaddr) & 0xFF000000) ==
            0x10000000))	//���ջ����ַ�Ƿ�Ϸ�.
	{ 
		jump2app=(iapfun)*(volatile uint32_t*)(appxaddr+4);		//�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)		
        MSR_MSP(*(volatile uint32_t*)appxaddr);					//��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
        jump2app();									//��ת��APP.
	}
}		 

uint32_t file_size;
uint8_t Receive_dat_buffer[2048];

uint16_t br;


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

extern HCD_HandleTypeDef hhcd_USB_OTG_FS;
#define ADDLOG	1
void Log()
{
	RTC_TimeTypeDef RTC_Time;
	RTC_DateTypeDef RTC_Date;

	HAL_RTC_GetTime(&hrtc,&RTC_Time,RTC_FORMAT_BIN);			
	HAL_RTC_GetDate(&hrtc,&RTC_Date,RTC_FORMAT_BIN);
	printf("20%d-%d-%d\t%d:%d:%d\t\t%s\t\r\n",RTC_Date.Year,RTC_Date.Month,RTC_Date.Date,RTC_Time.Hours,RTC_Time.Minutes,RTC_Time.Seconds,Week[RTC_Date.WeekDay]);
	
	f_res = f_open(&file, "log.txt", FA_OPEN_ALWAYS | FA_READ |FA_WRITE);
	f_lseek(&file,f_size(&file));
	f_printf(&file,"\r\n20%d-%d-%d\t%d:%d:%d\t\t%s\tupgrade successful\r\n",RTC_Date.Year,RTC_Date.Month,RTC_Date.Date,RTC_Time.Hours,RTC_Time.Minutes,RTC_Time.Seconds,Week[RTC_Date.WeekDay]);
	f_close(&file);
	printf("add log\r\n");
}
void USB_IAP()
{
	volatile uint32_t addrx;
	uint32_t Read_data=0;
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
	file_size=f_size(&file);    //��ȡ���ļ���СByte
	printf("File size:%dByte\r\n",file_size);
	addrx = FLASH_APP1_ADDR;
	while(1)
	{
		f_res = f_read(&file, Receive_dat_buffer, 2048, (UINT*)&br);
		Read_data+=br;   //��ȡ�����ֽ���
		if (f_res || br == 0) 
		{
			printf("Total:%dByte\r\n",Read_data);
			f_close(&file);
			break; 
		}
		iap_write_appbin(addrx,Receive_dat_buffer,br);//����ȡ������д��Flash��
	//	printf("address%x,size%d\r\n",addrx,br);
		printf("Write%4d%%\r\n",Read_data*100/file_size);
		addrx+=2048;//ƫ��2048  512*4=2048
	}
	//-----------------------------------
	if(((*(volatile uint32_t*)(FLASH_APP1_ADDR+4))&0xFF000000)==0x08000000)//�ж��Ƿ�Ϊ0X08XXXXXX.
	{	 
		printf("Upgrade successed ,jump to App\r\n");
		#if ADDLOG
			Log();
		#endif
		HAL_HCD_MspDeInit(&hhcd_USB_OTG_FS);
		HAL_UART_MspDeInit(&husart_debug);
		//NVIC_SystemReset();
	
		iap_load_app(FLASH_APP1_ADDR);//ִ��FLASH APP����
	}else 
	{
			printf("Upgrade failed\r\n");	   
	}
}


int main(void)
{
  HAL_Init();
  SystemClock_Config();
  LED_GPIO_Init();
  KEY_GPIO_Init();
  
  MX_DEBUG_USART_Init();

	
	if (HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_0) == 1)
	{
			printf("Jump to APP\r\n");
			iap_load_app(FLASH_APP1_ADDR);//ִ��FLASH APP����
	}
	else
	{
		printf("Upgrate\r\n");
			MX_RTC_Init();
	}
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


