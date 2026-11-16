/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
DMA_HandleTypeDef hdma_dac1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim6;
DMA_HandleTypeDef hdma_tim1_ch1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

// Comando recibido del ESP32
uint8_t comandoUART;

// Almacena la respuesta de Nucleo 2
char temperaturaRecibida[20];

// Arreglo de pulsos para 1 NeoPixel:
// 24 bits de color + 64 periodos en bajo
uint16_t datosNeoPixel[88];

// Indica que termino la transferencia DMA
volatile uint8_t neoPixelTerminado = 0;



/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_DAC_Init(void);
static void MX_TIM6_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Tabla de 32 muestras para generar una onda cuadrada
uint16_t ondaDAC[32];

// Prepara las muestras que enviara el DMA al DAC
void DAC_PrepararOnda(void)
{
    for (uint8_t i = 0; i < 32; i++)
    {
        if (i < 16)
            ondaDAC[i] = 4095;  // Nivel alto
        else
            ondaDAC[i] = 0;     // Nivel bajo
    }
}

// Reproduce una nota usando TIM6, DMA y DAC
void DAC_ReproducirTono(uint32_t frecuencia, uint32_t duracion)
{
    if (frecuencia == 0)
    {
        HAL_Delay(duracion);
        return;
    }

    // TIM6 funciona a 84 MHz en esta configuracion
    // Cada periodo de la onda contiene 32 muestras
    uint32_t tasaMuestras = frecuencia * 32U;
    uint32_t cuentas = (84000000U + tasaMuestras / 2U)
                       / tasaMuestras;

    // Configurar la frecuencia de actualizacion de TIM6
    __HAL_TIM_SET_AUTORELOAD(&htim6, cuentas - 1U);
    __HAL_TIM_SET_COUNTER(&htim6, 0);

    // Iniciar la transferencia circular mediante DMA
    if (HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1,
                          (uint32_t *)ondaDAC, 32,
                          DAC_ALIGN_12B_R) != HAL_OK)
    {
        Error_Handler();
    }

    // Activar TIM6 para actualizar el DAC
    if (HAL_TIM_Base_Start(&htim6) != HAL_OK)
    {
        Error_Handler();
    }

    // Mantener la nota durante el tiempo solicitado
    HAL_Delay(duracion);

    // Detener la generacion del sonido
    HAL_TIM_Base_Stop(&htim6);
    HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
    HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);
}
// Sonido 1: lectura realizada
void Sonido_Lectura(void)
{
    // Primer pitido corto
    DAC_ReproducirTono(1000, 80);
    HAL_Delay(60);

    // Segundo pitido corto
    DAC_ReproducirTono(1000, 80);
    HAL_Delay(60);

    // Tercer pitido un poco mas largo
    DAC_ReproducirTono(1000, 160);
}
 // Sonido 2: guardado correcto
 void Sonido_Guardado(void)
 {
     // Primera nota
     DAC_ReproducirTono(784, 120);
     HAL_Delay(70);

     // Segunda nota, mas aguda
     DAC_ReproducirTono(1047, 120);
     HAL_Delay(70);

     // Tercera nota de confirmacion
     DAC_ReproducirTono(1319, 300);
 }
