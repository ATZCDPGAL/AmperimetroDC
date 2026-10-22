#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "config.h"
#include "st7735.h"
#include "acs712.h"
#include "ui.h"

static const char *TAG = "MAIN_APP";

void app_main(void) {
    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  AMPERIMETRO DC 0-5A (ESP32 + ACS712-30A + TFT) ");
    ESP_LOGI(TAG, "=================================================");

    /* 1. Inicializar Display TFT ST7735 */
    esp_err_t ret = st7735_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error crítico al inicializar el display TFT.");
        return;
    }

    /* 2. Inicializar Sensor ACS712 */
    ret = acs712_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error crítico al inicializar el sensor ACS712.");
        return;
    }

    /* 3. Mostrar pantalla de calibración de tara */
    ui_show_calibrating();

    /* 4. Ejecutar calibración de cero (Tara) */
    float zero_mv = 0.0f;
    acs712_calibrate_zero(ACS712_CALIB_SAMPLES, &zero_mv);
    ESP_LOGI(TAG, "Voltaje de reposo (0A) establecido en: %.2f mV", zero_mv);

    // Breve pausa para que se aprecie la calibración
    vTaskDelay(pdMS_TO_TICKS(800));

    /* 5. Dibujar la interfaz principal del instrumento */
    ui_init();

    /* 6. Bucle principal de adquisición y visualización */
    acs712_data_t data;
    uint32_t log_counter = 0;

    while (1) {
        // Leer sensor con sobremuestreo y filtrado
        ret = acs712_read(&data);
        if (ret == ESP_OK) {
            // Actualizar interfaz gráfica en el display TFT
            ui_update(&data);

            // Reportar telemetría por puerto serie cada 500 ms (5 ciclos x 100 ms)
            if (++log_counter >= 5) {
                log_counter = 0;
                ESP_LOGI(TAG, "[MEDICIÓN] Corriente: %4.2f A | Vout: %4.0f mV | Vzero: %4.0f mV %s",
                         data.current_a,
                         data.voltage_mv,
                         data.zero_offset_mv,
                         data.overload ? " [ALERTA: SOBRECARGA > 5A]" : "");
            }
        }

        // Tasa de refresco a ~10 Hz (100 ms)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}