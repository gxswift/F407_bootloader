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

char USBDISKPath[4];
FATFS fs;
FIL file;
FRESULT f_res;
UINT fnum;

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
		printf("Bootloader RTC error\r\n");
  }
	//__HAL_RCC_CLEAR_RESET_FLAGS();
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
SRAM_HandleTypeDef hlcd;


static void HAL_FSMC_LCD_MspInit(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;	
 
	__HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
	
  __HAL_RCC_FSMC_CLK_ENABLE();

  /** FSMC GPIO Configuration  
  PF0   ------> FSMC_A0
  PE7   ------> FSMC_D4
  PE8   ------> FSMC_D5
  PE9   ------> FSMC_D6
  PE10   ------> FSMC_D7
  PE11   ------> FSMC_D8
  PE12   ------> FSMC_D9
  PE13   ------> FSMC_D10
  PE14   ------> FSMC_D11
  PE15   ------> FSMC_D12
  PD8   ------> FSMC_D13
  PD9   ------> FSMC_D14
  PD10   ------> FSMC_D15
  PD14   ------> FSMC_D0
  PD15   ------> FSMC_D1
  PD0   ------> FSMC_D2
  PD1   ------> FSMC_D3
  PD4   ------> FSMC_NOE
  PD5   ------> FSMC_NWE
  PG12   ------> FSMC_NE4
  */
  GPIO_InitStruct.Pin = FSMC_LCD_DC_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;

  HAL_GPIO_Init(FSMC_LCD_DC_PORT, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = FSMC_LCD_CS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(FSMC_LCD_CS_PORT, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10 
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14 
                          |GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_14 
                          |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4 
                          |GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  HAL_GPIO_WritePin(FSMC_LCD_BK_PORT, FSMC_LCD_BK_PIN, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = FSMC_LCD_BK_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
  HAL_GPIO_Init(FSMC_LCD_BK_PORT, &GPIO_InitStruct);
}

void HAL_SRAM_MspInit(SRAM_HandleTypeDef* hsram)
{
  HAL_FSMC_LCD_MspInit();
}

static void HAL_FSMC_LCD_MspDeInit(void)
{
  __HAL_RCC_FSMC_CLK_DISABLE();
  
  /** FSMC GPIO Configuration  
  PF0   ------> FSMC_A0
  PE7   ------> FSMC_D4
  PE8   ------> FSMC_D5
  PE9   ------> FSMC_D6
  PE10   ------> FSMC_D7
  PE11   ------> FSMC_D8
  PE12   ------> FSMC_D9
  PE13   ------> FSMC_D10
  PE14   ------> FSMC_D11
  PE15   ------> FSMC_D12
  PD8   ------> FSMC_D13
  PD9   ------> FSMC_D14
  PD10   ------> FSMC_D15
  PD14   ------> FSMC_D0
  PD15   ------> FSMC_D1
  PD0   ------> FSMC_D2
  PD1   ------> FSMC_D3
  PD4   ------> FSMC_NOE
  PD5   ------> FSMC_NWE
  PG12   ------> FSMC_NE4
  */
  HAL_GPIO_DeInit(GPIOE, GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10 
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14 
                          |GPIO_PIN_15);

  HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_14 
                          |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4 
                          |GPIO_PIN_5);
  HAL_GPIO_DeInit(FSMC_LCD_DC_PORT, FSMC_LCD_DC_PIN);
  HAL_GPIO_DeInit(FSMC_LCD_CS_PORT, FSMC_LCD_CS_PIN);
}

void HAL_SRAM_MspDeInit(SRAM_HandleTypeDef* hsram)
{
  HAL_FSMC_LCD_MspDeInit();
}

void MX_FSMC_Init(void)
{
  FSMC_NORSRAM_TimingTypeDef Timing;

  /* 配置FSMC参数 */
  hlcd.Instance = FSMC_NORSRAM_DEVICE;
  hlcd.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;

  hlcd.Init.NSBank = FSMC_LCD_BANKx;
  hlcd.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hlcd.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
  hlcd.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
  hlcd.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
  hlcd.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hlcd.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
  hlcd.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
  hlcd.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
  hlcd.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
  hlcd.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
  hlcd.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  hlcd.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
  hlcd.Init.PageSize = FSMC_PAGE_SIZE_NONE;
  
  Timing.AddressSetupTime      = 0x02; //地址建立时间
  Timing.AddressHoldTime       = 0x00; //地址保持时间
  Timing.DataSetupTime         = 0x05; //数据建立时间
  Timing.BusTurnAroundDuration = 0x00;
  Timing.CLKDivision           = 0x00;
  Timing.DataLatency           = 0x00;
  Timing.AccessMode = FSMC_ACCESS_MODE_A;
  HAL_SRAM_Init(&hlcd, &Timing, &Timing);

  /* Disconnect NADV */
//  __HAL_AFIO_FSMCNADV_DISCONNECTED();
}



void LCD_SetDirection( uint8_t ucOption )
{	
/**
  * Memory Access Control (36h)
  * This command defines read/write scanning direction of the frame memory.
  *
  * These 3 bits control the direction from the MPU to memory write/read.
  *
  * Bit  Symbol  Name  Description
  * D7   MY  Row Address Order     -- 以X轴镜像
  * D6   MX  Column Address Order  -- 以Y轴镜像
  * D5   MV  Row/Column Exchange   -- X轴与Y轴交换
  * D4   ML  Vertical Refresh Order  LCD vertical refresh direction control. 
  *
  * D3   BGR RGB-BGR Order   Color selector switch control
  *      (0 = RGB color filter panel, 1 = BGR color filter panel )
  * D2   MH  Horizontal Refresh ORDER  LCD horizontal refreshing direction control.
  * D1   X   Reserved  Reserved
  * D0   X   Reserved  Reserved
  */
	switch ( ucOption )
	{
		case 1:
//   左上角->右下角 
//	(0,0)	___ x(320)
//	     |  
//	     |
//       |	y(480) 
			LCD_WRITE_CMD(0x36); 
			LCD_WRITE_DATA(0x08); 
      
			LCD_WRITE_CMD(0x2A); 
			LCD_WRITE_DATA(0x00);	/* x start */	
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x01);  /* x end */	
			LCD_WRITE_DATA(0x3F);

			LCD_WRITE_CMD(0x2B); 
			LCD_WRITE_DATA(0x00);	/* y start */  
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x01);	/* y end */   
			LCD_WRITE_DATA(0xDF);					
		  break;
		
		case 2:
//		右上角-> 左下角
//		y(320)___ (0,0)            
//		         |
//		         |
//             |x(480)    
			LCD_WRITE_CMD(0x36); 
			LCD_WRITE_DATA(0x68);	
			LCD_WRITE_CMD(0x2A); 
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x01);
			LCD_WRITE_DATA(0xDF);	

			LCD_WRITE_CMD(0x2B); 
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x01);
			LCD_WRITE_DATA(0x3F);				
		  break;
		
		case 3:
