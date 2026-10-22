#ifndef UI_H_
#define UI_H_

#include "acs712.h"
#include <stdint.h>
#include <stdbool.h>

/* Inicializar pantalla con la interfaz gráfica base */
void ui_init(void);

/* Actualizar valores en pantalla (solo repinta zonas dinámicas sin parpadeo) */
void ui_update(const acs712_data_t *data);

/* Pantalla de bienvenida durante la calibración de tara */
void ui_show_calibrating(void);

#endif /* UI_H_ */
