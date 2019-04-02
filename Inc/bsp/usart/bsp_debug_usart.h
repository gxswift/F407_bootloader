#ifndef __BSP_DEBUG_USART_H__
#define __BSP_DEBUG_USART_H__

#include "stm32f4xx_hal.h"
#include <stdio.h>

#define DEBUG_USARTx                                 USART1
#define DEBUG_USARTx_BAUDRATE                        115200
#define DEBUG_USART_RCC_CLK_ENABLE()                 __HAL_RCC_USART1_CLK_ENABLE()
#define DEBUG_USART_RCC_CLK_DISABLE()                __HAL_RCC_USART1_CLK_DISABLE()

#define DEBUG_USARTx_GPIO_ClK_ENABLE()               __HAL_RCC_GPIOB_CLK_ENABLE()
#define DEBUG_USARTx_Tx_GPIO_PIN                     GPIO_PIN_6
#define DEBUG_USARTx_Tx_GPIO                         GPIOB
#define DEBUG_USARTx_Rx_GPIO_PIN                     GPIO_PIN_7
#define DEBUG_USARTx_Rx_GPIO                         GPIOB

#define DEBUG_USARTx_AFx                             GPIO_AF7_USART1

#define DEBUG_USART_IRQn                             USART1_IRQn
extern UART_HandleTypeDef husart_debug;

void MX_DEBUG_USART_Init(void);


#endif  