//		右下角->左上角
//		          |y(480)
//		          |           
//		x(320) ___|(0,0)		
			LCD_WRITE_CMD(0x36); 
			LCD_WRITE_DATA(0xC8);	
			LCD_WRITE_CMD(0x2A); 
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x01);
			LCD_WRITE_DATA(0x3F);	

			LCD_WRITE_CMD(0x2B); 
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x01);
			LCD_WRITE_DATA(0x3F);			  
		  break;

		case 4:
//		左下角->右上角
//		|x(480)
//		|
//		|___ y(320)					  
			LCD_WRITE_CMD(0x36); 
			LCD_WRITE_DATA(0xA8);	
    
			LCD_WRITE_CMD(0x2A); 
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x01);
			LCD_WRITE_DATA(0xDF);	

			LCD_WRITE_CMD(0x2B); 
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x00);
			LCD_WRITE_DATA(0x01);
			LCD_WRITE_DATA(0x3F);				
	    break;		
	}	
	/* 开始向GRAM写入数据 */
	LCD_WRITE_CMD (0x2C);	
}

static void ILI9488_REG_Config ( void )
{
  //************* Start Initial Sequence **********//
  /* PGAMCTRL (Positive Gamma Control) (E0h) */
  LCD_WRITE_CMD(0xE0);
  LCD_WRITE_DATA(0x00);
  LCD_WRITE_DATA(0x07);
  LCD_WRITE_DATA(0x10);
  LCD_WRITE_DATA(0x09);
  LCD_WRITE_DATA(0x17);
  LCD_WRITE_DATA(0x0B);
  LCD_WRITE_DATA(0x41);
  LCD_WRITE_DATA(0x89);
  LCD_WRITE_DATA(0x4B);
  LCD_WRITE_DATA(0x0A);
  LCD_WRITE_DATA(0x0C);
  LCD_WRITE_DATA(0x0E);
  LCD_WRITE_DATA(0x18);
  LCD_WRITE_DATA(0x1B);
  LCD_WRITE_DATA(0x0F);

  /* NGAMCTRL (Negative Gamma Control) (E1h)  */
  LCD_WRITE_CMD(0XE1);
  LCD_WRITE_DATA(0x00);
  LCD_WRITE_DATA(0x17);
  LCD_WRITE_DATA(0x1A);
  LCD_WRITE_DATA(0x04);
  LCD_WRITE_DATA(0x0E);
  LCD_WRITE_DATA(0x06);
  LCD_WRITE_DATA(0x2F);
  LCD_WRITE_DATA(0x45);
  LCD_WRITE_DATA(0x43);
  LCD_WRITE_DATA(0x02);
  LCD_WRITE_DATA(0x0A);
  LCD_WRITE_DATA(0x09);
  LCD_WRITE_DATA(0x32);
  LCD_WRITE_DATA(0x36);
  LCD_WRITE_DATA(0x0F);
  
  /* Adjust Control 3 (F7h)  */
  LCD_WRITE_CMD(0XF7);
  LCD_WRITE_DATA(0xA9);
  LCD_WRITE_DATA(0x51);
  LCD_WRITE_DATA(0x2C);
  LCD_WRITE_DATA(0x82);/* DSI write DCS command, use loose packet RGB 666 */

  /* Power Control 1 (C0h)  */
  LCD_WRITE_CMD(0xC0);
  LCD_WRITE_DATA(0x11);
  LCD_WRITE_DATA(0x09);

  /* Power Control 2 (C1h) */
  LCD_WRITE_CMD(0xC1);
  LCD_WRITE_DATA(0x41);

  /* VCOM Control (C5h)  */
  LCD_WRITE_CMD(0XC5);
  LCD_WRITE_DATA(0x00);
  LCD_WRITE_DATA(0x0A);
  LCD_WRITE_DATA(0x80);

  /* Frame Rate Control (In Normal Mode/Full Colors) (B1h) */
  LCD_WRITE_CMD(0xB1);
  LCD_WRITE_DATA(0xB0);
  LCD_WRITE_DATA(0x11);

  /* Display Inversion Control (B4h) */
  LCD_WRITE_CMD(0xB4);
  LCD_WRITE_DATA(0x02);

  /* Display Function Control (B6h)  */
  LCD_WRITE_CMD(0xB6);
  LCD_WRITE_DATA(0x02);
  LCD_WRITE_DATA(0x22);

  /* Entry Mode Set (B7h)  */
  LCD_WRITE_CMD(0xB7);
  LCD_WRITE_DATA(0xc6);

  /* HS Lanes Control (BEh) */
  LCD_WRITE_CMD(0xBE);
  LCD_WRITE_DATA(0x00);
  LCD_WRITE_DATA(0x04);

  /* Set Image Function (E9h)  */
  LCD_WRITE_CMD(0xE9);
  LCD_WRITE_DATA(0x00);
 
  /* 设置屏幕方向和尺寸 */
  LCD_SetDirection(LCD_DIRECTION);
  
  /* Interface Pixel Format (3Ah) */
  LCD_WRITE_CMD(0x3A);
  LCD_WRITE_DATA(0x55);/* 0x55 : 16 bits/pixel  */

  /* Sleep Out (11h) */
  LCD_WRITE_CMD(0x11);
  HAL_Delay(120);
  /* Display On */
  LCD_WRITE_CMD(0x29);
}

