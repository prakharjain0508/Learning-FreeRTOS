/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/
#include<stdio.h>
#include<stdint.h>
#include<string.h>

#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"

TaskHandle_t xTaskHandle1 = NULL;
TaskHandle_t xTaskHandle2 = NULL;

void vTask1_handler(void *params);
void vTask2_handler(void *params);

#ifdef USE_SEMIHOSTING
//used for semi-hosting
extern void initialise_monitor_handles();
#endif

static void prvSetupUart(void);
static void prvSetupHardware(void);  //private function used for Hardware(like peripherals) specific configurations

void printmsg(char *msg);

//some macros
#define TRUE 1
#define FALSE 0
#define AVAILABLE TRUE
#define NOT_AVAILABLE FALSE

char usr_msg[250] = {0};
uint8_t UART_ACCESS_KEY = AVAILABLE;

int main(void)
{

#ifdef USE_SEMIHOSTING
	initialise_monitor_handles();
	printf("This is a Hello world example code.\n");
#endif

	DWT->CTRL |= (1 << 0); 	//Enable CYCCNT in DWL_CTRL //Used for segger to maintain the timestamp

	//1. Resets the RCC clock configuration to the default reset state.
	//HSI ON, PLL OFF, HSE OFF, System clock = 16 MHz, cpu_clock = 16 MHz
	RCC_DeInit();

	//2. Update the SystenCoreClock Variable
	SystemCoreClockUpdate();

	prvSetupHardware();

	sprintf(usr_msg, "This is hello world application starting\r\n");
	printmsg(usr_msg);

	//start recording
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//3. Create 2 task, task-1 and task-2
    xTaskCreate( vTask1_handler, "Task-1", configMINIMAL_STACK_SIZE, NULL, 2, &xTaskHandle1);
    xTaskCreate( vTask2_handler, "Task-2", configMINIMAL_STACK_SIZE, NULL, 2, &xTaskHandle2);

    //4. Start the scheduler
    vTaskStartScheduler();


    //you will never return here
	for(;;);
}

void vTask1_handler(void *params)
{
	while(1)
	{
		if(UART_ACCESS_KEY == AVAILABLE)
		{
			UART_ACCESS_KEY = NOT_AVAILABLE;
			printmsg("Hello World: From Task-1\r\n");
			UART_ACCESS_KEY = AVAILABLE;
			taskYIELD();	//manually triggered context switch
		}

	}
}

void vTask2_handler(void *params)
{
	while(1)
	{
		if(UART_ACCESS_KEY == AVAILABLE)
		{
			UART_ACCESS_KEY = NOT_AVAILABLE;
			printmsg("Hello World: From Task-2\r\n");
			UART_ACCESS_KEY = AVAILABLE;
			taskYIELD();	//manually triggered context switch
		}
	}
}

static void prvSetupUart(void)
{
	GPIO_InitTypeDef gpio_uart_pins;
	USART_InitTypeDef uart2_init;

	//1. Enable the UART2 and GPIOA Peripheral clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	//PA2 is UART2_Tx, PA3 is UART_RX

	//2. Alternate function configuration of MCU pins to behave as UART2 TX and RX
	memset(&gpio_uart_pins, 0, sizeof(gpio_uart_pins));  //zeroing each and every member element of the structure
	gpio_uart_pins.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	gpio_uart_pins.GPIO_Mode = GPIO_Mode_AF;
	gpio_uart_pins.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &gpio_uart_pins);

	//3. AF mode settings for the pins
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);  //PA2
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART3);  //PA3

	//4. UART parameter initializations
	memset(&uart2_init, 0, sizeof(uart2_init));  //zeroing each and every member element of the structure
	uart2_init.USART_BaudRate = 115200;
	uart2_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart2_init.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	uart2_init.USART_Parity = USART_Parity_No;
	uart2_init.USART_StopBits = USART_StopBits_1;
	uart2_init.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &uart2_init);

	//5. Enable the UART to peripheral
	USART_Cmd(USART2, ENABLE);
}

static void prvSetupHardware(void)
{
	//setup UART2
	prvSetupUart();
}

void printmsg(char *msg)
{
	for(uint32_t i=0; i < strlen(msg); i++)
	{
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) != SET);
		USART_SendData(USART2, msg[i]);
	}

}
