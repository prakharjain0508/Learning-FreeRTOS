/**
  ******************************************************************************
  * @file    main.c
  * @author  Prakhar Jain
  * @brief   Default main function.
  ******************************************************************************
*/
#include<string.h>
#include<stdint.h>
#include<stdio.h>
#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#define TRUE 							1
#define FALSE 							0
#define LED_ON_COMMAND 					1
#define LED_OFF_COMMAND				 	2
#define LED_TOGGLE_COMMAND 				3
#define LED_TOGGLE_STOP_COMMAND 		4
#define LED_READ_STATUS_COMMAND 		5
#define RTC_READ_DATE_TIME_COMMAND 		6

//function prototypes
static void prvSetupHardware(void);
static void prvSetupUart(void);
void prvSetupGPIO(void);
void printmsg(char *msg);
void rtos_delay(uint32_t delay_in_ms);
uint8_t getCommandCode(uint8_t *buffer);
void getArguments(uint8_t *buffer);

//prototypes command helper functions
void make_led_on(void);
void make_led_off(void);
void led_toggle_start(uint32_t duration);
void led_toggle_stop(void);
void read_led_status(char *task_msg);
void read_rtc_info(char *task_msg);
void print_error_message(char *task_msg);

//tasks prototypes
void vTask1_menu_display(void *params);
void vTask2_cmd_handling(void *params);
void vTask3_cmd_processing(void *params);
void vTask4_uart_write(void *params);

//software timer callback function prototye
void led_toggle(TimerHandle_t xTimer);

//global space for some variables
char usr_msg[250] = {0};

//Task handles
TaskHandle_t xTaskHandle1 = NULL;
TaskHandle_t xTaskHandle2 = NULL;
TaskHandle_t xTaskHandle3 = NULL;
TaskHandle_t xTaskHandle4 = NULL;

//Queue handles
QueueHandle_t command_queue = NULL;
QueueHandle_t uart_write_queue = NULL;

//software timer handle
TimerHandle_t led_timer_handle = NULL;

//command structure
typedef struct APP_CMD
{
	uint8_t COMMAND_NUM;
	uint8_t COMMAND_ARGS[10];
}APP_CMD_t;

uint8_t command_buffer[20];
uint8_t command_len = 0;

//This is the menu
char menu[]={"\
\r\nLED_ON						----> 1 \
\r\nLED_OFF						----> 2 \
\r\nLED_TOGGLE					----> 3 \
\r\nLED_TOGGLE_OFF					----> 4 \
\r\nLED_READ_STATUS					----> 5 \
\r\nRTC_PRINT_DATETIME				----> 6 \
\r\nEXIT_APP					----> 0 \
\r\nType your option here : "};


int main(void)
{
	DWT->CTRL |= (1 << 0); 	//Enable CYCCNT in DWL_CTRL //Used for segger to maintain the timestamp

	//1. Resets the RCC clock configuration to the default reset state.
	//HSI ON, PLL OFF, HSE OFF, System clock = 16 MHz, cpu_clock = 16 MHz
	RCC_DeInit();

	//2. Update the SystenCoreClock Variable
	SystemCoreClockUpdate();

	prvSetupHardware();

	sprintf(usr_msg, "\r\nThis is Queue Command Processing Demo\r\n");
	printmsg(usr_msg);

	//lets create command queue
	command_queue = xQueueCreate(10, sizeof(APP_CMD_t*));  	//a memory pointer is 32-bit or 4-bytes

	//lets create the write queue
	uart_write_queue = xQueueCreate(10, sizeof(char*));  	//a memory pointer is 32-bit or 4-bytes

	if((command_queue != NULL) && (uart_write_queue != NULL))
	{
		//lets create Task-1
		xTaskCreate(vTask1_menu_display, "TASK1-MENU", 500, NULL, 1, &xTaskHandle1);

		//lets create Task-2
		xTaskCreate(vTask2_cmd_handling, "TASK2-CMD-HANDLING", 500, NULL, 2, &xTaskHandle2);

		//lets create Task-3
		xTaskCreate(vTask3_cmd_processing, "TASK3-CMD-PROCESS", 500, NULL, 2, &xTaskHandle3);

		//lets create Task-4
		xTaskCreate(vTask4_uart_write, "TASK4-UART-WRITE", 500, NULL, 2, &xTaskHandle4);

		//let start scheduler
		vTaskStartScheduler();
	}else
	{
		sprintf(usr_msg, "Queue creation failed!");
		printmsg(usr_msg);
	}



	for(;;);
}

void vTask1_menu_display(void *params)
{
	char *pData = menu;
	while(1)
	{
		xQueueSend(uart_write_queue, &pData, portMAX_DELAY);

		//lets wait here until someone notifies
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
	}
}

void vTask2_cmd_handling(void *params)
{
	uint8_t command_code = 0;

	APP_CMD_t *new_cmd;

	while(1)
	{
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		//1. send command to queue
		new_cmd = (APP_CMD_t*) pvPortMalloc(sizeof(APP_CMD_t));

		taskENTER_CRITICAL();
		command_code = getCommandCode(command_buffer);
		new_cmd->COMMAND_NUM = command_code;
		getArguments(new_cmd->COMMAND_ARGS);
		taskEXIT_CRITICAL();

		//send the command to the command queue
		xQueueSend(command_queue, &new_cmd, portMAX_DELAY);
	}
}