void LCD_OpenWindow(uint16_t usX, uint16_t usY, uint16_t usWidth, uint16_t usHeight)
{	
	LCD_WRITE_CMD(0x2A ); 				       /* 设置X坐标 */
	LCD_WRITE_DATA(usX>>8);	             /* 设置起始点：先高8位 */
	LCD_WRITE_DATA(usX&0xff);	           /* 然后低8位 */
	LCD_WRITE_DATA((usX+usWidth-1)>>8);  /* 设置结束点：先高8位 */
	LCD_WRITE_DATA((usX+usWidth-1)&0xff);/* 然后低8位 */

	LCD_WRITE_CMD(0x2B); 			           /* 设置Y坐标*/
	LCD_WRITE_DATA(usY>>8);              /* 设置起始点：先高8位 */
	LCD_WRITE_DATA(usY&0xff);            /* 然后低8位 */
	LCD_WRITE_DATA((usY+usHeight-1)>>8); /* 设置结束点：先高8位 */
	LCD_WRITE_DATA((usY+usHeight-1)&0xff);/* 然后低8位 */
}


static void LCD_SetCursor(uint16_t usX,uint16_t usY)	
{
	LCD_OpenWindow(usX,usY,1,1);
}

static __inline void LCD_FillColor ( uint32_t ulAmout_Point, uint16_t usColor )
{
	uint32_t i = 0;	
	LCD_WRITE_CMD ( 0x2C );	
	for ( i = 0; i < ulAmout_Point; i ++ )
		LCD_WRITE_DATA ( usColor );	
}

