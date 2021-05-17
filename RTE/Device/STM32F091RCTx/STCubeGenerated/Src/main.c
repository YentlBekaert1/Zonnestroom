/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "i2c-lcd.h"
#include "i2c-lcd.c"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
DAC_HandleTypeDef hdac;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim16;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
	//uart variablen voor debug op seriele poort
  char uart_buf[50];
	int uart_buf_len;
	uint8_t UART2_rxBuffer[16] = {0};
	UART_HandleTypeDef huart2;
	char received_consumer_value[8] = {0};
	char received_pulses_value[8] = {0};
	//TIMER16	Variable
	uint16_t timer_val;
	
	//variablen voor pulsmeting
	volatile uint32_t ticks = 0;
	volatile uint32_t LastPulse = 0, TijdTussenPulsen = 0, CurrentPulse = 0;
	bool new_pulse = false;

	//In te stellen waarde van verbruiker voor overige productie
	int DAC_Voltage_Out_value;
	int DAC_Max_Voltage_Out_value = 4095;
	int waarde_apparaat = 2000;
	float send_percent;
	
	float Gemeten_KWh = 0.0;
	float Pulsen_Per_KWH = 1000;
	
	//variablen voor menu
	char screen = 0;
	char arrowpos = 0;
	char arrowpos_sensor = 0;
	int counter = 0;
	volatile bool Instellingen = false;
	volatile bool TurnDetected = false;
	volatile bool Up = false;
	volatile bool Button = false;
	int selected_sensor;
	
	volatile bool Programma_start = false;
	
	//variable om de gemeten waarden te resetten als er geen puls meer komt.
	uint8_t pulsen_counter = 0;
		
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM16_Init(void);
static void MX_DAC_Init(void);
/* USER CODE BEGIN PFP */
static void screen0(void);
static void screen1(void);
static void screen2(void);
static void screen3(void);
static void screen4(void);
static void screen5(void);
static void screen6(void);
static void screen7(void);
static void Notify_by_startup(void);
static void Berekenen_KWH_Waarden(void);
static void Aansturen_DAC(float kwh);

unsigned char Arrow_L[]= { 
	0x00,
  0x04,
  0x06,
  0x1F,
  0x1F,
  0x06,
  0x04,
  0x00
};

