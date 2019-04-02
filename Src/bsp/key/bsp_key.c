#include "key/bsp_key.h"


void KEY_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
	
  KEY1_RCC_CLK_ENABLE();
  KEY2_RCC_CLK_ENABLE(); 
  KEY3_RCC_CLK_ENABLE();
  KEY4_RCC_CLK_ENABLE();	
  KEY5_RCC_CLK_ENABLE(); 
  
  GPIO_InitStruct.Pin = KEY1_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY1_GPIO, &GPIO_InitStruct);  
  
  GPIO_InitStruct.Pin = KEY2_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY2_GPIO, &GPIO_InitStruct);  

  GPIO_InitStruct.Pin = KEY3_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY3_GPIO, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = KEY4_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY4_GPIO, &GPIO_InitStruct); 
  
  GPIO_InitStruct.Pin = KEY5_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY5_GPIO, &GPIO_InitStruct);  
  
}