void LCD_SetPointPixel(uint16_t usX,uint16_t usY,uint16_t usColor)	
{	
	if((usX<LCD_DEFAULT_WIDTH)&&(usY<LCD_DEFAULT_HEIGTH))
  {
		LCD_OpenWindow(usX,usY,1,1);
		LCD_FillColor(1,usColor);
	}
}

uint16_t LCD_GetPointPixel ( uint16_t usX, uint16_t usY )
{ 
	uint16_t usPixelData;
	uint16_t usR=0, usG=0, usB=0 ;
	LCD_SetCursor ( usX, usY );
	
	LCD_WRITE_CMD ( 0x2E );   /* 读数据 */
	usR = LCD_READ_DATA (); 	/*FIRST READ OUT DUMMY DATA*/
	
	usR = LCD_READ_DATA ();  	/*READ OUT RED DATA  */
	usB = LCD_READ_DATA ();  	/*READ OUT BLUE DATA*/
	usG = LCD_READ_DATA ();  	/*READ OUT GREEN DATA*/
	
	usPixelData = (((usR>>11)<<11) | ((usG>>10)<<5) | (usB>>11));
	return usPixelData;
}
void LCD_Clear(uint16_t usX,uint16_t usY,uint16_t usWidth,uint16_t usHeight,uint16_t usColor)
{	 
#if 0   /* 优化代码执行速度 */
  uint32_t i;
	uint32_t n,m;
  LCD_OpenWindow(usX,usY,usWidth,usHeight); 
  LCD_WRITE_CMD(0x2C);
  
  m=usWidth * usHeight;
  n=m/8;
  m=m-8*n;
	for(i=0;i<n;i++)
	{
		LCD_WRITE_DATA(usColor);	
    LCD_WRITE_DATA(usColor);	
    LCD_WRITE_DATA(usColor);	
    LCD_WRITE_DATA(usColor);	
    
    LCD_WRITE_DATA(usColor);	
    LCD_WRITE_DATA(usColor);	
    LCD_WRITE_DATA(usColor);	
    LCD_WRITE_DATA(usColor);	
	}
  for(i=0;i<m;i++)
	{
		LCD_WRITE_DATA(usColor);	
	}
#else
  LCD_OpenWindow(usX,usY,usWidth,usHeight);
	LCD_FillColor(usWidth*usHeight,usColor);	
#endif	
}

void BSP_LCD_Init(void)
{
  MX_FSMC_Init();
	
  ILI9488_REG_Config();
  LCD_Clear(0,0,LCD_DEFAULT_WIDTH,LCD_DEFAULT_HEIGTH,BLACK);
  HAL_Delay(20);
}

extern const unsigned char ucAscii_2412[95][48];

void LCD_DispChar_EN( uint16_t usX, uint16_t usY, const char cChar, uint16_t usColor_Background, uint16_t usColor_Foreground)
{
	uint8_t ucTemp, ucRelativePositon, ucPage, ucColumn;
  
	ucRelativePositon = cChar - ' ';
  
	LCD_OpenWindow(usX,usY,12,24);
	LCD_WRITE_CMD(0x2C);
	
	for(ucPage=0;ucPage<48;ucPage++)
	{
		ucTemp=ucAscii_2412[ucRelativePositon][ucPage];		
		for(ucColumn=0;ucColumn<8;ucColumn++)
		{
			if(ucTemp&0x01)
				LCD_WRITE_DATA(usColor_Foreground);			
			else
				LCD_WRITE_DATA(usColor_Background);								
			ucTemp >>= 1;					
		}	
		ucPage++;
		ucTemp=ucAscii_2412[ucRelativePositon][ucPage];
		for(ucColumn=0;ucColumn<4;ucColumn++)
		{
			if(ucTemp&0x01)
				LCD_WRITE_DATA(usColor_Foreground);			
			else
				LCD_WRITE_DATA(usColor_Background);								
			ucTemp >>= 1;					
		}	
	}
}