// Melodia original de inicio de VitaCheck
void Sonido_Inicio(void)
{
    DAC_ReproducirTono(523, 500);
    HAL_Delay(165);

    DAC_ReproducirTono(659, 500);
    HAL_Delay(165);

    DAC_ReproducirTono(784, 700);
    HAL_Delay(165);

    DAC_ReproducirTono(1047, 800);
    HAL_Delay(165);

    DAC_ReproducirTono(784, 500);
    HAL_Delay(165);

    DAC_ReproducirTono(659, 500);
    HAL_Delay(165);

    DAC_ReproducirTono(523, 500);

}
    // Enviar un color al NeoPixel mediante TIM1 y DMA
    void NeoPixel_Color(uint8_t rojo,
                       uint8_t verde,
                       uint8_t azul)
    {
        // WS2812B utiliza el orden GRB
        uint32_t color = ((uint32_t)verde << 16) |
                         ((uint32_t)rojo << 8) |
                         azul;

        // Preparar los 24 bits del color
        for (uint8_t i = 0; i < 24; i++)
        {
            // Comprobar si el bit es 1 o 0
            if (color & (1UL << (23 - i)))
            {
                datosNeoPixel[i] = 67;  // Bit 1
            }
            else
            {
                datosNeoPixel[i] = 34;  // Bit 0
            }
        }

        // Agregar 64 periodos en bajo para el reset
        for (uint8_t i = 24; i < 88; i++)
        {
            datosNeoPixel[i] = 0;
        }

        // Preparar la señal de finalizacion
        neoPixelTerminado = 0;

        // Iniciar PWM con DMA por TIM1_CH1
        if (HAL_TIM_PWM_Start_DMA(&htim1,
                                 TIM_CHANNEL_1,
                                 (uint32_t *)datosNeoPixel,
                                 88) != HAL_OK)
        {
            Error_Handler();
        }

        // Esperar a que termine la transferencia
        uint32_t inicio = HAL_GetTick();

        while (!neoPixelTerminado)
        {
            // Evitar espera infinita por un error DMA
            if ((HAL_GetTick() - inicio) > 50)
            {
                HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
                Error_Handler();
            }
        }

        // Detener PWM al terminar la trama
        HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);

        // Mantener la salida PWM en nivel bajo
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);

        // Dar tiempo para actualizar el color
        HAL_Delay(1);
    }



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
  MX_DMA_Init();
  MX_DAC_Init();
  MX_TIM6_Init();
  MX_USART3_UART_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();



  /* USER CODE BEGIN 2 */

  // Preparar las muestras de la onda
  DAC_PrepararOnda();


  // Sistema encendido y en espera
  NeoPixel_Color(0, 0, 150);  // Azul

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

      // Recibir comando del ESP32 mediante USART1
      if (HAL_UART_Receive(&huart1, &comandoUART, 1, 10) == HAL_OK)
      {
    	  if (comandoUART == 'T')
    	  {
    	      uint8_t respuesta;
    	      uint8_t solicitud = 'R';
    	      uint8_t dato;
    	      uint8_t indice = 0;

    	      // Reproducir sonido al iniciar la lectura
    	      Sonido_Lectura();

    	      // AMARILLO: adquisicion de temperatura
    	      NeoPixel_Color(80, 80, 0);

    	      // Solicitar medicion a Nucleo 2
    	      HAL_StatusTypeDef envio =
    	          HAL_UART_Transmit(&huart3, &comandoUART, 1, 100);

    	      if (envio == HAL_OK &&
    	          HAL_UART_Receive(&huart3, &respuesta, 1, 1500) == HAL_OK &&
    	          respuesta == 'A')
    	      {
    	          // MORADO: iniciar transmision de datos
    	          NeoPixel_Color(60, 0, 80);

    	          // Solicitar el resultado de la medicion
    	          if (HAL_UART_Transmit(&huart3, &solicitud, 1, 100) == HAL_OK)
    	          {
    	              uint32_t inicio = HAL_GetTick();

    	              // Recibir la temperatura hasta salto de linea
    	              while ((HAL_GetTick() - inicio) < 1500)
    	              {
    	                  if (HAL_UART_Receive(&huart3, &dato, 1, 20) == HAL_OK)
    	                  {
    	                      if (dato == '\n')
    	                      {
    	                          break;
    	                      }

    	                      if (dato != '\r' &&
    	                          indice < sizeof(temperaturaRecibida) - 1)
    	                      {
    	                          temperaturaRecibida[indice++] = dato;
    	                      }
    	                  }
    	              }
    	          }

    	          temperaturaRecibida[indice] = '\0';

    	          if (indice > 0)
    	          {
    	              // Enviar resultado al ESP32
    	              HAL_UART_Transmit(&huart1,
    	                                (uint8_t *)temperaturaRecibida,
    	                                indice, 100);

    	              uint8_t salto = '\n';
    	              HAL_UART_Transmit(&huart1, &salto, 1, 100);
    	              HAL_Delay(500);  // Mantener el morado visible 500 ms
    	          }
    	          else
    	          {
    	              uint8_t error[] = "SIN RESPUESTA NUCLEO 2\n";
    	              HAL_UART_Transmit(&huart1, error,
    	                                sizeof(error) - 1, 100);
    	          }
    	      }
    	      else
    	      {
    	          // No se pudo completar la adquisicion
    	          uint8_t error[] = "ERROR ADQUISICION\n";

    	          HAL_UART_Transmit(&huart1, error,
    	                            sizeof(error) - 1, 100);
    	      }

    	      // AZUL: regresar al estado de espera
    	      NeoPixel_Color(0, 0, 150);
    	  }


    	  // G: solicitar guardado de temperatura en la SD
    	        else if (comandoUART == 'G')
    	        {
    	            uint8_t confirmacion = 0;

    	            // Enviar comando G a Nucleo 2 por USART3
    	            if (HAL_UART_Transmit(&huart3, &comandoUART,
    	                                  1, 100) == HAL_OK)
    	            {
    	                // Esperar respuesta de Nucleo 2
    	                if (HAL_UART_Receive(&huart3, &confirmacion,
    	                                     1, 1500) == HAL_OK &&
    	                    confirmacion == 'K')
    	                {
    	                    // VERDE: guardado exitoso en SD
    	                    NeoPixel_Color(0, 80, 0);

    	                    // Reproducir sonido de confirmacion
    	                    Sonido_Guardado();

    	                    // Informar al ESP32 que se guardo
    	                    HAL_UART_Transmit(&huart1,
    	                                      &confirmacion, 1, 100);
    	                }
    	                else
    	                {
    	                    // Error al guardar en SD
    	                    uint8_t error[] = "ERROR GUARDADO\n";

    	                    HAL_UART_Transmit(&huart1, error,
    	                                      sizeof(error) - 1, 100);
    	                }
    	            }
    	            else
    	            {
    	                // Error al enviar el comando a Nucleo 2
    	                uint8_t error[] = "ERROR UART NUCLEO 2\n";

    	                HAL_UART_Transmit(&huart1, error,
    	                                  sizeof(error) - 1, 100);
    	            }

    	            // AZUL: regresar al estado de espera
    	            NeoPixel_Color(0, 0, 150);
    	        }

          // I: melodia de inicio
          else if (comandoUART == 'I')
          {
              Sonido_Inicio();

              // Confirmar comando al ESP32
              HAL_UART_Transmit(&huart1, &comandoUART, 1, 100);
          }
      }

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
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
  sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */

  /* USER CODE END DAC_Init 2 */

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

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 104 ;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 0;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 2624;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USART_TX_Pin USART_RX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin|USART_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

// Se ejecuta cuando termina la transferencia PWM por DMA
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    // Verificar que termino TIM1 Channel 1
    if (htim->Instance == TIM1 &&
        htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        neoPixelTerminado = 1;
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
#ifdef USE_FULL_ASSERT
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
