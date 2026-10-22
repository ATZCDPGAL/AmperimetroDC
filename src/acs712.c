#include "acs712.h"
#include "config.h"
#include <math.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ACS712";

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;
static bool s_has_cali = false;
static float s_zero_offset_mv = ACS712_NOMINAL_ZERO_MV;
static float s_filtered_current = 0.0f;
static bool s_is_calibrated = false;

/* Inicializar ADC1 y esquema de calibración */
esp_err_t acs712_init(void) {
    ESP_LOGI(TAG, "Configurando ADC1 para sensor ACS712 en GPIO %d...", ACS712_ADC_CHANNEL);

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ACS712_ADC_UNIT,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error inicializando unidad ADC1: %d", ret);
        return ret;
    }

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ret = adc_oneshot_config_channel(adc_handle, ACS712_ADC_CHANNEL, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando canal ADC: %d", ret);
        return ret;
    }

    /* Calibración eFuse */
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ACS712_ADC_UNIT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
    if (ret == ESP_OK) {
        s_has_cali = true;
        ESP_LOGI(TAG, "Calibración eFuse (Line Fitting) habilitada.");
    } else {
        ESP_LOGW(TAG, "Calibración eFuse no disponible, usando cálculo estándar.");
    }
#elif ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ACS712_ADC_UNIT,
        .chan = ACS712_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);
    if (ret == ESP_OK) {
        s_has_cali = true;
        ESP_LOGI(TAG, "Calibración eFuse (Curve Fitting) habilitada.");
    }
#endif

    return ESP_OK;
}

/* Leer voltaje promedio en milivoltios tomando N muestras */
static float read_averaged_voltage_mv(uint16_t samples) {
    if (samples == 0) samples = 1;
    uint32_t sum_voltage = 0;
    int raw_val = 0;
    int voltage_val = 0;

    for (uint16_t i = 0; i < samples; i++) {
        adc_oneshot_read(adc_handle, ACS712_ADC_CHANNEL, &raw_val);
        if (s_has_cali) {
            adc_cali_raw_to_voltage(cali_handle, raw_val, &voltage_val);
        } else {
            // Conversión aproximada para atenuación de 12dB (0-3100 mV)
            voltage_val = (raw_val * 3100) / 4095;
        }
        sum_voltage += voltage_val;
        vTaskDelay(pdMS_TO_TICKS(1)); // Pequeño retardo entre muestras para filtrar ruido
    }

    return (float)sum_voltage / (float)samples;
}

/* Calibrar el cero (Tara) cuando no hay corriente circulando */
esp_err_t acs712_calibrate_zero(uint16_t samples, float *out_zero_mv) {
    ESP_LOGI(TAG, "Iniciando calibración de cero (tara) con %d muestras...", samples);
    float zero_mv = read_averaged_voltage_mv(samples);
    s_zero_offset_mv = zero_mv;
    s_is_calibrated = true;
    s_filtered_current = 0.0f;

    if (out_zero_mv) {
        *out_zero_mv = zero_mv;
    }
    ESP_LOGI(TAG, "Calibración completada. Cero Vout = %.2f mV", zero_mv);
    return ESP_OK;
}

/* Realizar lectura actual del sensor */
esp_err_t acs712_read(acs712_data_t *data) {
    if (!data) return ESP_ERR_INVALID_ARG;

    float v_mv = read_averaged_voltage_mv(ACS712_SAMPLES_COUNT);
    data->voltage_mv = v_mv;
    data->zero_offset_mv = s_zero_offset_mv;
    data->is_calibrated = s_is_calibrated;

    /* Diferencia con respecto al punto de reposo (0A) */
    float delta_v = v_mv - s_zero_offset_mv;

    /* En corriente DC, si la corriente entra por IP+ a IP- delta_v es positiva.
     * Si los cables están invertidos en la bornera, delta_v será negativa.
     * Tomamos el valor absoluto para medir la magnitud DC: */
    float raw_a = fabsf(delta_v) / ACS712_SENSITIVITY_MV_PER_A;
    data->raw_current_a = raw_a;

    /* Zona muerta (Deadband) para ruido residual del sensor (35 mA) */
    if (raw_a < 0.04f) {
        raw_a = 0.0f;
    }

    /* Filtro pasa-bajas IIR exponencial para estabilidad numérica */
    s_filtered_current = (s_filtered_current * 0.70f) + (raw_a * 0.30f);

    /* Si está muy cerca de cero tras el filtro, redondear a 0.00 */
    if (s_filtered_current < 0.02f) {
        s_filtered_current = 0.0f;
    }

    /* Límite y verificación de rango 0 - 5A */
    data->overload = (s_filtered_current > CURRENT_MAX_A);
    if (s_filtered_current > CURRENT_MAX_A) {
        data->current_a = CURRENT_MAX_A;
    } else {
        data->current_a = s_filtered_current;
    }

    return ESP_OK;
}

void acs712_set_zero_offset(float zero_mv) {
    s_zero_offset_mv = zero_mv;
    s_is_calibrated = true;
}

float acs712_get_zero_offset(void) {
    return s_zero_offset_mv;
}
