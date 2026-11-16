# ThermoCheck – Proyecto 3 de Electrónica Digital 2

**Universidad del Valle de Guatemala**  
**Curso:** BE3029 – Electrónica Digital 2  
**Integrantes:** Stephany Mejía y Camila Aristizabal  
**Año:** 2026

---

## Descripción del proyecto

ThermoCheck fue desarrollado como un prototipo de monitoreo de temperatura para una incubadora neonatal.

El sistema permitió adquirir mediciones mediante un sensor LM75, visualizarlas en una pantalla TFT ILI9341 y almacenarlas en una tarjeta SD.

Se integraron dos microcontroladores STM32 NUCLEO-F446RE y un ESP32 para implementar comunicación inalámbrica, indicadores visuales y efectos sonoros.

El prototipo se enfocó en el monitoreo y registro de temperatura, sin implementar un control automático de calefacción.

## Componentes utilizados

- 2 tarjetas STM32 NUCLEO-F446RE.
- 1 ESP32 DevKit V1.
- Sensor de temperatura LM75.
- Pantalla TFT ILI9341 de 2.4 pulgadas.
- Tarjeta SD.
- NeoPixel WS2812B.
- Buzzer pasivo.
- Transistor PN2222A.
- Diodo de protección.
- Resistencias de 1 kΩ y 10 kΩ.
- Protoboard y cables de conexión.

## Funcionamiento del sistema

El ESP32 recibió comandos desde la aplicación Dabble mediante Bluetooth BLE y los transmitió a la Nucleo 1 utilizando UART.

La Nucleo 1 administró los comandos, los indicadores luminosos mediante PWM y DMA, y los efectos sonoros mediante DAC, TIM6 y DMA.

La Nucleo 2 realizó la adquisición de temperatura utilizando el sensor LM75 mediante I²C. Además, permitió visualizar las mediciones en la pantalla TFT y almacenarlas en una tarjeta SD mediante SPI.

## Comandos implementados

| Botón de Dabble | Función |
|---|---|
| Círculo | Reproducir la melodía principal |
| Triángulo | Realizar una lectura de temperatura |
| Cuadrado | Guardar la última medición en la tarjeta SD |

## Indicadores visuales

Se utilizó un NeoPixel WS2812B para representar los diferentes estados del sistema:

- **Azul:** estado de espera.
- **Amarillo:** adquisición de temperatura.
- **Morado:** transmisión de información.
- **Verde:** almacenamiento exitoso.

## Indicador sonoro

Se implementó un buzzer pasivo controlado mediante el DAC de la Nucleo 1.

Para generar los sonidos se utilizó el temporizador TIM6 y DMA, permitiendo reproducir diferentes frecuencias.

El circuito incorporó un transistor PN2222A, resistencias y un diodo de protección.

Se desarrollaron efectos sonoros para las funciones de inicio, lectura, guardado y procesamiento.

## Protocolos y periféricos utilizados

| Protocolo o periférico | Aplicación |
|---|---|
| Bluetooth BLE | Comunicación entre Dabble y ESP32 |
| UART | Comunicación entre ESP32 y Nucleo 1 |
| USART3 | Comunicación entre Nucleo 1 y Nucleo 2 |
| I²C | Lectura del sensor LM75 |
| SPI | Comunicación con pantalla TFT y tarjeta SD |
| DAC + TIM6 + DMA | Generación de sonidos |
| PWM + DMA | Control del NeoPixel |

## Herramientas utilizadas

- STM32CubeIDE
- STM32CubeMX
- Visual Studio Code
- PlatformIO
- Dabble
- GitHub

## Resultados

Se desarrolló un prototipo de monitoreo de temperatura para una incubadora neonatal, integrando adquisición de datos, visualización, almacenamiento y comunicación inalámbrica.

El proyecto permitió aplicar los conocimientos adquiridos durante el curso mediante la integración de microcontroladores, protocolos de comunicación y periféricos digitales.

---

## Autoras

**Stephany Mejía**  
**Camila Aristizabal**

Ingeniería Biomédica  
Universidad del Valle de Guatemala  
2026