void vTask3_cmd_processing(void *params)
{
	APP_CMD_t *new_cmd;
	char task_msg[50];

	uint32_t toggle_duration = pdMS_TO_TICKS(500);

	while(1)
	{
		xQueueReceive(command_queue, (void*)&new_cmd, portMAX_DELAY);

		if(new_cmd->COMMAND_NUM == LED_ON_COMMAND)
		{
			make_led_on();
		}
		else if(new_cmd->COMMAND_NUM == LED_OFF_COMMAND)
		{
			make_led_off();
		}
		else if(new_cmd->COMMAND_NUM == LED_TOGGLE_COMMAND)
		{
			led_toggle_start(toggle_duration);
		}
		else if(new_cmd->COMMAND_NUM == LED_TOGGLE_STOP_COMMAND)
		{
			led_toggle_stop();
		}
		else if(new_cmd->COMMAND_NUM == LED_READ_STATUS_COMMAND)
		{
			read_led_status(task_msg);
		}
		else if(new_cmd->COMMAND_NUM == RTC_READ_DATE_TIME_COMMAND)
		{
			read_rtc_info(task_msg);
		}
		else
		{
			print_error_message(task_msg);
		}

		//lets free the allocated memory for the new command
		vPortFree(new_cmd);

	}
}

void vTask4_uart_write(void *params)
{
	char *pData = NULL;
	while(1)
	{
		xQueueReceive(uart_write_queue, &pData, portMAX_DELAY);
		printmsg(pData);

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
	gpio_uart_pins.GPIO_OType= GPIO_OType_PP;
	gpio_uart_pins.GPIO_Speed = GPIO_High_Speed;
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

	//lets enable the UART byte reception interrupt in the microcontroller
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

	//lets set the priority in NVC for the UART2 interrupt
	NVIC_SetPriority(USART2_IRQn, 5);

	//enable the UART2 IRQin the NVIC
	NVIC_EnableIRQ(USART2_IRQn);

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
	//1. system configuration for exti line (SYSCFG settings)
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC,EXTI_PinSource13);

	//2. EXTI line configuration 13,falling edge, interrup mode
	EXTI_InitTypeDef exti_init;
	exti_init.EXTI_Line = EXTI_Line13;
	exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
	exti_init.EXTI_Trigger = EXTI_Trigger_Falling;
	exti_init.EXTI_LineCmd = ENABLE;
	EXTI_Init(&exti_init);

	//3. NVIC settings (IRQ settings for the selected EXTI line(13)
	NVIC_SetPriority(EXTI15_10_IRQn,5);
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
	uint32_t tick_count_local = 0;

	//converting ms to ticks
	uint32_t delay_in_ticks = (delay_in_ms * configTICK_RATE_HZ ) / 1000;

	tick_count_local = xTaskGetTickCount();
	while(xTaskGetTickCount() < (tick_count_local + delay_in_ticks));
}

void USART2_IRQHandler(void)
{
	uint16_t data_byte;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE))
	{
		//a data byte is received from the user
		data_byte = USART_ReceiveData(USART2);

		command_buffer[command_len++] = (data_byte & 0xFF); 	//bit masking as im interested only in the last 8 bits

		if(data_byte == '\r')
		{
			//then user is finished entering the data

			//reset the command_len variable
			command_len = 0;

			//lets notify the command handling task
			xTaskNotifyFromISR(xTaskHandle2, 0, eNoAction, &xHigherPriorityTaskWoken);

			xTaskNotifyFromISR(xTaskHandle1, 0, eNoAction, &xHigherPriorityTaskWoken);
		}
	}

	//if the above freertos APIs wake up any higher priority task, then yield the processor to the higher
	//priority task which is just woken up
	if(xHigherPriorityTaskWoken)
	{
		taskYIELD();
	}

}

uint8_t getCommandCode(uint8_t *buffer)
{
	return buffer[0]-48;
}

void getArguments(uint8_t *buffer)
{

}

void make_led_on(void)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_SET);
}

void make_led_off(void)
{
	GPIO_WriteBit(GPIOA, GPIO_Pin_5, Bit_RESET);
}

void led_toggle(TimerHandle_t xTimer)
{
	GPIO_ToggleBits(GPIOA, GPIO_Pin_5);
}

void led_toggle_start(uint32_t duration)
{
	if(led_timer_handle == NULL)
	{
		//1. Lets create the software timer
		led_timer_handle = xTimerCreate("LED-TIMER", duration, pdTRUE, NULL, led_toggle);
	}

	//2. Start the software timer
	xTimerStart(led_timer_handle, portMAX_DELAY);
}

void led_toggle_stop(void)
{
	xTimerStop(led_timer_handle, portMAX_DELAY);
}

void read_led_status(char *task_msg)
{
	sprintf(task_msg, "\r\nLED status is: %d\r\n", GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_5));
	xQueueSend(uart_write_queue, &task_msg, portMAX_DELAY);
}

void read_rtc_info(char *task_msg)
{
	RTC_TimeTypeDef RTC_time;
	RTC_DateTypeDef RTC_date;
	//read time and date from RTC peripheral of the microcontroller
	RTC_GetTime(RTC_Format_BIN, &RTC_time);
	RTC_GetDate(RTC_Format_BIN, &RTC_date);

	sprintf(task_msg,"\r\nTime: %02d:%02d:%02d \r\n Date : %02d-%2d-%2d \r\n",RTC_time.RTC_Hours,RTC_time.RTC_Minutes,RTC_time.RTC_Seconds, \
									RTC_date.RTC_Date,RTC_date.RTC_Month,RTC_date.RTC_Year );
	xQueueSend(uart_write_queue,&task_msg,portMAX_DELAY);
}

void print_error_message(char *task_msg)
{
	sprintf(task_msg, "\r\nInvalid command received\r\n");
	xQueueSend(uart_write_queue, &task_msg, portMAX_DELAY);
}