void LCD_DispString_EN ( uint16_t usX, uint16_t usY, const char * pStr, uint16_t usColor_Background, uint16_t usColor_Foreground)
{
	while ( * pStr != '\0' )
	{
		if ( ( usX +  12 ) > LCD_DEFAULT_WIDTH )
		{
			usX = 0;
			usY += 24;
		}      
		if ( ( usY +  24 ) > LCD_DEFAULT_HEIGTH )
		{
			usX = 0;
			usY = 0;
		}      
		LCD_DispChar_EN ( usX, usY, * pStr, usColor_Background, usColor_Foreground);
		pStr ++;      
		usX += 12;
	}
}

void ProgressBar(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t percent,uint16_t color)
{
	uint16_t xend;
	uint16_t i,j;
	xend = x1 + (x2 - x1)*percent/100;
	for (i = x1;i < xend;i++)
	{
		for (j = y1;j < y2;j++)
		{
			LCD_SetPointPixel(i,j,color);
		}
	}
}
//-------------------------------------------------------------------------------------------------------------------------
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
	  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
 
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
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
  }
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000); 
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}
/*
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
*/
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
	if(WriteAddr<0x08000000||WriteAddr%4)return;
		
	HAL_FLASH_Unlock();
	addrx=WriteAddr;
	endaddr=WriteAddr+NumToWrite*4;

	while(addrx<endaddr)//擦除扇区
	{
		if (*(volatile uint32_t *)addrx!=0XFFFFFFFF)
		{   
			FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;
			FlashEraseInit.Sector=GetSector(addrx);
			FlashEraseInit.NbSectors=1; 
			FlashEraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;
			if(HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError)!=HAL_OK) 
				break;
		}else addrx+=4;
	}

	while(WriteAddr<endaddr)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,WriteAddr,*pBuffer)!=HAL_OK)//写入数据
			break;
		WriteAddr+=4;
		pBuffer++;
	}  
	HAL_FLASH_Lock();
} 


uint32_t iapbuf[512];
void iap_write_appbin(uint32_t appxaddr,uint8_t *appbuf,uint16_t appsize)
{
	uint32_t t;
	uint16_t i=0;
	uint32_t temp;
	uint32_t fwaddr=appxaddr;
	uint8_t *dfu=appbuf;
	for(t=0;t<appsize;t+=4)
	{						   
		temp=(uint32_t)dfu[3]<<24;   
		temp|=(uint32_t)dfu[2]<<16;    
		temp|=(uint32_t)dfu[1]<<8;
		temp|=(uint32_t)dfu[0];	  
		dfu+=4;
		iapbuf[i++]=temp;	    
		if(i==512)
		{
			i=0; 
			STMFLASH_Write(fwaddr,iapbuf,512);
			fwaddr+=2048;
		}   
	} 
	if(i)STMFLASH_Write(fwaddr,iapbuf,i); 
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
#define LCD_Display	1

#define Dispaly_Color	DARKGREEN
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
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t SectorError=0;	
	#if LCD_Display
	uint32_t percent;
	char str[4];

	LCD_DispString_EN(170,5,"Upgrading...",0,Dispaly_Color);
	LCD_DispString_EN(20,40,"Main Board",0,Dispaly_Color);
	LCD_DispString_EN(450,40,"%",0,Dispaly_Color);
	#endif
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
		Read_data+=br;
		if (f_res || br == 0) 
		{
			printf("Total:%dByte\r\n",Read_data);
			f_close(&file);
			if (Read_data != file_size)
			{
				printf("Upgrade failed\r\n");	
				#if LCD_Display
				LCD_DispString_EN(140,200,"Upgrade Fault!",0,Dispaly_Color);
				#endif
				while(1);
			}
			break; 
		}
		iap_write_appbin(addrx,Receive_dat_buffer,br);
		percent = Read_data*100/file_size;
		printf("Write%4d%%\r\n",percent);
		#if LCD_Display
		sprintf(str,"%3d",percent);
		LCD_DispString_EN(410,40,str,0,Dispaly_Color);
		ProgressBar(150,42,400,62,percent,Dispaly_Color);
		#endif
		
		addrx+=2048;
	}
	//-----------------------------------
	if(((*(volatile uint32_t*)(FLASH_APP1_ADDR+4))&0xFF000000)==0x08000000)
	{	 
		printf("Upgrade successed ,jump to App\r\n");
		#if ADDLOG
			Log();
		#endif
		#if LCD_Display
		LCD_DispString_EN(140,200,"Upgrade Completed!",0,Dispaly_Color);
		//--------------------------------------------------------------------------------------------
			if (*(volatile uint32_t *)0x080E0000!=0XFFFFFFFF)
			{   
				HAL_FLASH_Unlock();
				FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;
				FlashEraseInit.Sector=FLASH_SECTOR_11;
				FlashEraseInit.NbSectors=1; 
				FlashEraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;
				HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError);
				HAL_FLASH_Lock();
			}
		//--------------------------------------------------------------------------------------------
		HAL_Delay(1000);
		#endif
		HAL_HCD_MspDeInit(&hhcd_USB_OTG_FS);
		USBH_DeInit(&hUSBHost);		
		HAL_UART_MspDeInit(&huart1);
			__HAL_RCC_RTC_DISABLE();