unsigned char Selector[]= { 
  0x02,
  0x06,
  0x0E,
  0x1E,
  0x1E,
  0x0E,
  0x06,
  0x02
};

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM16_Init();
  MX_DAC_Init();
  /* USER CODE BEGIN 2 */
	
		//LCD INIT
		HAL_Delay(500);
		lcd_init ();
		CreateCustomCharacter(Arrow_L,1);
		CreateCustomCharacter(Selector,2);
		
		/*HAL_Delay(500);
		lcd_put_cur(0,0);
		lcd_send_data(2);
		lcd_send_data(3);
		lcd_send_data(4);
		lcd_send_string(" Project");
		lcd_put_cur(1,0);
		lcd_send_data(5);
		lcd_send_data(6);
		lcd_send_data(7);
		lcd_send_string(" Zonnestroom");
		HAL_Delay(5000);*/
		lcd_clear();
		screen0();
		lcd_put_cur(0,0);
		lcd_send_data(1);
		HAL_Delay(500);
		
		//Start timer
		//clock = 48MHZ Prescaler = 4800 -1 => 10Khz = 1 tick per 0.0001 sec = 100us
		//clock = 1MHZ Prescaler = 10000 -1 => 100Hz = 1 tick per 0.001 sec = 10ms
		HAL_TIM_Base_Start_IT(&htim16);
	
		//start DAC
		HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
		
		//UART inturrupt
		HAL_UART_Receive_IT (&huart2, UART2_rxBuffer, 16);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {	
		/* read PB4 = de toggle swicth */
		if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4))
		{
			//start meten van kWh en aanturen DAC
			Programma_start = true;
		}else{
			//stop meten van kWh en aanturen DAC
			Programma_start = false;
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
			if (screen == 1){
				lcd_put_cur(0,6);
				lcd_send_string("       ");
				lcd_put_cur(1,7);
				lcd_send_string("    ");
		
			}
		}
		// lus die overlopen wordt als het programma gestart is
		if(Programma_start == true){
			if(new_pulse == true){
				Berekenen_KWH_Waarden();
				Aansturen_DAC(Gemeten_KWh);
				char prod[10];
				char outp[10];
				sprintf(prod, "%2.2f", Gemeten_KWh);
				sprintf(outp, "%.0f", send_percent*100);
				if(screen == 1){
					lcd_put_cur(0,6);
					lcd_send_string("       ");
					lcd_put_cur(0,6);
					lcd_send_string(prod);
					lcd_put_cur(1,7);
					lcd_send_string("    ");
					lcd_put_cur(1,7);
					lcd_send_string(outp);
				}
				new_pulse = false;
				pulsen_counter ++;
			}
		}
		// als er op de knop van de rotary encoder gedrukt wordt. Deze knop dient als enter
		if(Button == true){
			HAL_Delay(200);
			switch (screen){
				//Als je op de homescreen bent
				case 0:
					//waneer er op Settings wordt geklikt
					if(arrowpos == 0){
						screen = 2;
						screen2();
						lcd_put_cur(0,0);
						lcd_send_data(1);
						arrowpos = 0;
					}
					//waneer er op kWh measeurement wordt geklikt
					else{
						screen = 1;
						screen1();
						char prod[10];
						char outp[10];
						sprintf(prod, "%2.2f", Gemeten_KWh);
						sprintf(outp, "%.0f", send_percent*100);
						lcd_put_cur(0,6);
						lcd_send_string("       ");
						lcd_put_cur(0,6);
						lcd_send_string(prod);
						lcd_put_cur(1,7);
						lcd_send_string("    ");
						lcd_put_cur(1,7);
						lcd_send_string(outp);
					}
					break;
				//als je op de mesurement screen bent
				case 1:
						screen = 0;
						screen0();
						arrowpos = 0;
						lcd_put_cur(0,0);
						lcd_send_data(1);
				break;
				//als je op de settingsscreen bent
				case 2:
					switch(arrowpos){
						//instellen pulsen per kilowattuur
						case 0:
							screen = 3;
							screen3();
							arrowpos = 0;
						break;
						//instellen Verbruiker
						case 1:
							screen = 4;
							screen4();
							arrowpos = 1;
						break;
						//kiezen van sensor
						case 2:
							screen = 6;
							screen6();
							lcd_put_cur(0,0);
							lcd_send_data(1);
							arrowpos = 2;
							arrowpos_sensor = 0;
						break;
						//back
						case 3:
							screen = 0;
							screen0();
							arrowpos = 0;
							lcd_put_cur(0,0);
							lcd_send_data(1);
						break;
					}
				break;
				// als je op het scherm bent om de Pulsen per kWh in te stellen
				case 3:
					screen = 2;
					screen2();
					lcd_put_cur(0,0);
					lcd_send_data(1);
				break;
				// als je op het scherm bent om de Verbruiker in te stellen
				case 4:
					screen = 2;
					screen2();
					lcd_put_cur(1,0);
					lcd_send_data(1);
				break;
				//als je op de sensor settingsscreen bent
				case 6:
					switch(arrowpos_sensor){
						//Kies sensor 1
						case 0:
							selected_sensor = 1;
							printf("S: %i\n", selected_sensor);
							screen6();
							lcd_put_cur(0,0);
							lcd_send_data(1);
						break;
						//Kies sensor 2
						case 1:
							selected_sensor = 2;
							printf("S: %i\n", selected_sensor);
							screen6();
							lcd_put_cur(1,0);
							lcd_send_data(1);
						break;
						//Kies sensor 3
						case 2:
							selected_sensor = 3;
							printf("S: %i\n", selected_sensor);
							screen7();
							lcd_put_cur(0,0);
							lcd_send_data(1);
						break;
						//back to settingscreen
						case 3:
							screen = 2;
							screen5();
							arrowpos = 2;
							lcd_put_cur(0,0);
							lcd_send_data(1);
						break;
					}
				break;
			}
			Button = false;
		}
		//Als er aan de knop van de rotary encoder gedraaid wordt. Hiermee kun je omhoog of omlaag gaan.
		if(TurnDetected == true){
			HAL_Delay(200);
			switch(screen){
				//if homecreen
				case 0:
					switch (arrowpos){
						//positie om naar settings te gaan
						case 0:
							if(!Up){
								screen0();
								lcd_put_cur(1,0);
								lcd_send_data(1);
								arrowpos = 1;
							}
							break;
						//positie om naar kWh leasurement te gaan
						case 1:
							if(Up){
								screen0();
								lcd_put_cur(0,0);
								lcd_send_data(1);
								arrowpos = 0;
							}
							break;
					}
				break;
				//if settingsscreen
				case 2:
					//positie om naar Pulsen per kWh te gaan
					switch (arrowpos){
						case 0:
							if(!Up){
								screen2();
								lcd_put_cur(1,0);
								lcd_send_data(1);
								arrowpos = 1;
							}
							break;
						//positie om naar Verbruiker te gaan
						case 1:
							if(Up){
								screen2();
								lcd_put_cur(0,0);
								lcd_send_data(1);
								arrowpos = 0;
							}else{
								screen5();
								lcd_put_cur(0,0);
								lcd_send_data(1);
								arrowpos = 2;
							}
						break;
						//positie om naar Sensor settings te gaan
						case 2:
							if(Up){
								screen2();
								lcd_put_cur(1,0);
								lcd_send_data(1);
								arrowpos = 1;
							}else{
								screen5();
								lcd_put_cur(1,0);
								lcd_send_data(1);
								arrowpos = 3;
							}
						break;
						//positie om naar Terug te gaan te gaan
						case 3:
							if(Up){
								screen5();
								lcd_put_cur(0,0);
								lcd_send_data(1);
								arrowpos = 2;
							}
						break;
					}
					break;
					//if settingsscreen Pulsen per kWh
					case 3:
						if(Up){
							Pulsen_Per_KWH = Pulsen_Per_KWH + 100;
							char pulsen[10];
							sprintf(pulsen, "%.f",Pulsen_Per_KWH);
							lcd_put_cur(1,1);
							lcd_send_string("         ");
							lcd_put_cur(1,1);
							lcd_send_string(pulsen);
							printf("P: %i\n", (int)Pulsen_Per_KWH);
							
						}else{
							Pulsen_Per_KWH = Pulsen_Per_KWH - 100;
							if(Pulsen_Per_KWH < 100){
								Pulsen_Per_KWH = 0;
							}
							char pulsen[10];
							sprintf(pulsen, "%.f",Pulsen_Per_KWH);
							lcd_put_cur(1,1);
							lcd_send_string("         ");
							lcd_put_cur(1,1);
							lcd_send_string(pulsen);
							printf("P: %i\n", (int)Pulsen_Per_KWH);
						}
					break;
					//if settingsscreen Verbuiker
					case 4:
						if(Up){
							waarde_apparaat = waarde_apparaat + 100;
							char consumer[10];
							sprintf(consumer, "%0.i",waarde_apparaat);
							lcd_put_cur(1,1);
							lcd_send_string("         ");
							lcd_put_cur(1,1);
							lcd_send_string(consumer);
							printf("C: %i\n", waarde_apparaat);
							
						}else{
							waarde_apparaat = waarde_apparaat - 100;
							if(waarde_apparaat < 100){
								waarde_apparaat = 0;
							}
							char consumer[10];
							sprintf(consumer, "%0.i",waarde_apparaat);
							lcd_put_cur(1,1);
							lcd_send_string("         ");
							lcd_put_cur(1,1);
							lcd_send_string(consumer);
							printf("C: %i\n", waarde_apparaat);
						}
					break;
					// als je op het scherm bent om senor te kiezen
					case 6:
					//positie om naar Pulsen per kWh te gaan
					switch (arrowpos_sensor){
						//positie om Sensor1 te kiezen
						case 0:
							if(!Up){
								screen6();
								lcd_put_cur(1,0);
								lcd_send_data(1);
								arrowpos_sensor = 1;
							}
							break;
						//positie om Sensor2 te kiezen
						case 1:
							if(Up){
								screen6();
								lcd_put_cur(0,0);
								lcd_send_data(1);
								arrowpos_sensor = 0;
							}else{
								screen7();
								lcd_put_cur(0,0);
								lcd_send_data(1);
								arrowpos_sensor = 2;
							}
						break;
						//positie om Sensor3 te kiezen
						case 2:
							if(Up){
								screen6();
								lcd_put_cur(1,0);
								lcd_send_data(1);
								arrowpos_sensor = 1;
							}else{
								screen7();
								lcd_put_cur(1,0);
								lcd_send_data(1);
								arrowpos_sensor = 3;
							}
						break;
						//positie om naar Terug te gaan 
						case 3:
							if(Up){
								screen7();
								lcd_put_cur(0,0);
								lcd_send_data(1);
								arrowpos_sensor = 2;
							}
						break;
					}
					break;
			}
			TurnDetected = false;
		}	
	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV16;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_HSI;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{

  /* USER CODE BEGIN DAC_Init 0 */

  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */

  /* USER CODE END DAC_Init 1 */
  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 10000 - 1;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 50000- 1;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : B1_Pin PC11_Pin */
  GPIO_InitStruct.Pin = B1_Pin|PC11_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RE_OUTA_Pin RE_OUTB_Pin */
  GPIO_InitStruct.Pin = RE_OUTA_Pin|RE_OUTB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : Button1_Pin */
  GPIO_InitStruct.Pin = Button1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Button1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

}

