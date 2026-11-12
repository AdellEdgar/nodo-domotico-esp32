/**
 * @file bme280.c
 * @brief Sensor ambiental BME280 (I2C): temperatura, humedad y presión.
 *
 * El BME280 se maneja con registros: se escribe la dirección del registro
 * y se leen los bytes que siguen. El registro 0xD0 (chip_id) identifica
 * el chip: 0x60 en el BME280 y 0x58 en el BMP280, que no mide humedad.
 */
#include "bme280.h"
#include "config.h"
#include "i2c_bus.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bme280";

#define REG_CHIP_ID   0xD0
#define ID_BME280     0x60
#define ID_BMP280     0x58
#define REG_CALIB_TP  0x88   // 0x88–0xA1: T1–T3, P1–P9 y H1 (26 bytes)
#define REG_CALIB_H   0xE1   // 0xE1–0xE7: H2–H6 (7 bytes)
#define TIMEOUT_MS    100
#define REG_RESET     0xE0
#define CMD_RESET     0xB6
#define REG_CTRL_HUM  0xF2
#define REG_CTRL_MEAS 0xF4
#define REG_DATOS     0xF7   // 0xF7–0xFE: presión (3), temperatura (3), humedad (2)
#define REG_CONFIG    0xF5

// Sobremuestreo x1 en todo, sin filtro, una medida por segundo
#define VAL_CTRL_HUM  0x01   // osrs_h = x1
#define VAL_CONFIG    0xA0   // t_sb = 1000 ms, filtro desactivado
#define VAL_CTRL_MEAS 0x27   // osrs_t = x1, osrs_p = x1, modo normal

static i2c_master_dev_handle_t dev;

/** Lee 'len' bytes a partir del registro 'reg'. */
static esp_err_t leer_registros(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, buf, len, TIMEOUT_MS);
}

/** Escribe un byte en el registro 'reg'. */
static esp_err_t escribir_registro(uint8_t reg, uint8_t valor)
{
    uint8_t buf[2] = { reg, valor };
    return i2c_master_transmit(dev, buf, sizeof(buf), TIMEOUT_MS);
}

/** Coeficientes de calibración grabados en fábrica (datasheet, tabla 16). */
static struct {
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    uint8_t  H1; int16_t H2; uint8_t H3; int16_t H4, H5; int8_t H6;
} cal;

static esp_err_t leer_calibracion(void)
{
    uint8_t b[26];
    esp_err_t err = leer_registros(REG_CALIB_TP, b, sizeof(b));
    if (err != ESP_OK) return err;

    // Cada coeficiente son 2 bytes en little endian (primero el byte bajo)
    cal.T1 = (uint16_t)(b[1]  << 8 | b[0]);
    cal.T2 = (int16_t) (b[3]  << 8 | b[2]);
    cal.T3 = (int16_t) (b[5]  << 8 | b[4]);
    cal.P1 = (uint16_t)(b[7]  << 8 | b[6]);
    cal.P2 = (int16_t) (b[9]  << 8 | b[8]);
    cal.P3 = (int16_t) (b[11] << 8 | b[10]);
    cal.P4 = (int16_t) (b[13] << 8 | b[12]);
    cal.P5 = (int16_t) (b[15] << 8 | b[14]);
    cal.P6 = (int16_t) (b[17] << 8 | b[16]);
    cal.P7 = (int16_t) (b[19] << 8 | b[18]);
    cal.P8 = (int16_t) (b[21] << 8 | b[20]);
    cal.P9 = (int16_t) (b[23] << 8 | b[22]);
    cal.H1 = b[25];                              // 0xA1 (0xA0 no se usa)

    uint8_t h[7];
    err = leer_registros(REG_CALIB_H, h, sizeof(h));
    if (err != ESP_OK) return err;

    cal.H2 = (int16_t)(h[1] << 8 | h[0]);
    cal.H3 = h[2];
    // H4 y H5 son de 12 bits y comparten el byte 0xE5
    cal.H4 = (int16_t)((int8_t)h[3] * 16 | (h[4] & 0x0F));
    cal.H5 = (int16_t)((int8_t)h[5] * 16 | (h[4] >> 4));
    cal.H6 = (int8_t)h[6];
    return ESP_OK;
}

