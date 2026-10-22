# Amperímetro DC (0 - 5A) con ESP32, Sensor ACS712 (30A) y Display TFT 128x160 RGB

Este proyecto implementa un **amperímetro digital para corriente continua (DC)** de alta precisión con un rango de medición de **0 a 5 Amperios**, desarrollado en el framework nativo **ESP-IDF** (v5 / PlatformIO).

---

## 1. Componentes del Sistema

1. **Microcontrolador**: ESP32 DevKit V1 (38 pines o 30 pines).
2. **Sensor de Corriente**: Sensor de efecto Hall **ACS712 (modelo 30A)**:
   - Sensibilidad típica: **$66\text{ mV/A}$**.
   - Salida analógica proporcional a la corriente: $V_{out} = V_{zero} + (I \times 66\text{ mV})$.
   - Rango 0-5A: $V_{out} \approx 2.50\text{V}$ (a 0A) hasta $2.83\text{V}$ (a 5A).
3. **Pantalla**: Módulo Display **TFT 128*160 RGB** (1.8 pulgadas, controlador **ST7735**, resolución $128 \times 160$ píxeles, comunicación SPI).

---

## 2. Diagrama de Conexiones (Pinout)

### A. Display TFT 128*160 RGB (ST7735) $\leftrightarrow$ ESP32

| Pin del Display (TFT 128*RGB) | Nombre / Función | Pin ESP32 | Descripción |
| :--- | :--- | :--- | :--- |
| **GND** | Ground (0V) | **GND** | Masa común |
| **VCC** | Alimentación | **3.3V** o **5V (VIN)** | Alimentación del módulo display |
| **SCL** | SPI Clock (SCK) | **GPIO 18** | Reloj SPI (VSPI SCK) |
| **SDA** | SPI MOSI (Data In) | **GPIO 23** | Datos SPI (VSPI MOSI) |
| **RES** | Reset (RST) | **GPIO 22** | Reset por hardware del display |
| **DC** | Data / Command (A0) | **GPIO 21** | Selección comando (0) / dato (1) |
| **CS** | Chip Select (SS) | **GPIO 5** | Selección SPI activo en bajo |
| **BLK** | Backlight (LED) | **GPIO 15** o **3.3V** | Control de retroiluminación |

> **Nota sobre el Backlight (BLK):** Si tu módulo no enciende la pantalla o prefieres brillo constante al 100%, puedes conectar el pin `BLK` directamente al pin `3.3V` del ESP32.

---

### B. Sensor ACS712 (30A) $\leftrightarrow$ ESP32

| Pin del ACS712 | Conexión en ESP32 / Fuente | Notas de Funcionamiento |
| :--- | :--- | :--- |
| **VCC** | **5V (VIN del ESP32)** | El ACS712 requiere 5V para operar dentro de su curva lineal. |
| **GND** | **GND** | Masa común con el ESP32 y la fuente de alimentación. |
| **OUT** | **GPIO 34 (ADC1_CH6)** | Entrada analógica del ESP32 (pin de solo entrada, libre de ruido). |

### C. Conexión de la Carga (Bornera de 2 tornillos del ACS712)

La bornera verde del ACS712 se intercala **en serie** con la carga que deseas medir:
```text
[ Fuente DC (+) ] ---> [ Bornera ACS712 (Pin 1) ]
                        [ Bornera ACS712 (Pin 2) ] ---> [ (+) Carga DC ]
[ Fuente DC (-) ] -------------------------------------> [ (-) Carga DC ]
```

---

## 3. Características y Algoritmo de Medición

- **Calibración Automática de Tara (Zero Offset):** Al encender el ESP32, toma 200 muestras promediadas sin carga para medir el valor real de reposo ($V_{zero}$ real $\approx 2500\text{ mV}$), compensando variaciones del bus USB o fuente de 5V.
- **Calibración eFuse:** Emplea el controlador `esp_adc/adc_cali` con esquema de ajuste por línea para corregir la no linealidad del ADC interno del ESP32.
- **Sobremuestreo (Oversampling):** Cada ciclo realiza 64 lecturas para suprimir ruido eléctrico y componentes parásitas de 50/60 Hz.
- **Filtro Digital IIR Pasa-Bajas:** Suaviza la lectura en tiempo real sin retraso perceptible.
- **Filtro de Zona Muerta (Deadband):** Elimina el temblor o fluctuación en vacío (corrientes $< 0.04\text{A}$ se presentan como $0.00\text{A}$).
- **Gráficos Libres de Parpadeo (Zero-Flicker):**
  - Dígitos de corriente de gran formato (tamaño 3, alto contraste).
  - Código de colores dinámico: **Verde** (< 3.0A), **Amarillo** (3.0A - 4.2A), **Rojo** (> 4.2A o sobrecarga).
  - Barra analógica tipo vúmetro proporcional de 0 a 5 Amperios con escala graduada.
  - Indicador de alerta de sobrecarga (`! SOBRECARGA !`) si se superan los 5A.
  - Telemetría serial por puerto UART a 115200 baudios.

---

## 4. Instrucciones para Flashear y Monitorear

Desde la terminal en el directorio `AMPDC`:

1. **Compilar el proyecto:**
   ```powershell
   pio run
   ```

2. **Subir el firmware al ESP32 (asegúrate de conectar tu ESP32 por USB):**
   ```powershell
   pio run --target upload
   ```

3. **Abrir el monitor serie (115200 baudios):**
   ```powershell
   pio device monitor
   ```

---

## 5. Ajustes Opcionales (`include/config.h`)

Si tu pantalla tiene un pequeño desfase de borde (1 o 2 columnas) debido a diferencias entre fabricantes del módulo ST7735 128*RGB ver 1, puedes ajustar en `include/config.h`:
```c
#define ST7735_X_OFFSET     2   // Ajustable a 0 o 2
#define ST7735_Y_OFFSET     1   // Ajustable a 0, 1 o 32 según el lote del display
```
