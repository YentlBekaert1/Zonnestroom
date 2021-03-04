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

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim16;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
	//uart variablen voor debug op seriele poort
  char uart_buf[50];
	int uart_buf_len;
	
	//TIMER16	Variable
	uint16_t timer_val;
	
	//variablen voor pulsmeting
	volatile uint32_t ticks = 0;
	volatile uint32_t LastPulse = 0, TijdTussenPulsen = 0, CurrentPulse = 0;

	//In te stellen waarde van verbruiker voor overige productie
	int DAC_Voltage_Out_value;
	int DAC_Max_Voltage_Out_value = 4095;
	int waarde_apparaat = 2000;
	float send_percent;
	
	float Gemeten_KWh = 0.0;
	float Pulsen_Per_KWH = 1000;
	
	//variablen voor menu
	bool Open_menu = false;
	bool Menu_Enter = false;
	bool start_up_menu = true;
	bool enter = false;
	
	int Rotery_encoder_counter = 0;  
	
	char Menu_Items[3] [40] =
		{ "Pulsen per KW  ",
			"Vermogen verbr.",
			"Close Menu     ",
		};
	int Selected_Menu_Item = 0;
	int new_value = 0;
	bool instellen_verbuiker = false;
	bool instellen_pulsen = false;

		
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM16_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */
static void LCD_Startup(void);
static void LCD_Update(float Gemeten_KWh, int output_DAC);
static void Berekenen_KWH_Waarden(void);
static void Aansturen_DAC(float kwh);
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
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
	
		//LCD INIT
		HAL_Delay(500);
		lcd_init ();
		HAL_Delay(500);
		LCD_Startup();
		HAL_Delay(500);
		
		//Start timer
		//clock = 48MHZ Prescaler = 4800 -1 => 10Khz = 1 tick per 0.0001 sec = 100us
		HAL_TIM_Base_Start_IT(&htim16);
	
		//start DAC
		HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
		
		HAL_TIM_Encoder_Start(&htim1,TIM_CHANNEL_ALL); 
		//timer 1 voor rotary encoder
		
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		/*if(start_up_menu == true){
			Menu();
			start_up_menu = false;
		}*/
		
		Berekenen_KWH_Waarden();
		Aansturen_DAC(Gemeten_KWh);
		LCD_Update(Gemeten_KWh, send_percent*100);
		HAL_Delay(100);
		
		
		while(Open_menu == true){
		lcd_put_cur(0,0);
		lcd_send_string("Selecteer Item: ");
		lcd_put_cur(1,0);
		lcd_send_string(Menu_Items[Selected_Menu_Item]);
		
		Rotery_encoder_counter = TIM1->CNT;  
		
		if(new_value != Rotery_encoder_counter){
			new_value = Rotery_encoder_counter;
			if(Selected_Menu_Item == 2){
				Selected_Menu_Item = 0;
			}else{
					Selected_Menu_Item++;
			}				
			
			lcd_clear();
		}
		while(instellen_pulsen == true){
			lcd_clear();
			bool on = true;
			int Pulsen_Per_KWH_tijdelijk = 0;
			while(on == true){
				if (TIM1->CNT < 40000){
					Pulsen_Per_KWH_tijdelijk  = Pulsen_Per_KWH + TIM1->CNT*100;  
				}
				lcd_put_cur(0,0);
				lcd_send_string("Pulsen per kWh");
				lcd_put_cur(1,0);
				char verb[10];
				sprintf(verb, "%7i", Pulsen_Per_KWH_tijdelijk);
				lcd_send_string(verb);
			
				if(enter == true)
				{
					enter = false;
					HAL_GPIO_EXTI_IRQHandler(Button1_Pin);
					Pulsen_Per_KWH = (float)Pulsen_Per_KWH_tijdelijk;
					printf("close instellen verbr.");
					on = false;
					instellen_pulsen = false;
				}
			}
		}
		
		while(instellen_verbuiker == true){
			lcd_clear();
			bool on = true;
			int waarde_verbruiker_tijdelijk = waarde_apparaat;
			while(on == true){
				waarde_verbruiker_tijdelijk = TIM1->CNT*100;  
				lcd_put_cur(0,0);
				lcd_send_string("Waarde verbr. (W)");
				lcd_put_cur(1,0);
				char verb[10];
				sprintf(verb, "%7i", waarde_verbruiker_tijdelijk);
				lcd_send_string(verb);
				if(enter == true)
				{
					enter = false;
					HAL_GPIO_EXTI_IRQHandler(Button1_Pin);
					waarde_apparaat = waarde_verbruiker_tijdelijk;
					printf("close instellen verbr.");
					on = false;
					instellen_verbuiker = false;
				}
			}
		}
		
		//enter button ingdrukt
		if(enter == true){
			printf("Menu enter\n");
			enter = false;
			
			if(Selected_Menu_Item == 0){
				instellen_pulsen = true;
				//printf("Insettel pulsen per kwh\n");
			}
			
			if(Selected_Menu_Item == 1){
				//Open_menu = false;
				instellen_verbuiker = true;
				//printf("instellen verbruiker\n");
			}
			
			//close menu
			if(Selected_Menu_Item == 2){
				Open_menu = false;
				LCD_Startup();
				printf("close_Menu\n");
			}
		}
	}
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
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
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
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
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 4;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 50000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 10;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 10;
  if (HAL_TIM_Encoder_Init(&htim1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

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
  htim16.Init.Prescaler = 4800 - 1;
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
  huart2.Init.BaudRate = 115200;
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

  /*Configure GPIO pin : Button1_Pin */
  GPIO_InitStruct.Pin = Button1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(Button1_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

}

/* USER CODE BEGIN 4 */
void LCD_Update(float Gemeten_KWh, int output_DAC){
  char prod[10];
	char verb[10];
  sprintf(prod, "%2.2f", Gemeten_KWh);
	sprintf(verb, "%3d", output_DAC);
	
	/*lcd_put_cur(0,7);
	lcd_send_string("     ");
	lcd_put_cur(1,7);
	lcd_send_string("     ");*/
	lcd_put_cur(0,8);
	lcd_send_string(prod);
	lcd_put_cur(1,9);
	lcd_send_string(verb);
}

void LCD_Startup(void){
	lcd_clear();
	lcd_put_cur(0,0);
	lcd_send_string("Gemeten");
	lcd_put_cur(0,8);
	lcd_send_string("0");
	lcd_put_cur(0,13);
	lcd_send_string("KWh");
	lcd_put_cur(1,0);
	lcd_send_string("Output");
	lcd_put_cur(1,11);
	lcd_send_string("0");
	lcd_put_cur(1,13);
	lcd_send_string("%");
}
//BESTUREN DAC
void Aansturen_DAC(float kwh){
		//bepalen hoeveel percent van de uitgang van de DAC mag aangestuurd worden.
		//*100 omdat waarde_apparaat voorlopig in W staat idpv KW
		send_percent = ((float)kwh*1000)/(float)waarde_apparaat;
		DAC_Voltage_Out_value = DAC_Max_Voltage_Out_value*send_percent;
	
		//verander de value van de DAC
		if(send_percent>1){
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, DAC_Max_Voltage_Out_value);
		}else{
			HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, DAC_Voltage_Out_value);
		}
		//print naar de seriele poort
		printf("The float value : %f\n", send_percent);
		printf("DAC_Voltage_Out_value : %i\n", DAC_Voltage_Out_value);
}

//Bereken waarde KWH meter
void Berekenen_KWH_Waarden(void){
		float waarde = 3600000/Pulsen_Per_KWH;
		Gemeten_KWh = (float)waarde/TijdTussenPulsen;
		printf("The gemeten KWh : %f\n\n", Gemeten_KWh);
		printf("tijd tussen pulsen: %d\n\n", TijdTussenPulsen);
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
			
			if(Open_menu == false){
				printf("openmenu\n");
				Open_menu = true;
			}else{
				enter = true;
				printf("enter\n");
			}
	}
	//Interrupt blauwe button button 
	//if(GPIO_Pin == B1_Pin){
	//
	//}
		
	//Interrupt op externe pin PC11 meten van Kwh
	if(GPIO_Pin == PC11_Pin){
		
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		
		ticks = __HAL_TIM_GET_COUNTER(&htim16);
		
		// Sla het huidige 'uur' op.
		CurrentPulse = ticks;
		TijdTussenPulsen =   CurrentPulse - LastPulse ;
		LastPulse = CurrentPulse;
		
	}
}

//timer 16 interrupt
//void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
//{
	//if (htim == &htim16){
			//ticks++;
	//}
//}
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
