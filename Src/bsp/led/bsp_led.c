#include "led/bsp_led.h"

void LED_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
	
  LED1_RCC_CLK_ENABLE();
  LED2_RCC_CLK_ENABLE();
  LED3_RCC_CLK_ENABLE();
  HAL_GPIO_WritePin(LED1_GPIO, LED1_GPIO_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED2_GPIO, LED2_GPIO_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED2_GPIO, LED3_GPIO_PIN, GPIO_PIN_RESET);  

  GPIO_InitStruct.Pin = LED1_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED1_GPIO, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = LED2_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED2_GPIO, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = LED3_GPIO_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LED3_GPIO, &GPIO_InitStruct);
  
}