//			__set_FAULTMASK(1);
//		NVIC_SystemReset();
		iap_load_app(FLASH_APP1_ADDR);
	}else 
	{
		#if LCD_Display
		LCD_DispString_EN(140,200,"Upgrade Fault!",0,Dispaly_Color);
		#endif
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
//不同U盘识别时间不一定，用硬件条件触发，避免开机跳转慢
int main(void)
{
	FLASH_EraseInitTypeDef FlashEraseInit;
	uint32_t SectorError=0;
	uint32_t tickstart;
  HAL_Init();
  SystemClock_Config();
	MX_GPIO_Init();
	MX_USART1_UART_Init();
	
	if ((HAL_GPIO_ReadPin(GPIOE,GPIO_PIN_0) == 1 ) && ((*(volatile uint32_t *)0x080E0000!=0XB9F9D0C2)))
	{
		printf("%X\t",*(volatile uint32_t *)0x080E0000);
			printf("Jump to APP\r\n");
			iap_load_app(FLASH_APP1_ADDR);
	}

	#if ADDLOG
		MX_RTC_Init();
	#endif
	
	#if LCD_Display
	BSP_LCD_Init();
	LCD_BK_ON();
	LCD_Clear(0,0,LCD_DEFAULT_WIDTH,LCD_DEFAULT_HEIGTH,0);
	LCD_DispString_EN(170,5,"Initializing...",0,Dispaly_Color);
	#endif

  if(FATFS_LinkDriver(&USBH_Driver, USBDISKPath) == 0)
  {
    if(USBH_Init(&hUSBHost, USBH_UserProcess, HOST_FS)==USBH_OK)
    {
    }
    USBH_RegisterClass(&hUSBHost, USBH_MSC_CLASS);
    USBH_Start(&hUSBHost);      
  }
	tickstart = HAL_GetTick();
	while(1)
	{
		USBH_Process(&hUSBHost);
		if(Appli_state==APPLICATION_READY)
		{
			USB_IAP();
			while(1);
		}
		if (HAL_GetTick()-tickstart > 5000)
		{
	#if LCD_Display
			LCD_DispString_EN(100,5,"Timeout,Upgrade Fault!",0,Dispaly_Color);
			HAL_Delay(1000);
	#endif
			if (*(volatile uint32_t *)0x080E0000!=0XFFFFFFFF)
			{   
				HAL_FLASH_Unlock();
				FlashEraseInit.TypeErase=FLASH_TYPEERASE_SECTORS;
				FlashEraseInit.Sector=FLASH_SECTOR_11;
				FlashEraseInit.NbSectors=1; 
				FlashEraseInit.VoltageRange=FLASH_VOLTAGE_RANGE_3;
				HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError);
				HAL_FLASH_Lock();
			}
			printf("Timeout,Jump to APP\r\n");
		HAL_HCD_MspDeInit(&hhcd_USB_OTG_FS);
		USBH_DeInit(&hUSBHost);		
		HAL_UART_MspDeInit(&huart1);
			__HAL_RCC_RTC_DISABLE();
			iap_load_app(FLASH_APP1_ADDR);			
		}
	}
}




