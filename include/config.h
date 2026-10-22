#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

/* =========================================================================
 * PINES DE CONEXIÓN DISPLAY TFT 128*128 RGB VER 1 (ST7735)
 * ========================================================================= */
#define PIN_TFT_SCL         GPIO_NUM_18   // SPI Clock (SCL / SCK)
#define PIN_TFT_SDA         GPIO_NUM_23   // SPI Data / MOSI (SDA)
#define PIN_TFT_CS          GPIO_NUM_5    // Chip Select (CS / SS)
#define PIN_TFT_DC          GPIO_NUM_21   // Data/Command (DC / A0 / RS)
#define PIN_TFT_RST         GPIO_NUM_22   // Reset (RES / RST)
#define PIN_TFT_BLK         GPIO_NUM_15   // Backlight control (BLK / LED) - Opcional

/* Dimensiones del Display (128x160) */
#define TFT_WIDTH           128
#define TFT_HEIGHT          160

/* Offsets para controlador ST7735 128x160 */
/* En módulos 1.8" 128x160 (Red Tab / Black Tab) los offsets estándar son X=0, Y=0 (o X=2, Y=1) */
#define ST7735_X_OFFSET     0
#define ST7735_Y_OFFSET     0

/* =========================================================================
 * CONFIGURACIÓN SENSOR ACS712 (30A)
 * ========================================================================= */
/* Pin analógico de entrada (GPIO34 = ADC1_CH6, entrada libre de ruidos y segura) */
#define ACS712_ADC_UNIT     ADC_UNIT_1
#define ACS712_ADC_CHANNEL  ADC_CHANNEL_6 // GPIO 34

/* Sensibilidad para modelo ACS712-30A: 66 mV por Amperio */
#define ACS712_SENSITIVITY_MV_PER_A   66.0f

/* Tensión teórica de reposo a 0A (Vcc / 2 = 2500 mV si Vcc=5V) */
#define ACS712_NOMINAL_ZERO_MV        2500.0f

/* Rango de medición para este amperímetro */
#define CURRENT_MIN_A                 0.0f
#define CURRENT_MAX_A                 5.0f

/* Número de muestras para promediar por cada lectura (oversampling) */
#define ACS712_SAMPLES_COUNT          64

/* Muestras para calibración de cero inicial (tara) */
#define ACS712_CALIB_SAMPLES          200

#endif /* CONFIG_H_ */