/* USER CODE BEGIN 4 */
void screen0(){
	lcd_clear();
	lcd_put_cur(0,1);
	lcd_send_string("Settings");
	lcd_put_cur(1,1);
	lcd_send_string("kWh measurement");
}
void screen1(){
	lcd_clear();
	lcd_put_cur(0,0);
	lcd_send_string("Value");
	lcd_put_cur(0,13);
	lcd_send_string("kWh");
	lcd_put_cur(1,0);
	lcd_send_string("Output");
	lcd_put_cur(1,13);
	lcd_send_string("%");
}
void screen2(){
	lcd_clear();
	lcd_put_cur(0,1);
	lcd_send_string("Pulses per KWH");
	lcd_put_cur(1,1);
	lcd_send_string("Consumer");
}

void screen3(){
	char pulsen[10];
	sprintf(pulsen, "%.f",Pulsen_Per_KWH);
	
	lcd_clear();
	lcd_put_cur(0,1);
	lcd_send_string("Pulses per KWH");
	lcd_put_cur(1,1);
	lcd_send_string(pulsen);
}
void screen4(){
	char consumer[10];
	sprintf(consumer, "%.i" ,waarde_apparaat);
	lcd_clear();
	lcd_put_cur(0,1);
	lcd_send_string("Consumer (W)");
	lcd_put_cur(1,1);
	lcd_send_string(consumer);
}
void screen5(){
	lcd_clear();
	lcd_put_cur(0,1);
	lcd_send_string("Sensor");
	lcd_put_cur(1,1);
	lcd_send_string("Back");
}
void screen6(){
	lcd_clear();
	lcd_put_cur(0,1);
	lcd_send_string("Sensor1");
	lcd_put_cur(1,1);
	lcd_send_string("Sensor2");
	if(selected_sensor == 1){
		lcd_put_cur(0,15);
		lcd_send_data(2);
	}
	if(selected_sensor == 2){
		lcd_put_cur(1,15);
		lcd_send_data(2);
	}
}
void screen7(){
	lcd_clear();
	lcd_put_cur(0,1);
	lcd_send_string("Sensor3");
	lcd_put_cur(1,1);
	lcd_send_string("Back");
	if(selected_sensor == 3){
		lcd_put_cur(0,15);
		lcd_send_data(2);
	}
}
void Notify_by_startup(){
		lcd_put_cur(0,0);
		lcd_send_string("Please look first to settings                            ");
		HAL_Delay(1000);
	lcd_put_cur(0,0);
		lcd_send_string("lease look first to settings                            ");
		HAL_Delay(200);
	lcd_put_cur(0,0);
		lcd_send_string("ease look first to settings                            ");
		HAL_Delay(200);
	lcd_put_cur(0,0);
		lcd_send_string("ase look first to settings                            ");
		HAL_Delay(200);
	lcd_put_cur(0,0);
		lcd_send_string("e look first to settings                            ");
		HAL_Delay(200);
	lcd_put_cur(0,0);
		lcd_send_string(" look first to settings                            ");
		HAL_Delay(200);
	lcd_put_cur(0,0);
		lcd_send_string("look first to settings                            ");
		HAL_Delay(200);
		lcd_put_cur(0,0);
		lcd_send_string("ook first to settings                            ");
		HAL_Delay(200);
		lcd_put_cur(0,0);
		lcd_send_string("ok first to settings                            ");
		HAL_Delay(200);
		lcd_put_cur(0,0);
		lcd_send_string("k first to settings                            ");
		HAL_Delay(200);
		lcd_put_cur(0,0);
		lcd_send_string(" first to settings                            ");
		HAL_Delay(200);
		lcd_put_cur(0,0);
		lcd_send_string("first to settings                            ");
		HAL_Delay(200);
		lcd_put_cur(0,0);
		lcd_send_string("irst to settings                            ");
		HAL_Delay(200);
		lcd_put_cur(0,0);
		lcd_send_string("rst to settings                            ");
		HAL_Delay(200);
		lcd_put_cur(0,0);
		lcd_send_string("st to settings                            ");
		HAL_Delay(200);
		lcd_put_cur(0,0);
		lcd_send_string("t to settings                            ");
		HAL_Delay(200);
		lcd_put_cur(0,0);
		lcd_send_string(" to settings                            ");
		HAL_Delay(200);
		lcd_put_cur(0,0);
		lcd_send_string("to settings                            ");
		HAL_Delay(2000);
		
}

