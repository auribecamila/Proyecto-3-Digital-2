/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 * PROYECTO 3
 * CAMILA ARISTIZABAL / STEPHANY MEJÍA
 * Copyright (c) 2024 STMicroelectronics.
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
#include "fatfs.h"
#include "fatfs_sd.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ili9341.h"
#include "bitmaps.h"
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
SPI_HandleTypeDef hspi1;
FATFS fs;
FATFS *pfs;
FIL fil;
FRESULT fres;
DWORD fre_clust;
uint32_t totalSpace, freeSpace;
char buffer[100];
//Sensor de temperatura LM75
#define LM75_ADDR       (0x48 << 1)
#define LM75_REG_TEMP   0x00

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;


UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
extern const uint16_t fondo[];

uint8_t solicitud;
int16_t temperatura10;
char respuesta[20];
uint8_t medicionValida = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

void DibujarInterfazTermometro(void);
void DibujarTermometro(int x, int y);
void ActualizarTemperaturaDemo(float temperatura);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* Lectura de temperatura del LM75 mediante I2C */
HAL_StatusTypeDef LeerTemperaturaLM75(int16_t *temperatura10)
{
    uint8_t datos[2];
    int16_t temperatura9;

    if (HAL_I2C_Mem_Read(&hi2c1,
                         LM75_ADDR,
                         LM75_REG_TEMP,
                         I2C_MEMADD_SIZE_8BIT,
                         datos,
                         2,
                         100) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // Se obtienen los 9 bits de temperatura
    temperatura9 = ((uint16_t)datos[0] << 1) |
                    (datos[1] >> 7);

    // Conversión de complemento a dos
    if (temperatura9 & 0x0100)
    {
        temperatura9 -= 512;
    }

    // Cada unidad equivale a 0.5 °C.
    // Guardamos la temperatura en décimas de grado.
    *temperatura10 = temperatura9 * 5;

    return HAL_OK;
}

/* Recibe la solicitud de lectura desde la Nucleo 1 */
char RecibirSolicitud(void)
{
	 solicitud = 0;

	    if (HAL_UART_Receive(&huart3, &solicitud, 1, 100) == HAL_OK)
	    {
	        return (char)solicitud;
	    }

	    return 0;
}

FRESULT GuardarTemperaturaSD(int16_t temperatura)
{
    FRESULT resultado;
    UINT bytesEscritos;
    char registro[40];

    int valorAbsoluto = temperatura;

    if (valorAbsoluto < 0)
    {
        valorAbsoluto = -valorAbsoluto;
    }

    // Preparar el registro con su unidad de medida
    snprintf(registro, sizeof(registro),
             "%s%d.%d C\r\n",
             (temperatura < 0) ? "-" : "",
             valorAbsoluto / 10,
             valorAbsoluto % 10);

    // Abrir el archivo para escritura
    resultado = f_open(&USERFile, "temperaturas.txt",
                       FA_OPEN_ALWAYS | FA_WRITE);

    if (resultado != FR_OK)
    {
        return resultado;
    }

    // Colocarse al final para conservar los registros anteriores
    resultado = f_lseek(&USERFile, f_size(&USERFile));

    if (resultado == FR_OK)
    {
        // Escribir el nuevo registro
        resultado = f_write(&USERFile,
                            registro,
                            strlen(registro),
                            &bytesEscritos);

        if (resultado == FR_OK && bytesEscritos != strlen(registro))
        {
            resultado = FR_DISK_ERR;
        }
    }

    // Asegurar que los datos se escriban en la tarjeta
    if (resultado == FR_OK)
    {
        resultado = f_sync(&USERFile);
    }

    // Cerrar el archivo
    {
        FRESULT resultadoCierre = f_close(&USERFile);

        if (resultado == FR_OK)
        {
            resultado = resultadoCierre;
        }
    }

    return resultado;
}
/* Colores RGB565 */
#define COLOR_FONDO       0x1082
#define COLOR_PANEL       0x2104
#define COLOR_AZUL        0x02B5
#define COLOR_TURQUESA    0x07FF
#define COLOR_CORAL       0xF986
#define COLOR_BLANCO      0xFFFF
#define COLOR_GRIS        0xC618
#define COLOR_VERDE       0x07E0

/* Dibuja el termómetro vertical */
void DibujarTermometro(int x, int y)
{
    /* Fondo del termómetro */
    FillRect(x, y, 34, 108, COLOR_PANEL);

    /* Tubo exterior */
    FillRect(x + 11, y, 12, 76, COLOR_BLANCO);

    /* Interior del tubo */
    FillRect(x + 14, y + 4, 6, 68, COLOR_AZUL);

    /* Columna de líquido de demostración */
    FillRect(x + 14, y + 32, 6, 40, COLOR_CORAL);

    /* Bulbo */
    FillRect(x + 5, y + 69, 24, 24, COLOR_BLANCO);
    FillRect(x + 9, y + 73, 16, 16, COLOR_CORAL);

    /* Marcas de escala */
    H_line(x + 25, y + 12, 7, COLOR_BLANCO);
    H_line(x + 25, y + 27, 7, COLOR_BLANCO);
    H_line(x + 25, y + 42, 7, COLOR_BLANCO);
    H_line(x + 25, y + 57, 7, COLOR_BLANCO);
}

/* Dibuja toda la interfaz */
void DibujarInterfazTermometro(void)
{
    /* Fondo */
    LCD_Clear(COLOR_FONDO);
    FillRect(0, 0, 320, 240, COLOR_FONDO);

    /* Encabezado */
    FillRect(0, 0, 320, 36, COLOR_AZUL);

    LCD_Print("THERMOCHECK", 10, 5, 2,
              COLOR_BLANCO, COLOR_AZUL);

    LCD_Print("MEDICAL", 230, 13, 1,
              COLOR_BLANCO, COLOR_AZUL);

    /* Tarjeta principal */
    FillRect(10, 45, 300, 143, COLOR_PANEL);
    Rect(10, 45, 300, 143, COLOR_TURQUESA);

    /* Título de la medición */
    LCD_Print("TEMPERATURA", 88, 53, 1,
              COLOR_TURQUESA, COLOR_PANEL);

    LCD_Print("CORPORAL", 88, 67, 1,
              COLOR_BLANCO, COLOR_PANEL);

    /* Termómetro gráfico */
    DibujarTermometro(28, 72);

    /* Valor de demostración */
    LCD_Print("36.8", 115, 91, 3,
              COLOR_BLANCO, COLOR_PANEL);

    LCD_Print("grados C", 120, 126, 1,
              COLOR_TURQUESA, COLOR_PANEL);

    /* Indicador visual de demostración */
    FillRect(88, 151, 205, 25, COLOR_AZUL);

    LCD_Print("VALOR DE DEMO", 104, 159, 1,
              COLOR_BLANCO, COLOR_AZUL);

    /* Pie de pantalla */
    H_line(10, 198, 300, COLOR_TURQUESA);

    LCD_Print("SENSOR: NO CONECTADO", 10, 207, 1,
              COLOR_GRIS, COLOR_FONDO);

    LCD_Print("SISTEMA EN ESPERA", 10, 223, 1,
              COLOR_VERDE, COLOR_FONDO);
}

/* Actualiza el valor mostrado.
   Por ahora solo se usará para pruebas. */
void ActualizarTemperaturaDemo(float temperatura)
{
    char texto[12];

    int temperatura10 = (int)(temperatura * 10.0f + 0.5f);
    int parte_entera = temperatura10 / 10;
    int decimal = temperatura10 % 10;

    snprintf(texto, sizeof(texto), "%d.%d",
             parte_entera, decimal);

    FillRect(90, 85, 140, 25, COLOR_PANEL);
    LCD_Print(texto, 95, 89, 2, COLOR_BLANCO, COLOR_PANEL);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	FRESULT resultadoSD;
	uint8_t sdMontada = 0;

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
  MX_SPI1_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  LCD_Init();

  /* Asegurar que ambos dispositivos SPI estén deseleccionados */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // LCD CS
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // SD CS

  // Montar sistema de archivos
  resultadoSD = f_mount(&USERFatFS, USERPath, 1);

  if (resultadoSD == FR_OK)
  {
      sdMontada = 1;

      char mensaje[] = "SD MONTADA CORRECTAMENTE\r\n";

      HAL_UART_Transmit(&huart2,
                        (uint8_t *)mensaje,
                        strlen(mensaje),
                        HAL_MAX_DELAY);
  }
  else
  {
      char mensaje[40];

      snprintf(mensaje, sizeof(mensaje),
               "ERROR AL MONTAR SD: %d\r\n", resultadoSD);

      HAL_UART_Transmit(&huart2,
                        (uint8_t *)mensaje,
                        strlen(mensaje),
                        HAL_MAX_DELAY);
  }


  DibujarInterfazTermometro();


  HAL_UART_Transmit(&huart2,
                    (uint8_t *)"SISTEMA INICIADO\r\n",
                    strlen("SISTEMA INICIADO\r\n"),
                    HAL_MAX_DELAY);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

	      /* USER CODE BEGIN 3 */

	      // Recibir comando enviado por Nucleo 1
	      char comando = RecibirSolicitud();

	      // T: ADQUISICION DE TEMPERATURA
	      if (comando == 'T')
	      {
	          // Invalidar la medicion anterior
	          medicionValida = 0;

	          // Leer temperatura del LM75 mediante I2C
	          if (LeerTemperaturaLM75(&temperatura10) == HAL_OK)
	          {
	              int valorAbsoluto = temperatura10;

	              if (valorAbsoluto < 0)
	              {
	                  valorAbsoluto = -valorAbsoluto;
	              }

	              // Actualizar la pantalla TFT
	              ActualizarTemperaturaDemo(temperatura10 / 10.0f);

	              // Preparar la temperatura para transmitirla
	              snprintf(respuesta, sizeof(respuesta),
	                       "%s%d.%d\r\n",
	                       (temperatura10 < 0) ? "-" : "",
	                       valorAbsoluto / 10,
	                       valorAbsoluto % 10);

	              // Guardar la ultima medicion como valida
	              medicionValida = 1;

	              // A: adquisicion finalizada
	              uint8_t confirmacion = 'A';

	              HAL_UART_Transmit(&huart3,
	                                &confirmacion, 1, 100);
	          }
	          else
	          {
	              // E: error al leer el sensor
	              uint8_t error = 'E';

	              HAL_UART_Transmit(&huart3,
	                                &error, 1, 100);
	          }
	      }

	      // R: TRANSMISION DE TEMPERATURA
	      else if (comando == 'R')
	      {
	          if (medicionValida)
	          {
	              // Enviar temperatura a Nucleo 1
	              HAL_UART_Transmit(&huart3,
	                                (uint8_t *)respuesta,
	                                strlen(respuesta), 100);

	              // Conservar la medicion para guardarla
	          }
	          else
	          {
	              // No existe una medicion valida
	              uint8_t error[] = "ERROR SIN LECTURA\r\n";

	              HAL_UART_Transmit(&huart3,
	                                error,
	                                sizeof(error) - 1, 100);
	          }
	      }

	      // G: GUARDAR TEMPERATURA EN SD
	      else if (comando == 'G')
	      {
	          // E por defecto: error
	          uint8_t confirmacion = 'E';

	          // Verificar que exista una medicion valida
	          if (medicionValida)
	          {
	              // Intentar montar la SD si no estaba montada
	              if (!sdMontada)
	              {
	                  resultadoSD = f_mount(&USERFatFS,
	                                       USERPath, 1);

	                  if (resultadoSD == FR_OK)
	                  {
	                      sdMontada = 1;
	                  }
	              }

	              // Guardar la ultima temperatura
	              if (sdMontada)
	              {
	                  resultadoSD = GuardarTemperaturaSD(temperatura10);

	                  if (resultadoSD == FR_OK)
	                  {
	                      // K: guardado exitoso
	                      confirmacion = 'K';
	                  }
	                  else
	                  {
	                      // Indicar que hubo error de escritura
	                      sdMontada = 0;
	                  }
	              }
	          }

	          // Confirmar el resultado a Nucleo 1
	          HAL_UART_Transmit(&huart3,
	                            &confirmacion, 1, 100);
	      }
	  }
}
/* USER CODE END 3*/




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
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_RESET_Pin */
  GPIO_InitStruct.Pin = LCD_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(LCD_RESET_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_DC_Pin */
  GPIO_InitStruct.Pin = LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(LCD_DC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin SD_SS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|SD_SS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
	while (1) {
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
