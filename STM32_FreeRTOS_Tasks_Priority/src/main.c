/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/
#include<string.h>
#include<stdint.h>
#include<stdio.h>
#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"

#define TRUE 1
#define FALSE 0

//function prototypes
static void prvSetupHardware(void);
static void prvSetupUart(void);
void prvSetupGPIO(void);
void printmsg(char *msg);
void rtos_delay(uint32_t delay_in_ms);

//tasks prototypes
void vTask1_handler(void *params);
void vTask2_handler(void *params);

//global space for some variables
char usr_msg[250] = {0};
TaskHandle_t xTaskHandle1 = NULL;
TaskHandle_t xTaskHandle2 = NULL;
uint8_t switch_prio = FALSE;

int main(void)
{
	DWT->CTRL |= (1 << 0); 	//Enable CYCCNT in DWL_CTRL //Used for segger to maintain the timestamp

	//1. Resets the RCC clock configuration to the default reset state.
	//HSI ON, PLL OFF, HSE OFF, System clock = 16 MHz, cpu_clock = 16 MHz
	RCC_DeInit();

	//2. Update the SystenCoreClock Variable
	SystemCoreClockUpdate();

	prvSetupHardware();

	sprintf(usr_msg, "This is Task Switching Priority Demo\r\n");
	printmsg(usr_msg);

	//lets create Task-1 and Task-2
	xTaskCreate(vTask1_handler, "TASK-1", 500, NULL, 2, &xTaskHandle1);
	xTaskCreate(vTask2_handler, "TASK-2", 500, NULL, 3, &xTaskHandle2);

	//let start scheduler
	vTaskStartScheduler();

	for(;;);
}

void vTask1_handler(void *params)
{
	UBaseType_t p1,p2;

	sprintf(usr_msg, "Task-1 is running\r\n");
	printmsg(usr_msg);

	sprintf(usr_msg, "Task-1 Priority %ld\r\n", uxTaskPriorityGet(xTaskHandle1));
	printmsg(usr_msg);

	sprintf(usr_msg, "Task-2 Priority %ld\r\n", uxTaskPriorityGet(xTaskHandle2));
	printmsg(usr_msg);

	while(1)
	{
		if(switch_prio)
		{
			switch_prio = FALSE;

			p1 = uxTaskPriorityGet(xTaskHandle1);
			p2 = uxTaskPriorityGet(xTaskHandle2);

			//switch_prio
			vTaskPrioritySet(xTaskHandle1, p2);
			vTaskPrioritySet(xTaskHandle2, p1);
		}
		else
		{
			GPIO_ToggleBits(GPIOA, GPIO_Pin_5);
			rtos_delay(200);
		}
	}
}

void vTask2_handler(void *params)
{
	UBaseType_t p1,p2;

	sprintf(usr_msg, "Task-2 is running\r\n");
	printmsg(usr_msg);

	sprintf(usr_msg, "Task-1 Priority %ld\r\n", uxTaskPriorityGet(xTaskHandle1));
	printmsg(usr_msg);

	sprintf(usr_msg, "Task-2 Priority %ld\r\n", uxTaskPriorityGet(xTaskHandle2));
	printmsg(usr_msg);

	while(1)
	{
		if(switch_prio)
		{
			switch_prio = FALSE;

			p1 = uxTaskPriorityGet(xTaskHandle1);
			p2 = uxTaskPriorityGet(xTaskHandle2);

			//switch_prio
			vTaskPrioritySet(xTaskHandle1, p2);
			vTaskPrioritySet(xTaskHandle2, p1);
		}
		else
		{
			GPIO_ToggleBits(GPIOA, GPIO_Pin_5);
			rtos_delay(1000);
		}
	}
}

static void prvSetupHardware(void)
{
	//setup Button and LED
	prvSetupGPIO();

	//setup UART2
	prvSetupUart();
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

void prvSetupGPIO(void)
{
//this function is board specific

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	GPIO_InitTypeDef led_init, button_init;
	led_init.GPIO_Mode = GPIO_Mode_OUT;
	led_init.GPIO_OType = GPIO_OType_PP;
	led_init.GPIO_Pin = GPIO_Pin_5;
	led_init.GPIO_Speed = GPIO_Low_Speed;
	led_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &led_init);

	button_init.GPIO_Mode = GPIO_Mode_IN;
	button_init.GPIO_OType = GPIO_OType_PP;
	button_init.GPIO_Pin = GPIO_Pin_13;
	button_init.GPIO_Speed = GPIO_Low_Speed;
	button_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &button_init);

	//interrupt configuration for the button (PC13)
	//1. System configuration for exti line (SYSCFG settings)
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource13);

	//2. EXTI line configuration 13, falling edge, interrupt mode
	EXTI_InitTypeDef exti_init;
	exti_init.EXTI_Line = EXTI_Line13;
	exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
	exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
	exti_init.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti_init);

	//3. NVIC settings (IRQ settings for the selected EXTI line 13)
	NVIC_SetPriority(EXTI15_10_IRQn, 5);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void printmsg(char *msg)
{
	for(uint32_t i=0; i < strlen(msg); i++)
	{
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) != SET);
		USART_SendData(USART2, msg[i]);
	}

	while( USART_GetFlagStatus(USART2, USART_FLAG_TC) != SET);
}

void rtos_delay(uint32_t delay_in_ms)
{
	uint32_t tick_count_local = xTaskGetTickCount();;

	//converting ms to ticks
	uint32_t delay_in_ticks = (delay_in_ms * configTICK_RATE_HZ ) / 1000;

	while(xTaskGetTickCount() < (tick_count_local + delay_in_ticks));
}


void EXTI15_10_IRQHandler(void)
{
	//1. Clear the interrupt pending bit of the EXTI line (13)
	EXTI_ClearITPendingBit(EXTI_Line13);
	switch_prio = TRUE;
}