//BESTUREN DAC
void Aansturen_DAC(float kwh){
		//bepalen hoeveel percent van de uitgang van de DAC mag aangestuurd worden.
		//*100 omdat waarde_apparaat voorlopig in W staat idpv KW
		send_percent = ((float)kwh*1000)/(float)waarde_apparaat;
		if(send_percent > 1){
			send_percent = 1;
		}
		DAC_Voltage_Out_value = DAC_Max_Voltage_Out_value*send_percent;
	
		//verander de value van de DAC
		if(send_percent>1){
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, DAC_Max_Voltage_Out_value);
		}else{
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, DAC_Voltage_Out_value);
		}
		//print naar de seriele poort
		//printf("The float value : %f\n", send_percent);
		printf("D: %.f\n", send_percent*100);
}

//Bereken waarde KWH meter
void Berekenen_KWH_Waarden(void){
		float waarde = 360000/Pulsen_Per_KWH;
		Gemeten_KWh = (float)waarde/TijdTussenPulsen;
		printf("K: %f\n", Gemeten_KWh);
		//printf("tijd tussen pulsen: %d\n", TijdTussenPulsen);
}

	
//UART
int fputc (int ch, FILE *f)
{
	HAL_UART_Transmit(&huart2,(uint8_t*)&ch,1,HAL_MAX_DELAY);
	return ch;
}

