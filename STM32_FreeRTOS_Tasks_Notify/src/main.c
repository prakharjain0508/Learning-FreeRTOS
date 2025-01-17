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
#define NOT_PRESSED FALSE
#define PRESSED TRUE

TaskHandle_t xTaskHandle1 = NULL;
TaskHandle_t xTaskHandle2 = NULL;

//function prototypes
static void prvSetupHardware(void);
static void prvSetupUart(void);
void prvSetupGPIO(void);
void printmsg(char *msg);
void rtos_delay(uint32_t delay_in_ms);

void vtask_led_handler(void *params);
void vtask_button_handler(void *params);


char usr_msg[250] = {0};
//global space for some variables
uint8_t button_status_flag = NOT_PRESSED;

int main(void)
{
	DWT->CTRL |= (1 << 0); 	//Enable CYCCNT in DWL_CTRL //Used for segger to maintain the timestamp

	//1. Resets the RCC clock configuration to the default reset state.
	//HSI ON, PLL OFF, HSE OFF, System clock = 16 MHz, cpu_clock = 16 MHz
	RCC_DeInit();

	//2. Update the SystenCoreClock Variable
	SystemCoreClockUpdate();

	prvSetupHardware();

	sprintf(usr_msg, "This is Demo of Task Notify APIs\r\n");
	printmsg(usr_msg);

	//Start recording
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//lets create led_task
	xTaskCreate(vtask_led_handler, "TASK-LED", 500, NULL, 2, &xTaskHandle1);
	xTaskCreate(vtask_button_handler, "TASK-Button", 500, NULL, 2, &xTaskHandle2);

	//let start scheduler
	vTaskStartScheduler();

	for(;;);
}

void vtask_led_handler(void *params)
{
	uint32_t current_notification_value = 0;
	while(1)
	{
		//lets wait until we receive any notification from button_task
		if(xTaskNotifyWait(0, 0, &current_notification_value, portMAX_DELAY) == pdTRUE)
		{
			//we received the notification. So lets toggle the LED
			GPIO_ToggleBits(GPIOA, GPIO_Pin_5);
			sprintf(usr_msg, "Notification is received : Button press count : %ld \r\n", current_notification_value);
			printmsg(usr_msg);
		}
	}
}

void vtask_button_handler(void *params)
{
	while(1)
	{
		if(! GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) )
		{
			//we will come here if button is pressed

			//lets wait here for 100ms to compensate for the button debouncing
			rtos_delay(200);

			//lets send the notification to led_task
			xTaskNotify(xTaskHandle1, 0x0, eIncrement);
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
	//setup Button and LED
	prvSetupGPIO();

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

	while( USART_GetFlagStatus(USART2, USART_FLAG_TC) != SET);
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

}


void rtos_delay(uint32_t delay_in_ms)
{
	uint32_t current_tick_count = xTaskGetTickCount();
	uint32_t delay_in_ticks = (delay_in_ms * configTICK_RATE_HZ ) / 1000;

	while(xTaskGetTickCount() < (current_tick_count + delay_in_ticks));
}
