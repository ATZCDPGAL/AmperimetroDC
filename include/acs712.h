#ifndef ACS712_H_
#define ACS712_H_

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/* Estructura con datos de medición del sensor ACS712 */
typedef struct {
    float current_a;       // Corriente filtrada en Amperios (0.00 a 5.00 A)
    float raw_current_a;   // Corriente instantánea sin filtrar
    float voltage_mv;      // Voltaje medido en la salida OUT del sensor (mV)
    float zero_offset_mv;  // Voltaje de referencia de cero calibrado (mV)
    bool overload;         // Alerta: supera los 5.0 Amperios
    bool is_calibrated;    // Indica si se completó la calibración de tara
} acs712_data_t;

/* Inicializar el ADC del ESP32 y el sistema de calibración eFuse */
esp_err_t acs712_init(void);

/* Calibrar el cero (Tara) midiendo el voltaje sin carga */
esp_err_t acs712_calibrate_zero(uint16_t samples, float *out_zero_mv);

/* Realizar lectura con sobremuestreo y filtrado */
esp_err_t acs712_read(acs712_data_t *data);

/* Modificar manualmente el voltaje de referencia de cero */
void acs712_set_zero_offset(float zero_mv);

/* Obtener el offset de cero actual */
float acs712_get_zero_offset(void);

#endif /* ACS712_H_ */
