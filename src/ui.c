#include "ui.h"
#include "st7735.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define CARD_X       4
#define CARD_Y       25
#define CARD_W       120
#define CARD_H       48

#define BAR_X        14
#define BAR_Y        122
#define BAR_MAX_W    100
#define BAR_H        12

static float s_last_current = -999.0f;
static float s_last_vout = -999.0f;
static int s_last_bar_w = 0;
static bool s_last_overload = false;

/* Pantalla inicial mientras se calibra la tara del sensor (128x160) */
void ui_show_calibrating(void) {
    st7735_fill_screen(ST7735_BLACK);
    
    // Encabezado
    st7735_fill_rect(0, 0, TFT_WIDTH, 20, 0x0841);
    st7735_draw_string(28, 6, "DC AMMETER", ST7735_CYAN, 0x0841, 1);
    st7735_draw_fast_h_line(0, 20, TFT_WIDTH, ST7735_CYAN);

    // Mensaje de calibración centrado verticalmente para 160px
    st7735_draw_string(14, 50, "CALIBRANDO TARA", ST7735_YELLOW, ST7735_BLACK, 1);
    st7735_draw_string(14, 70, "Sin carga conectada", ST7735_LIGHTGREY, ST7735_BLACK, 1);
    st7735_draw_string(18, 86, "(0.00 Amperios)", ST7735_LIGHTGREY, ST7735_BLACK, 1);
    st7735_draw_string(14, 106, "Por favor espere...", ST7735_DARKGREY, ST7735_BLACK, 1);

    // Barra de espera
    st7735_draw_rect(14, 130, 100, 10, ST7735_DARKCYAN);
    st7735_fill_rect(16, 132, 96, 6, ST7735_NAVY);
}

/* Dibujar la interfaz estática base para resolución 128x160 */
void ui_init(void) {
    st7735_fill_screen(ST7735_BLACK);

    // 1. Barra superior de título
    st7735_fill_rect(0, 0, TFT_WIDTH, 20, 0x0841);
    st7735_draw_string(28, 6, "DC AMMETER", ST7735_CYAN, 0x0841, 1);
    st7735_draw_fast_h_line(0, 20, TFT_WIDTH, ST7735_DARKCYAN);

    // 2. Tarjeta del valor principal de corriente (48px de alto)
    st7735_fill_rect(CARD_X, CARD_Y, CARD_W, CARD_H, ST7735_CARD_BG);
    st7735_draw_rect(CARD_X, CARD_Y, CARD_W, CARD_H, ST7735_DARKGREY);

    // Símbolo de unidad 'A' en tamaño 2
    st7735_draw_string(96, 42, "A", ST7735_YELLOW, ST7735_CARD_BG, 2);

    // 3. Área estática de la barra analógica (0 a 5A)
    st7735_draw_rect(BAR_X - 2, BAR_Y - 2, BAR_MAX_W + 4, BAR_H + 4, ST7735_LIGHTGREY);

    // Marcas de escala (Ticks)
    for (int i = 0; i <= 5; i++) {
        int tick_x = BAR_X + (i * BAR_MAX_W) / 5;
        st7735_draw_fast_v_line(tick_x, BAR_Y + BAR_H + 3, 3, ST7735_LIGHTGREY);
    }

    // Leyendas de escala
    st7735_draw_string(BAR_X - 2, 142, "0", ST7735_DARKGREY, ST7735_BLACK, 1);
    st7735_draw_string(BAR_X + 18, 142, "1", ST7735_DARKGREY, ST7735_BLACK, 1);
    st7735_draw_string(BAR_X + 38, 142, "2", ST7735_DARKGREY, ST7735_BLACK, 1);
    st7735_draw_string(BAR_X + 58, 142, "3", ST7735_DARKGREY, ST7735_BLACK, 1);
    st7735_draw_string(BAR_X + 78, 142, "4", ST7735_DARKGREY, ST7735_BLACK, 1);
    st7735_draw_string(BAR_X + 96, 142, "5A", ST7735_YELLOW, ST7735_BLACK, 1);

    // Resetear valores de tracking
    s_last_current = -999.0f;
    s_last_vout = -999.0f;
    s_last_bar_w = 0;
    s_last_overload = false;
}

