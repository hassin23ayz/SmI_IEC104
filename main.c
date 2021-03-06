/*
 * main.c
 *
 *  Created on: 08.08.2014
 *      Author: sagok
 */


/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "cmsis_os.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_pwr.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_it.h"

#include "main.h"
//#include "iwdg.h"
//#include "lwip.h"
#include "ethernetif.h"
#include "usart.h"
#include "clocks.h"
#include "gpio.h"

#include "ConfBoard.h"


//uint8_t 	Modbus_DataTX[255];		// ����� ����������� Modbus
uint8_t 	Modbus_DataRX[255];		// ����� �������� Modbus

// ����������
GPIO_TypeDef* GPIO_PORT[PORTn] = {LED1_GPIO_PORT, LED2_GPIO_PORT, LED3_GPIO_PORT, LED4_GPIO_PORT, RS485_1_DE_GPIO_PORT, RS485_2_DE_GPIO_PORT, MODBUS_DE_GPIO_PORT};
const uint16_t GPIO_PIN[PORTn] = {LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN, RS485_1_DE, RS485_2_DE, MODBUS_DE};


UART_HandleTypeDef MODBUS;				//UART4
UART_HandleTypeDef BOOT_UART;			//USART1
UART_HandleTypeDef RS485_1;				//USART2
UART_HandleTypeDef RS485_2;				//USART3

xQueueHandle 	  xQueueMODBUS;			//��� ���������� ������ �� �������

uint16_t	xMasterOsEvent;				// ��������� ������� ����� MODBUS

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void FREERTOS_Init(void);

int main(void) {


	  HAL_Init();						// ����. Flash � Systick.
	  SystemClock_Config();				// ������ �����������.
	  GPIO_Init();						// ������ ������.
	  Clocks_Init();					// ������ �����.

	  BOOT_UART_Init(115200);			// ��������� BOOT ����������.
	  USART_TRACE("------------------------------------\n");
	  USART_TRACE("BOOT_Init.. ok\n");

	  RS485_1_UART_Init(115200);		// ��������� RS485 1 ������.
	  USART_TRACE("RS485_1_UART_Init.. ok\n");

  	  RS485_2_UART_Init(115200);		// ��������� RS485 2 ������.
  	  USART_TRACE("RS485_2_UART_Init.. ok\n");

	  // ��� ����� �������� ��������� ������� �� ���������(MODBUS) ��������, ������, �����....

	  Port_Init(LED1,GPIO_MODE_OUTPUT_PP);
	  Port_Init(LED2,GPIO_MODE_OUTPUT_PP);
	  Port_Init(LED3,GPIO_MODE_OUTPUT_PP);
	  Port_Init(LED4,GPIO_MODE_OUTPUT_PP);

	  Port_Init(MODBUS_DEn,GPIO_MODE_INPUT);			// ���� �� ����������

	  FREERTOS_Init();			// ����. FREERTOS.

	  osKernelStart();			// ����� ������������

	while (1) {
	}

	return 0;
}

/*************************************************************************
 * vApplicationStackOverflowHook
 *************************************************************************/
void vApplicationStackOverflowHook( xTaskHandle pxTask, signed char *pcTaskName )
{
	( void ) pxTask;
	( void ) pcTaskName;
	for( ;; );
}
/*************************************************************************
 * vApplicationIdleHook
 *************************************************************************/
extern void vApplicationIdleHook(void)
{

}
/*************************************************************************
* System Clock Configuration
* HSE 8MHz : PLLM = 8 PLLN = 336 PLLP = /2 PLLQ = 7 => HCLK = 168 MHz
* 				APB1 = /4 => PCLK1 							= 42MHz
* 				APB2 = /2 => PCLK1 							= 84MHz
*
* 				TIM2...TIM7,12,13,14 CLK = 2 * PCLK1 		= 84MHz
* 				TIM1,8,9,10,11 CLK = 2 * PCLK2 				= 168MHz
* 				Systick										= 21MHz
*
 *************************************************************************/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;								//5
  RCC_OscInitStruct.PLL.PLLN = 336;								//210
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;					//RCC_PLLP_DIV2
  RCC_OscInitStruct.PLL.PLLQ = 7;								// 7
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

}
/*************************************************************************
 * Port_Init
 *************************************************************************/
void Port_Init(Port_TypeDef Port,uint32_t mode)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  PORTx_GPIO_CLK_ENABLE(Port);

  GPIO_InitStruct.Pin = GPIO_PIN[Port];
  GPIO_InitStruct.Mode = mode;// GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;

  HAL_GPIO_Init(GPIO_PORT[Port], &GPIO_InitStruct);
}
/*************************************************************************
 * Port_On
 *************************************************************************/
void Port_On(Port_TypeDef Port)
{
  HAL_GPIO_WritePin(GPIO_PORT[Port], GPIO_PIN[Port], GPIO_PIN_SET);
}
/*************************************************************************
 * Port_Off
 *************************************************************************/
void Port_Off(Port_TypeDef Port)
{
  HAL_GPIO_WritePin(GPIO_PORT[Port], GPIO_PIN[Port], GPIO_PIN_RESET);
}
/*************************************************************************
 * Port_Toggle
 *************************************************************************/
void Port_Toggle(Port_TypeDef Port)
{
  HAL_GPIO_TogglePin(GPIO_PORT[Port], GPIO_PIN[Port]);
}
/*************************************************************************
 * PUTCHAR_PROTOTYPE
 *************************************************************************/
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&BOOT_UART, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}
int __io_putstrDMA(char *ptr, int len)
{
//  HAL_UART_Transmit_DMA(&BOOT_UART, (uint8_t *)ptr, len);
  HAL_UART_Transmit(&BOOT_UART, (uint8_t *)ptr, len, 0xFFFF);
  return len;
}
/*************************************************************************
 *
 *************************************************************************/