esp_err_t bme280_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = BME280_DIR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle(), &cfg, &dev));

    // Reset por software: deja el sensor en modo sleep, donde acepta la configuración
    escribir_registro(REG_RESET, CMD_RESET);
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t id;
    esp_err_t err = leer_registros(REG_CHIP_ID, &id, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No responde en 0x%02X: %s", BME280_DIR, esp_err_to_name(err));
        return err;
    }
    if (id == ID_BMP280) {
        ESP_LOGE(TAG, "ID 0x%02X: es un BMP280 (sin humedad)", id);
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (id != ID_BME280) {
        ESP_LOGE(TAG, "ID 0x%02X desconocido", id);
        return ESP_ERR_NOT_FOUND;
    }

    err = leer_calibracion();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error leyendo la calibracion: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Calibracion: T1=%u T2=%d P1=%u H1=%u H2=%d",
             cal.T1, cal.T2, cal.P1, cal.H1, cal.H2);

    // ctrl_hum solo se aplica al escribir después ctrl_meas, así que va primero
    err = escribir_registro(REG_CTRL_HUM, VAL_CTRL_HUM);
    if (err == ESP_OK) err = escribir_registro(REG_CONFIG, VAL_CONFIG);
    if (err == ESP_OK) err = escribir_registro(REG_CTRL_MEAS, VAL_CTRL_MEAS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error configurando: %s", esp_err_to_name(err));
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(50));   // Espera a la primera medida (unos 10 ms con x1)

    uint8_t ctrl[4];   // 0xF2 a 0xF5: ctrl_hum, status, ctrl_meas, config
    leer_registros(REG_CTRL_HUM, ctrl, sizeof(ctrl));
    ESP_LOGI(TAG, "Registros: ctrl_hum=0x%02X ctrl_meas=0x%02X config=0x%02X",
             ctrl[0], ctrl[2], ctrl[3]);

    ESP_LOGI(TAG, "BME280 detectado en 0x%02X (ID 0x%02X)", BME280_DIR, id);
    return ESP_OK;
}

esp_err_t bme280_leer(float *temp, float *hum, float *pres)
{
    uint8_t d[8];
    esp_err_t err = leer_registros(REG_DATOS, d, sizeof(d));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Error de lectura: %s", esp_err_to_name(err));
        return err;
    }

    // Valores en bruto: presión y temperatura de 20 bits, humedad de 16
    int32_t adc_P = (int32_t)d[0] << 12 | (int32_t)d[1] << 4 | d[2] >> 4;
    int32_t adc_T = (int32_t)d[3] << 12 | (int32_t)d[4] << 4 | d[5] >> 4;
    int32_t adc_H = (int32_t)d[6] << 8  | d[7];

    // Temperatura (resultado en centésimas de °C). t_fine se usa
    // también para compensar la presión y la humedad.
    int32_t v1 = (((adc_T >> 3) - ((int32_t)cal.T1 << 1)) * cal.T2) >> 11;
    int32_t v2 = (((((adc_T >> 4) - (int32_t)cal.T1) *
                    ((adc_T >> 4) - (int32_t)cal.T1)) >> 12) * cal.T3) >> 14;
    int32_t t_fine = v1 + v2;
    *temp = ((t_fine * 5 + 128) >> 8) / 100.0f;

    // Presión (resultado en Pa con 8 bits de decimales)
    int64_t p1 = (int64_t)t_fine - 128000;
    int64_t p2 = p1 * p1 * cal.P6;
    p2 += (p1 * cal.P5) << 17;
    p2 += (int64_t)cal.P4 << 35;
    p1 = ((p1 * p1 * cal.P3) >> 8) + ((p1 * cal.P2) << 12);
    p1 = ((((int64_t)1 << 47) + p1) * cal.P1) >> 33;
    if (p1 == 0) return ESP_ERR_INVALID_RESPONSE;   // Evita dividir entre 0
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - p2) * 3125) / p1;
    p1 = ((int64_t)cal.P9 * (p >> 13) * (p >> 13)) >> 25;
    p2 = ((int64_t)cal.P8 * p) >> 19;
    p = ((p + p1 + p2) >> 8) + ((int64_t)cal.P7 << 4);
    *pres = (uint32_t)p / 256.0f / 100.0f;           // Pa -> hPa

    // Humedad (resultado en % con 10 bits de decimales)
    int32_t h = t_fine - 76800;
    h = (((((adc_H << 14) - ((int32_t)cal.H4 << 20) - ((int32_t)cal.H5 * h)) + 16384) >> 15) *
         (((((((h * (int32_t)cal.H6) >> 10) * (((h * (int32_t)cal.H3) >> 11) + 32768)) >> 10) +
            2097152) * (int32_t)cal.H2 + 8192) >> 14));
    h -= (((((h >> 15) * (h >> 15)) >> 7) * (int32_t)cal.H1) >> 4);
    if (h < 0) h = 0;
    if (h > 419430400) h = 419430400;                // Limita a 0–100 %
    *hum = (uint32_t)(h >> 12) / 1024.0f;

    return ESP_OK;
}