/* Actualización dinámica y libre de parpadeo */
void ui_update(const acs712_data_t *data) {
    if (!data) return;

    // 1. Actualizar el valor numérico grande de Corriente si cambia
    if (fabsf(data->current_a - s_last_current) >= 0.01f || data->overload != s_last_overload) {
        s_last_current = data->current_a;
        s_last_overload = data->overload;

        char buf_cur[16];
        snprintf(buf_cur, sizeof(buf_cur), "%4.2f", data->current_a);

        // Color dinámico según la magnitud de corriente
        uint16_t num_color = ST7735_GREEN;
        if (data->current_a >= 4.2f || data->overload) {
            num_color = ST7735_RED;
        } else if (data->current_a >= 3.0f) {
            num_color = ST7735_YELLOW;
        }

        // Dígitos en tamaño 3 (18x24 px por carácter) con fondo sólido sin flicker
        st7735_draw_string(14, 37, buf_cur, num_color, ST7735_CARD_BG, 3);
    }

    // 2. Información de telemetría y diagnóstico (Voltajes en mV)
    if (fabsf(data->voltage_mv - s_last_vout) >= 2.0f) {
        s_last_vout = data->voltage_mv;

        char buf_v[24];
        if (data->overload) {
            snprintf(buf_v, sizeof(buf_v), "! SOBRECARGA >5A !");
            st7735_draw_string(10, 80, buf_v, ST7735_RED, ST7735_BLACK, 1);
        } else {
            snprintf(buf_v, sizeof(buf_v), "Vout:  %4.0f mV   ", data->voltage_mv);
            st7735_draw_string(14, 80, buf_v, ST7735_LIGHTGREY, ST7735_BLACK, 1);
        }

        char buf_z[24];
        snprintf(buf_z, sizeof(buf_z), "Vzero: %4.0f mV   ", data->zero_offset_mv);
        st7735_draw_string(14, 93, buf_z, ST7735_DARKGREY, ST7735_BLACK, 1);

        char buf_d[24];
        float delta_v = data->voltage_mv - data->zero_offset_mv;
        snprintf(buf_d, sizeof(buf_d), "Delta: %+4.0f mV   ", delta_v);
        st7735_draw_string(14, 106, buf_d, ST7735_CYAN, ST7735_BLACK, 1);
    }

    // 3. Barra analógica de progreso (0.0 a 5.0 A)
    int target_w = (int)((data->current_a / CURRENT_MAX_A) * (float)BAR_MAX_W);
    if (target_w > BAR_MAX_W) target_w = BAR_MAX_W;
    if (target_w < 0) target_w = 0;

    if (target_w != s_last_bar_w) {
        // Color dinámico de la barra
        uint16_t bar_color = ST7735_GREEN;
        if (data->current_a >= 4.2f || data->overload) {
            bar_color = ST7735_RED;
        } else if (data->current_a >= 3.0f) {
            bar_color = ST7735_YELLOW;
        }

        if (target_w > s_last_bar_w) {
            // Rellenar nuevo segmento
            st7735_fill_rect(BAR_X + s_last_bar_w, BAR_Y, target_w - s_last_bar_w, BAR_H, bar_color);
            // Repintar porción anterior si cambió el color
            if (bar_color != ST7735_GREEN) {
                st7735_fill_rect(BAR_X, BAR_Y, s_last_bar_w, BAR_H, bar_color);
            }
        } else {
            // Borrar segmento que retrocedió
            st7735_fill_rect(BAR_X + target_w, BAR_Y, s_last_bar_w - target_w, BAR_H, ST7735_BLACK);
            // Asegurar color correcto del segmento restante
            st7735_fill_rect(BAR_X, BAR_Y, target_w, BAR_H, bar_color);
        }
        s_last_bar_w = target_w;
    }
}