int fgetc(FILE *f)
{
	int ch = 0;
	HAL_UART_Receive(&huart2,(uint8_t*)&ch,1,HAL_MAX_DELAY);
	return		ch;
}



//Externe interrputs
void HAL_GPIO_EXTI_Callback( uint16_t GPIO_Pin)
{
	//Interrupt Button 1 button 
	if(GPIO_Pin == Button1_Pin){
		 Button = true;
	}
	if(GPIO_Pin == RE_OUTA_Pin){
		 TurnDetected = true;
		 //Up = (HAL_GPIO_ReadPin(RE_OUTA_GPIO_Port,RE_OUTA_Pin) == HAL_GPIO_ReadPin(RE_OUTB_GPIO_Port,RE_OUTB_Pin));
		if(HAL_GPIO_ReadPin(RE_OUTB_GPIO_Port,RE_OUTB_Pin)){
			Up = HAL_GPIO_ReadPin(RE_OUTA_GPIO_Port,RE_OUTA_Pin);
		}else{
			Up = !HAL_GPIO_ReadPin(RE_OUTA_GPIO_Port,RE_OUTA_Pin);
		}
	}

	
	//Interrupt op externe pin PC11 meten van Kwh
	if(GPIO_Pin == PC11_Pin){
		
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		
		ticks = __HAL_TIM_GET_COUNTER(&htim16);
		
		// Sla het huidige 'uur' op.
		CurrentPulse = ticks;
		TijdTussenPulsen =   CurrentPulse - LastPulse ;
		LastPulse = CurrentPulse;
		
		new_pulse = true;
		
	}
	
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
   // HAL_UART_Transmit(&huart2, UART2_rxBuffer, 8, 100);
    HAL_UART_Receive_IT(&huart2, UART2_rxBuffer, 16);
		//als er c wordt ontvanfen (ACII code) lees dan de waarden en verwerk ze
		if(UART2_rxBuffer[0] == 99){	
			/*lcd_clear();
			lcd_put_cur(0,0);
			lcd_send_string((char*)UART2_rxBuffer);*/
			int i = 0;
			int index1 = 0;
			int index2 = 0;
			for(i=0; i<16 ; i++){
				if(0<i && i<8){
					if(UART2_rxBuffer[i] !=63){
					received_consumer_value[index1] = UART2_rxBuffer[i];
					index1++;					
					
				}
			}
				else if(i>9){
					if(UART2_rxBuffer[i] !=63){
					received_pulses_value[index2] = UART2_rxBuffer[i];
					index2++;
					}
				}
			}
			int received_consumer;
			sscanf(received_consumer_value, "%d", &received_consumer);
			waarde_apparaat = received_consumer;
			int received_pulses;
			sscanf(received_pulses_value, "%d", &received_pulses);
			Pulsen_Per_KWH = received_pulses;
			/*char outp[10];
			sprintf(outp, "%d", received_consumer);
			lcd_put_cur(1,0);
			lcd_send_string(outp);
			lcd_put_cur(1,8);
			lcd_send_string((char*)received_pulses_value);*/
		}
		//als er Q wordt ontvangen (ACII code) stuur dan de values van pulsen en waarde apparaat door.
		if(UART2_rxBuffer[0] == 81){
			printf("P:%i\n", (int)Pulsen_Per_KWH);
			printf("C:%i\n", waarde_apparaat);
			printf("S:%i\n", selected_sensor);
		}
}


//timer 16 interrupt
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim16){
		if(pulsen_counter < 1 && Programma_start == true){
				Gemeten_KWh = 0;
				Berekenen_KWH_Waarden();
				Aansturen_DAC(Gemeten_KWh);
				printf("K: %f\n", Gemeten_KWh);
				if (screen == 1){
					lcd_put_cur(0,6);
					lcd_send_string("       ");
					lcd_put_cur(0,6);
					lcd_send_data(48);
					lcd_put_cur(1,7);
					lcd_send_string("    ");
					lcd_put_cur(1,7);
					lcd_send_data(48);
				}
		}
		pulsen_counter = 0;
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
